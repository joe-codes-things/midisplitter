#pragma once

#include <pluginterfaces/vst/ivstmessage.h>
#include <atomic>
#include <limits>
#include <vector>
#include <memory>
#include <functional>
#include "IPCMessage.h"
#include "VoiceAssignment.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

/**
 * IPCManagerVST3 class for VST3 inter-process communication.
 * 
 * Implements VST3's IConnectionPoint interface to enable communication
 * between Sender and Receiver plugin instances. Handles encoding/decoding
 * of voice assignment data and MIDI events for transmission.
 * 
 * Requirements: 8.1, 8.2, 8.3, 8.4, 8.5, 8.6, 8.7
 */
class IPCManagerVST3 : public IConnectionPoint
{
public:
    /**
     * Constructor
     */
    IPCManagerVST3();
    
    /**
     * Destructor
     */
    virtual ~IPCManagerVST3();
    
    // IConnectionPoint interface implementation
    tresult PLUGIN_API connect(IConnectionPoint* other) override;
    tresult PLUGIN_API disconnect(IConnectionPoint* other) override;
    tresult PLUGIN_API notify(IMessage* message) override;
    
    // IUnknown interface implementation
    tresult PLUGIN_API queryInterface(const TUID interfaceId, void** obj) override;
    uint32 PLUGIN_API addRef() override;
    uint32 PLUGIN_API release() override;
    
    /**
     * Send voice assignment data to connected receivers
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
     * Check if connected to other plugin instances
     * @return true if connected
     */
    bool isConnected() const;
    
    /**
     * Get number of connected instances
     * @return Number of connections
     */
    size_t getConnectionCount() const;

    /**
     * Disconnect from all connected instances.
     */
    void disconnectAll();
    
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
    
private:
    // Reference counting
    std::atomic<uint32> refCount_;
    
    // Connection management
    std::vector<IPtr<IConnectionPoint>> connections_;
    bool isSender_;
    uint8_t voiceNumber_;
    
    // Message handling
    std::function<void(const std::vector<MIDIEvent>&, uint8_t)> receiveCallback_;
    
    // Message IDs
    static constexpr const char* MSG_VOICE_DATA = "VoiceData";
    static constexpr const char* MSG_CONTROL_CHANGE = "ControlChange";
    
    /**
     * Encode voice assignment into IMessage
     * @param assignment Voice assignment to encode
     * @param message Target IMessage object
     * @return true if encoding successful
     */
    bool encodeMessage(const VoiceAssignment& assignment, IMessage* message);
    
    /**
     * Decode IMessage into voice data
     * @param message Source IMessage object
     * @param events Output vector for MIDI events
     * @param voiceNumber Output voice number
     * @return true if decoding successful
     */
    bool decodeMessage(IMessage* message, std::vector<MIDIEvent>& events, uint8_t& voiceNumber);
    
    /**
     * Create a new VST3 message
     * @param messageId Message identifier string
     * @return New IMessage object, or nullptr if failed
     */
    IPtr<IMessage> createMessage(const char* messageId);
    
    /**
     * Send message to all connected instances
     * @param message Message to send
     * @return true if sent to at least one connection
     */
    bool broadcastMessage(IMessage* message);
    
    /**
     * Handle incoming voice data message
     * @param message Received message
     */
    void handleVoiceDataMessage(IMessage* message);
    
    /**
     * Handle incoming control change message
     * @param message Received message
     */
    void handleControlChangeMessage(IMessage* message);
    
    /**
     * Serialize MIDI events to binary data
     * @param events Vector of MIDI events
     * @param data Output binary data
     * @return true if serialization successful
     */
    bool serializeEvents(const std::vector<MIDIEvent>& events, std::vector<uint8_t>& data);
    
    /**
     * Deserialize binary data to MIDI events
     * @param data Input binary data
     * @param events Output vector of MIDI events
     * @return true if deserialization successful
     */
    bool deserializeEvents(const std::vector<uint8_t>& data, std::vector<MIDIEvent>& events);
};

// Implementation

inline IPCManagerVST3::IPCManagerVST3()
    : refCount_(1)
    , isSender_(true)
    , voiceNumber_(1)
{
}

inline IPCManagerVST3::~IPCManagerVST3()
{
    // Disconnect from all connections
    for (auto& connection : connections_)
    {
        if (connection)
        {
            connection->disconnect(this);
        }
    }
    connections_.clear();
}

inline tresult PLUGIN_API IPCManagerVST3::connect(IConnectionPoint* other)
{
    if (!other)
        return kInvalidArgument;
        
    // Check if already connected
    for (const auto& connection : connections_)
    {
        if (connection.get() == other)
            return kResultOk;  // Already connected
    }
    
    // Add new connection
    connections_.push_back(IPtr<IConnectionPoint>(other, false));
    other->addRef();
    
    return kResultOk;
}

inline tresult PLUGIN_API IPCManagerVST3::disconnect(IConnectionPoint* other)
{
    if (!other)
        return kInvalidArgument;
        
    // Find and remove connection
    auto it = std::find_if(connections_.begin(), connections_.end(),
                          [other](const IPtr<IConnectionPoint>& conn) {
                              return conn.get() == other;
                          });
                          
    if (it != connections_.end())
    {
        connections_.erase(it);
        return kResultOk;
    }
    
    return kResultFalse;
}

inline tresult PLUGIN_API IPCManagerVST3::notify(IMessage* message)
{
    if (!message)
        return kInvalidArgument;
        
    const char* messageId = message->getMessageID();
    if (!messageId)
        return kInvalidArgument;
        
    if (strcmp(messageId, MSG_VOICE_DATA) == 0)
    {
        handleVoiceDataMessage(message);
    }
    else if (strcmp(messageId, MSG_CONTROL_CHANGE) == 0)
    {
        handleControlChangeMessage(message);
    }
    
    return kResultOk;
}

inline tresult PLUGIN_API IPCManagerVST3::queryInterface(const TUID interfaceId, void** obj)
{
    QUERY_INTERFACE(interfaceId, obj, IConnectionPoint::iid, IConnectionPoint)
    QUERY_INTERFACE(interfaceId, obj, FUnknown::iid, IConnectionPoint)
    *obj = nullptr;
    return kNoInterface;
}

inline uint32 PLUGIN_API IPCManagerVST3::addRef()
{
    return refCount_.fetch_add(1, std::memory_order_relaxed) + 1;
}

inline uint32 PLUGIN_API IPCManagerVST3::release()
{
    uint32 count = refCount_.fetch_sub(1, std::memory_order_acq_rel) - 1;
    if (count == 0)
        delete this;
    return count;
}

inline bool IPCManagerVST3::sendVoiceData(const std::vector<VoiceAssignment>& assignments)
{
    if (!isSender_ || connections_.empty())
        return false;
        
    bool sentAny = false;
    
    // Send each voice assignment as a separate message
    for (const auto& assignment : assignments)
    {
        auto message = createMessage(MSG_VOICE_DATA);
        if (message && encodeMessage(assignment, message))
        {
            if (broadcastMessage(message))
            {
                sentAny = true;
            }
        }
    }
    
    return sentAny;
}

inline void IPCManagerVST3::setReceiveCallback(std::function<void(const std::vector<MIDIEvent>&, uint8_t)> callback)
{
    receiveCallback_ = callback;
}

inline bool IPCManagerVST3::isConnected() const
{
    return !connections_.empty();
}

inline size_t IPCManagerVST3::getConnectionCount() const
{
    return connections_.size();
}

inline void IPCManagerVST3::disconnectAll()
{
    for (auto& connection : connections_)
    {
        if (connection)
            connection->disconnect(this);
    }

    connections_.clear();
}

inline void IPCManagerVST3::setMode(bool isSender)
{
    isSender_ = isSender;
}

inline void IPCManagerVST3::setVoiceNumber(uint8_t voiceNumber)
{
    if (voiceNumber >= 1 && voiceNumber <= 8)
    {
        voiceNumber_ = voiceNumber;
    }
}

inline bool IPCManagerVST3::encodeMessage(const VoiceAssignment& assignment, IMessage* message)
{
    if (!message)
        return false;
        
    // Set voice number
    message->getAttributes()->setInt("voiceNumber", assignment.voiceNumber);
    
    // Serialize MIDI events
    std::vector<uint8_t> eventData;
    if (!serializeEvents(assignment.notes, eventData))
        return false;
        
    // Set event data
    message->getAttributes()->setBinary("eventData", eventData.data(), 
                                       static_cast<uint32>(eventData.size()));
    
    return true;
}

inline bool IPCManagerVST3::decodeMessage(IMessage* message, std::vector<MIDIEvent>& events, uint8_t& voiceNumber)
{
    if (!message)
        return false;
        
    auto attributes = message->getAttributes();
    if (!attributes)
        return false;
        
    // Get voice number
    int64 voiceNum = 0;
    if (attributes->getInt("voiceNumber", voiceNum) != kResultOk)
        return false;
        
    if (voiceNum < 0 || voiceNum > std::numeric_limits<uint8_t>::max())
        return false;

    voiceNumber = static_cast<uint8_t>(voiceNum);
    
    // Get event data
    const void* data = nullptr;
    uint32 size = 0;
    if (attributes->getBinary("eventData", data, size) != kResultOk)
        return false;
        
    // Deserialize events
    std::vector<uint8_t> eventData(static_cast<const uint8_t*>(data), 
                                  static_cast<const uint8_t*>(data) + size);
    
    return deserializeEvents(eventData, events);
}

inline IPtr<IMessage> IPCManagerVST3::createMessage(const char*)
{
    // This would typically be provided by the VST3 host
    // For now, return nullptr - actual implementation would need host context
    return nullptr;
}

inline bool IPCManagerVST3::broadcastMessage(IMessage* message)
{
    if (!message || connections_.empty())
        return false;
        
    bool sentAny = false;
    
    for (auto& connection : connections_)
    {
        if (connection && connection->notify(message) == kResultOk)
        {
            sentAny = true;
        }
    }
    
    return sentAny;
}

inline void IPCManagerVST3::handleVoiceDataMessage(IMessage* message)
{
    if (!receiveCallback_)
        return;
        
    std::vector<MIDIEvent> events;
    uint8_t voiceNumber = 0;
    
    if (decodeMessage(message, events, voiceNumber))
    {
        // Only process if this is our assigned voice (for Receiver mode)
        if (!isSender_ && voiceNumber == voiceNumber_)
        {
            receiveCallback_(events, voiceNumber);
        }
    }
}

inline void IPCManagerVST3::handleControlChangeMessage(IMessage* message)
{
    // Control changes are broadcast to all receivers
    if (!receiveCallback_)
        return;
        
    std::vector<MIDIEvent> events;
    uint8_t voiceNumber = 0;
    
    if (decodeMessage(message, events, voiceNumber))
    {
        receiveCallback_(events, voiceNumber);
    }
}

inline bool IPCManagerVST3::serializeEvents(const std::vector<MIDIEvent>& events, std::vector<uint8_t>& data)
{
    data.clear();
    
    // Simple serialization: count + events
    uint32_t eventCount = static_cast<uint32_t>(events.size());
    
    // Add event count
    data.resize(sizeof(eventCount));
    std::memcpy(data.data(), &eventCount, sizeof(eventCount));
    
    // Add events
    size_t eventSize = sizeof(MIDIEvent);
    size_t totalSize = sizeof(eventCount) + (eventCount * eventSize);
    data.resize(totalSize);
    
    uint8_t* ptr = data.data() + sizeof(eventCount);
    for (const auto& event : events)
    {
        std::memcpy(ptr, &event, eventSize);
        ptr += eventSize;
    }
    
    return true;
}

inline bool IPCManagerVST3::deserializeEvents(const std::vector<uint8_t>& data, std::vector<MIDIEvent>& events)
{
    events.clear();
    
    if (data.size() < sizeof(uint32_t))
        return false;
        
    // Read event count
    uint32_t eventCount = 0;
    std::memcpy(&eventCount, data.data(), sizeof(eventCount));
    
    // Validate data size
    size_t expectedSize = sizeof(eventCount) + (eventCount * sizeof(MIDIEvent));
    if (data.size() != expectedSize)
        return false;
        
    // Read events
    events.reserve(eventCount);
    const uint8_t* ptr = data.data() + sizeof(eventCount);
    
    for (uint32_t i = 0; i < eventCount; ++i)
    {
        MIDIEvent event;
        std::memcpy(&event, ptr, sizeof(MIDIEvent));
        events.push_back(event);
        ptr += sizeof(MIDIEvent);
    }
    
    return true;
}
