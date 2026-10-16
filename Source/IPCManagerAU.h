#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <functional>
#include <memory>
#include <atomic>
#include "IPCMessage.h"
#include "VoiceAssignment.h"

/**
 * IPCManagerAU class for Audio Units inter-process communication.
 * 
 * Uses JUCE's InterprocessConnection to enable communication between
 * Sender and Receiver AU instances via local sockets. Handles encoding/decoding
 * of voice assignment data and MIDI events for transmission.
 * 
 * Requirements: 9.1, 9.2, 9.3, 9.4, 9.5, 9.6
 */
class IPCManagerAU : public juce::InterprocessConnection
{
public:
    /**
     * Constructor
     */
    IPCManagerAU();
    
    /**
     * Destructor
     */
    virtual ~IPCManagerAU();
    
    // InterprocessConnection interface implementation
    void connectionMade() override;
    void connectionLost() override;
    void messageReceived(const juce::MemoryBlock& message) override;
    
    /**
     * Start as server (Sender mode)
     * @param sessionId Unique session identifier for socket naming
     * @return true if server started successfully
     */
    bool startServer(const juce::String& sessionId);
    
    /**
     * Connect to server (Receiver mode)
     * @param sessionId Session identifier to connect to
     * @return true if connection established
     */
    bool connectToServer(const juce::String& sessionId);
    
    /**
     * Send voice assignment data to connected clients
     * @param assignments Vector of voice assignments to send
     * @return true if sent successfully
     */
    bool sendVoiceData(const std::vector<VoiceAssignment>& assignments);
    
    /**
     * Set callback for receiving voice data
     * @param callback Function to call when voice data is received
     */
    void setReceiveCallback(std::function<void(const std::vector<MIDIEvent>&, uint8_t)> callback);
    
    /**
     * Check if connected
     * @return true if connected to server or has clients
     */
    bool isConnected() const;
    
    /**
     * Set plugin mode (Sender or Receiver)
     * @param isSender true for Sender mode, false for Receiver mode
     */
    void setMode(bool isSender);
    
    /**
     * Set voice number for Receiver mode
     * @param voiceNumber Voice number to listen for (1-8)
     */
    void setVoiceNumber(uint8_t voiceNumber);
    
    /**
     * Get current session ID
     * @return Session identifier string
     */
    juce::String getSessionId() const;
    
    /**
     * Disconnect and cleanup
     */
    void shutdown();
    
    /**
     * Send control change message to all receivers
     * @param controlEvent Control change MIDI event
     * @return true if sent successfully
     */
    bool sendControlChange(const MIDIEvent& controlEvent);
    
private:
    // Connection state
    std::atomic<bool> isServer_;
    std::atomic<bool> connected_;
    bool isSender_;
    uint8_t voiceNumber_;
    juce::String sessionId_;
    
    // Message handling
    std::function<void(const std::vector<MIDIEvent>&, uint8_t)> receiveCallback_;
    
    // Message types
    enum class MessageType : uint32_t
    {
        VoiceData = 0x56444154,      // "VDAT"
        ControlChange = 0x43434348,  // "CCCH"
        Heartbeat = 0x48424554       // "HBET"
    };
    
    // Message header structure
    struct MessageHeader
    {
        uint32_t magic = 0x4D494449;  // "MIDI"
        uint32_t version = 1;
        MessageType type;
        uint32_t dataSize;
        uint8_t voiceNumber;
        uint8_t reserved[3] = {0};
    };
    
    /**
     * Generate socket name from session ID
     * @param sessionId Session identifier
     * @return Socket name for InterprocessConnection
     */
    juce::String generateSocketName(const juce::String& sessionId) const;
    
    /**
     * Encode voice assignment into message
     * @param assignment Voice assignment to encode
     * @param message Output memory block
     * @return true if encoding successful
     */
    bool encodeMessage(const VoiceAssignment& assignment, juce::MemoryBlock& message);
    
    /**
     * Decode message into voice data
     * @param message Input memory block
     * @param events Output vector for MIDI events
     * @param voiceNumber Output voice number
     * @return true if decoding successful
     */
    bool decodeMessage(const juce::MemoryBlock& message, std::vector<MIDIEvent>& events, uint8_t& voiceNumber);
    
    /**
     * Send message with header
     * @param type Message type
     * @param voiceNumber Voice number
     * @param data Message data
     * @return true if sent successfully
     */
    bool sendMessage(MessageType type, uint8_t voiceNumber, const juce::MemoryBlock& data);
    
    /**
     * Handle incoming voice data message
     * @param header Message header
     * @param data Message data
     */
    void handleVoiceDataMessage(const MessageHeader& header, const juce::MemoryBlock& data);
    
    /**
     * Handle incoming control change message
     * @param header Message header
     * @param data Message data
     */
    void handleControlChangeMessage(const MessageHeader& header, const juce::MemoryBlock& data);
    
    /**
     * Handle heartbeat message
     * @param header Message header
     */
    void handleHeartbeatMessage(const MessageHeader& header);
    
    /**
     * Serialize MIDI events to binary data
     * @param events Vector of MIDI events
     * @param data Output memory block
     * @return true if serialization successful
     */
    bool serializeEvents(const std::vector<MIDIEvent>& events, juce::MemoryBlock& data);
    
    /**
     * Deserialize binary data to MIDI events
     * @param data Input memory block
     * @param events Output vector of MIDI events
     * @return true if deserialization successful
     */
    bool deserializeEvents(const juce::MemoryBlock& data, std::vector<MIDIEvent>& events);
    
    /**
     * Validate message header
     * @param header Message header to validate
     * @return true if header is valid
     */
    bool validateHeader(const MessageHeader& header) const;
};

// Implementation

inline IPCManagerAU::IPCManagerAU()
    : isServer_(false)
    , connected_(false)
    , isSender_(true)
    , voiceNumber_(1)
{
}

inline IPCManagerAU::~IPCManagerAU()
{
    shutdown();
}

inline void IPCManagerAU::connectionMade()
{
    connected_ = true;
    
    // Send heartbeat to establish connection
    juce::MemoryBlock emptyData;
    sendMessage(MessageType::Heartbeat, 0, emptyData);
}

inline void IPCManagerAU::connectionLost()
{
    connected_ = false;
}

inline void IPCManagerAU::messageReceived(const juce::MemoryBlock& message)
{
    if (message.getSize() < sizeof(MessageHeader))
        return;
        
    MessageHeader header;
    std::memcpy(&header, message.getData(), sizeof(MessageHeader));
    
    if (!validateHeader(header))
        return;
        
    // Extract message data
    juce::MemoryBlock data;
    if (header.dataSize > 0)
    {
        size_t dataOffset = sizeof(MessageHeader);
        if (message.getSize() >= dataOffset + header.dataSize)
        {
            data.setSize(header.dataSize);
            std::memcpy(data.getData(), 
                       static_cast<const uint8_t*>(message.getData()) + dataOffset,
                       header.dataSize);
        }
    }
    
    // Handle message based on type
    switch (header.type)
    {
        case MessageType::VoiceData:
            handleVoiceDataMessage(header, data);
            break;
            
        case MessageType::ControlChange:
            handleControlChangeMessage(header, data);
            break;
            
        case MessageType::Heartbeat:
            handleHeartbeatMessage(header);
            break;
    }
}

inline bool IPCManagerAU::startServer(const juce::String& sessionId)
{
    sessionId_ = sessionId;
    juce::String socketName = generateSocketName(sessionId);
    
    if (createPipe(socketName, 5000))  // 5 second timeout
    {
        isServer_ = true;
        return true;
    }
    
    return false;
}

inline bool IPCManagerAU::connectToServer(const juce::String& sessionId)
{
    sessionId_ = sessionId;
    juce::String socketName = generateSocketName(sessionId);
    
    if (connectToPipe(socketName, 5000))  // 5 second timeout
    {
        isServer_ = false;
        return true;
    }
    
    return false;
}

inline bool IPCManagerAU::sendVoiceData(const std::vector<VoiceAssignment>& assignments)
{
    if (!connected_ || !isSender_)
        return false;
        
    bool sentAny = false;
    
    // Send each voice assignment as a separate message
    for (const auto& assignment : assignments)
    {
        juce::MemoryBlock messageData;
        if (encodeMessage(assignment, messageData))
        {
            if (sendMessage(MessageType::VoiceData, assignment.voiceNumber, messageData))
            {
                sentAny = true;
            }
        }
    }
    
    return sentAny;
}

inline void IPCManagerAU::setReceiveCallback(std::function<void(const std::vector<MIDIEvent>&, uint8_t)> callback)
{
    receiveCallback_ = callback;
}

inline bool IPCManagerAU::isConnected() const
{
    return connected_;
}

inline void IPCManagerAU::setMode(bool isSender)
{
    isSender_ = isSender;
}

inline void IPCManagerAU::setVoiceNumber(uint8_t voiceNumber)
{
    if (voiceNumber >= 1 && voiceNumber <= 8)
    {
        voiceNumber_ = voiceNumber;
    }
}

inline juce::String IPCManagerAU::getSessionId() const
{
    return sessionId_;
}

inline void IPCManagerAU::shutdown()
{
    disconnect();
    connected_ = false;
    isServer_ = false;
}

inline bool IPCManagerAU::sendControlChange(const MIDIEvent& controlEvent)
{
    if (!connected_ || !isSender_)
        return false;
        
    std::vector<MIDIEvent> events = { controlEvent };
    juce::MemoryBlock messageData;
    
    if (serializeEvents(events, messageData))
    {
        return sendMessage(MessageType::ControlChange, 0, messageData);
    }
    
    return false;
}

inline juce::String IPCManagerAU::generateSocketName(const juce::String& sessionId) const
{
    return "MIDISplitter_" + sessionId;
}

inline bool IPCManagerAU::encodeMessage(const VoiceAssignment& assignment, juce::MemoryBlock& message)
{
    return serializeEvents(assignment.notes, message);
}

inline bool IPCManagerAU::decodeMessage(const juce::MemoryBlock& message, std::vector<MIDIEvent>& events, uint8_t& voiceNumber)
{
    return deserializeEvents(message, events);
}

inline bool IPCManagerAU::sendMessage(MessageType type, uint8_t voiceNumber, const juce::MemoryBlock& data)
{
    if (!connected_)
        return false;
        
    // Create message with header
    MessageHeader header;
    header.type = type;
    header.dataSize = static_cast<uint32_t>(data.getSize());
    header.voiceNumber = voiceNumber;
    
    juce::MemoryBlock fullMessage;
    fullMessage.setSize(sizeof(MessageHeader) + data.getSize());
    
    // Copy header
    std::memcpy(fullMessage.getData(), &header, sizeof(MessageHeader));
    
    // Copy data
    if (data.getSize() > 0)
    {
        std::memcpy(static_cast<uint8_t*>(fullMessage.getData()) + sizeof(MessageHeader),
                   data.getData(), data.getSize());
    }
    
    return juce::InterprocessConnection::sendMessage(fullMessage);
}

inline void IPCManagerAU::handleVoiceDataMessage(const MessageHeader& header, const juce::MemoryBlock& data)
{
    if (!receiveCallback_)
        return;
        
    std::vector<MIDIEvent> events;
    if (deserializeEvents(data, events))
    {
        // Only process if this is our assigned voice (for Receiver mode)
        if (!isSender_ && header.voiceNumber == voiceNumber_)
        {
            receiveCallback_(events, header.voiceNumber);
        }
    }
}

inline void IPCManagerAU::handleControlChangeMessage(const MessageHeader& header, const juce::MemoryBlock& data)
{
    // Control changes are broadcast to all receivers
    if (!receiveCallback_)
        return;
        
    std::vector<MIDIEvent> events;
    if (deserializeEvents(data, events))
    {
        receiveCallback_(events, header.voiceNumber);
    }
}

inline void IPCManagerAU::handleHeartbeatMessage(const MessageHeader& header)
{
    // Heartbeat received - connection is alive
    // Could implement connection health monitoring here
}

inline bool IPCManagerAU::serializeEvents(const std::vector<MIDIEvent>& events, juce::MemoryBlock& data)
{
    // Simple serialization: count + events
    uint32_t eventCount = static_cast<uint32_t>(events.size());
    size_t totalSize = sizeof(eventCount) + (eventCount * sizeof(MIDIEvent));
    
    data.setSize(totalSize);
    
    // Write event count
    std::memcpy(data.getData(), &eventCount, sizeof(eventCount));
    
    // Write events
    uint8_t* ptr = static_cast<uint8_t*>(data.getData()) + sizeof(eventCount);
    for (const auto& event : events)
    {
        std::memcpy(ptr, &event, sizeof(MIDIEvent));
        ptr += sizeof(MIDIEvent);
    }
    
    return true;
}

inline bool IPCManagerAU::deserializeEvents(const juce::MemoryBlock& data, std::vector<MIDIEvent>& events)
{
    events.clear();
    
    if (data.getSize() < sizeof(uint32_t))
        return false;
        
    // Read event count
    uint32_t eventCount = 0;
    std::memcpy(&eventCount, data.getData(), sizeof(eventCount));
    
    // Validate data size
    size_t expectedSize = sizeof(eventCount) + (eventCount * sizeof(MIDIEvent));
    if (data.getSize() != expectedSize)
        return false;
        
    // Read events
    events.reserve(eventCount);
    const uint8_t* ptr = static_cast<const uint8_t*>(data.getData()) + sizeof(eventCount);
    
    for (uint32_t i = 0; i < eventCount; ++i)
    {
        MIDIEvent event;
        std::memcpy(&event, ptr, sizeof(MIDIEvent));
        events.push_back(event);
        ptr += sizeof(MIDIEvent);
    }
    
    return true;
}

inline bool IPCManagerAU::validateHeader(const MessageHeader& header) const
{
    return (header.magic == 0x4D494449) &&  // "MIDI"
           (header.version == 1) &&
           (header.voiceNumber <= 8);
}
