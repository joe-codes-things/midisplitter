#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <functional>
#include <memory>
#include <atomic>
#include <array>
#include <algorithm>
#include "IPCMessage.h"
#include "VoiceAssignment.h"
#include "IPCTraceLogger.h"
#include "LockFreeQueue.h"

/**
 * File-based VST3 inter-process transport for the experimental MIDI bridge.
 * 
 * Sender events are queued without file I/O on the audio thread. A worker
 * writes sequenced event files, and Receiver workers consume their voice's
 * files once.
 * 
 * This replaces the IConnectionPoint-based IPCManagerVST3 which cannot
 * communicate between separate plugin instances.
 */
class IPCManagerVST3_Socket : private juce::Thread
{
public:
    static constexpr const char* EXPERIMENT_SESSION_ID = "vst3-midi-bridge-experiment";

    /**
     * Constructor
     */
    IPCManagerVST3_Socket();
    
    /**
     * Destructor
     */
    ~IPCManagerVST3_Socket() override;
    
    /**
     * Start as server (Sender mode)
     * @param sessionId Unique session identifier
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
     * @param voiceNumber Voice number to listen for (1-16)
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
    
    /**
     * Get number of active connections
     * @return Connection count
     */
    int getConnectionCount() const;
    
    /**
     * Disconnect all clients (Sender mode)
     */
    void disconnectAll();
    
    /**
     * Poll for incoming data (Receiver mode)
     * Checks for new files and calls receive callback if data found
     * @return true if data was received
     */
    bool pollForData();

private:
    struct VoicePacket
    {
        uint8_t voiceNumber = 0;
        uint16_t eventCount = 0;
        std::array<MIDIEvent, IPCMessage::MAX_EVENTS_PER_MESSAGE> events{};
    };

    void run() override;
    bool writeVoiceData(const VoicePacket& packet);

    // Configuration
    bool isSender_ = false;
    uint8_t voiceNumber_ = 1;
    juce::String sessionId_;
    std::atomic<uint64_t> nextMessageSequence_{1};
    LockFreeQueue<VoicePacket> outgoingQueue_{64};
    static constexpr int MAX_PENDING_FILES_PER_VOICE = 256;
    
    // Callback for received data
    std::function<void(const std::vector<MIDIEvent>&, uint8_t)> receiveCallback_;
    
    // Connection status
    std::atomic<bool> isConnected_{false};
    
    // File-based IPC helpers
    juce::File getIPCDirectory() const;
    juce::File getVoiceDataFile(int voiceNumber, uint64_t sequence) const;
    void clearSessionFiles() const;
    
    // Encode voice assignments to memory block
    juce::MemoryBlock encodeVoiceData(const std::vector<VoiceAssignment>& assignments);
    
    // Decode memory block to voice assignments
    std::vector<VoiceAssignment> decodeVoiceData(const juce::MemoryBlock& data);
};

// ============================================================================
// Implementation
// ============================================================================

inline IPCManagerVST3_Socket::IPCManagerVST3_Socket()
    : juce::Thread("MIDISplitter VST3 IPC")
    , isSender_(false)
    , voiceNumber_(1)
    , isConnected_(false)
{
}

inline IPCManagerVST3_Socket::~IPCManagerVST3_Socket()
{
    shutdown();
}

inline bool IPCManagerVST3_Socket::startServer(const juce::String& sessionId)
{
    sessionId_ = sessionId;
    isSender_ = true;
    isConnected_ = true;  // Server is always "connected" when running
    
    // Create the IPC directory if it doesn't exist
    juce::File ipcDir = getIPCDirectory();
    juce::Result result = ipcDir.createDirectory();
    
    // Log to IPC trace logger
    auto& logger = IPCTraceLogger::getInstance();
    
    if (!result.wasOk())
    {
        logger.logError("Sender", "VST3", "Failed to create IPC directory: " + ipcDir.getFullPathName() + " - " + result.getErrorMessage());
        DBG("Failed to create IPC directory: " << ipcDir.getFullPathName() << " - " << result.getErrorMessage());
        return false;
    }
    
    logger.logEvent(IPCTraceLogger::EventType::Info, "Sender", "VST3", 0,
                    "Created IPC directory: " + ipcDir.getFullPathName());
    DBG("Created IPC directory: " << ipcDir.getFullPathName());
    
    // Remove any existing data files for this session
    clearSessionFiles();

    if (startThread())
        return true;

    isConnected_ = false;
    logger.logError("Sender", "VST3", "Failed to start the VST3 IPC worker thread");
    return false;
}

inline bool IPCManagerVST3_Socket::connectToServer(const juce::String& sessionId)
{
    sessionId_ = sessionId;
    isSender_ = false;
    
    // Check if the IPC directory exists (means server is running)
    juce::File ipcDir = getIPCDirectory();
    isConnected_ = ipcDir.isDirectory();

    if (!isThreadRunning() && !startThread())
        return false;

    return isConnected_;
}

inline bool IPCManagerVST3_Socket::sendVoiceData(const std::vector<VoiceAssignment>& assignments)
{
    if (!isConnected_)
        return false;
    
    if (!isSender_)
        return false;  // Only sender can send data

    size_t packetsToQueue = 0;
    for (const auto& assignment : assignments)
    {
        if (assignment.notes.empty())
            continue;

        if (assignment.voiceNumber < 1 || assignment.voiceNumber > 16 ||
            assignment.notes.size() > IPCMessage::MAX_EVENTS_PER_MESSAGE)
            return false;

        ++packetsToQueue;
    }

    if (packetsToQueue == 0)
        return true;

    if (outgoingQueue_.getFreeSpace() < static_cast<int>(packetsToQueue))
        return false;

    for (const auto& assignment : assignments)
    {
        if (assignment.notes.empty())
            continue;

        VoicePacket packet;
        packet.voiceNumber = static_cast<uint8_t>(assignment.voiceNumber);
        packet.eventCount = static_cast<uint16_t>(assignment.notes.size());
        std::copy(assignment.notes.begin(), assignment.notes.end(), packet.events.begin());

        if (!outgoingQueue_.push(packet))
            return false;
    }

    return true;
}

inline void IPCManagerVST3_Socket::setReceiveCallback(std::function<void(const std::vector<MIDIEvent>&, uint8_t)> callback)
{
    receiveCallback_ = callback;
}

inline bool IPCManagerVST3_Socket::isConnected() const
{
    return isConnected_.load();
}

inline void IPCManagerVST3_Socket::setMode(bool isSender)
{
    isSender_ = isSender;
}

inline void IPCManagerVST3_Socket::setVoiceNumber(uint8_t voiceNumber)
{
    voiceNumber_ = voiceNumber;
}

inline juce::String IPCManagerVST3_Socket::getSessionId() const
{
    return sessionId_;
}

inline void IPCManagerVST3_Socket::shutdown()
{
    stopThread(-1);

    if (isSender_)
    {
        // Clear IPC files but keep the directory
        // This allows receivers to detect the sender is still present
        clearSessionFiles();
        
        // DON'T delete the IPC directory - it should persist across plugin reloads
        // The directory will be cleaned up on system restart (it's in /tmp)
    }
    
    isConnected_ = false;
}

inline bool IPCManagerVST3_Socket::sendControlChange(const MIDIEvent& controlEvent)
{
    std::vector<VoiceAssignment> assignments;
    assignments.reserve(16);
    for (int voice = 1; voice <= 16; ++voice)
    {
        VoiceAssignment assignment(voice);
        assignment.notes.push_back(controlEvent);
        assignments.push_back(std::move(assignment));
    }
    return sendVoiceData(assignments);
}

inline int IPCManagerVST3_Socket::getConnectionCount() const
{
    // File-based IPC: return 1 if connected
    return isConnected_.load() ? 1 : 0;
}

inline void IPCManagerVST3_Socket::disconnectAll()
{
    // Remove all IPC files
    clearSessionFiles();
}

inline void IPCManagerVST3_Socket::run()
{
    while (!threadShouldExit())
    {
        if (isSender_)
        {
            VoicePacket packet;
            if (outgoingQueue_.pop(packet))
            {
                if (!writeVoiceData(packet))
                    IPCTraceLogger::getInstance().logError("Sender", "VST3", "Failed to write queued voice data");
                continue;
            }
        }
        else
        {
            isConnected_ = getIPCDirectory().isDirectory();
            if (isConnected_)
                pollForData();
        }

        juce::Thread::sleep(2);
    }
}

inline bool IPCManagerVST3_Socket::writeVoiceData(const VoicePacket& packet)
{
    VoiceAssignment assignment(static_cast<int>(packet.voiceNumber));
    assignment.notes.assign(packet.events.begin(), packet.events.begin() + packet.eventCount);

    const uint64_t sequence = nextMessageSequence_.fetch_add(1);
    const juce::File dataFile = getVoiceDataFile(packet.voiceNumber, sequence);
    const juce::MemoryBlock data = encodeVoiceData({ assignment });

    juce::TemporaryFile tempFile(dataFile);
    if (!tempFile.getFile().replaceWithData(data.getData(), data.getSize()) ||
        !tempFile.overwriteTargetFileWithTemporary())
    {
        IPCTraceLogger::getInstance().logError(
            "Sender", "VST3", "Failed to write IPC data file: " + dataFile.getFullPathName());
        return false;
    }

    juce::Array<juce::File> foundFiles;
    getIPCDirectory().findChildFiles(
        foundFiles, juce::File::findFiles, false,
        "voice_" + juce::String(packet.voiceNumber) + "_*.dat");

    if (foundFiles.size() > MAX_PENDING_FILES_PER_VOICE)
    {
        std::vector<juce::File> files(foundFiles.begin(), foundFiles.end());
        std::sort(files.begin(), files.end(), [](const juce::File& left, const juce::File& right) {
            return left.getFileName() < right.getFileName();
        });

        const int filesToRemove = static_cast<int>(files.size()) - MAX_PENDING_FILES_PER_VOICE;
        for (int index = 0; index < filesToRemove; ++index)
        {
            if (!files[static_cast<size_t>(index)].deleteFile())
                IPCTraceLogger::getInstance().logError(
                    "Sender", "VST3", "Failed to remove expired IPC data file: " +
                                          files[static_cast<size_t>(index)].getFullPathName());
        }
    }

    return true;
}

inline juce::MemoryBlock IPCManagerVST3_Socket::encodeVoiceData(const std::vector<VoiceAssignment>& assignments)
{
    juce::MemoryOutputStream stream;
    
    // Write number of assignments
    stream.writeInt(static_cast<int>(assignments.size()));
    
    for (const auto& assignment : assignments)
    {
        // Write voice number
        stream.writeInt(assignment.voiceNumber);
        
        // Write number of events
        stream.writeInt(static_cast<int>(assignment.notes.size()));
        
        // Write each event
        for (const auto& event : assignment.notes)
        {
            stream.writeInt64(event.timestamp);
            stream.writeByte(static_cast<juce::int8>(event.status));
            stream.writeByte(static_cast<juce::int8>(event.note));
            stream.writeByte(static_cast<juce::int8>(event.velocity));
            stream.writeByte(static_cast<juce::int8>(event.channel));
        }
    }
    
    return stream.getMemoryBlock();
}

inline std::vector<VoiceAssignment> IPCManagerVST3_Socket::decodeVoiceData(const juce::MemoryBlock& data)
{
    std::vector<VoiceAssignment> assignments;

    if (data.getSize() < sizeof(int32_t))
        return assignments;

    juce::MemoryInputStream stream(data, false);
    
    // Read number of assignments
    const int count = stream.readInt();
    if (count < 0 || count > 16)
        return assignments;
    
    for (int i = 0; i < count; ++i)
    {
        if (stream.getNumBytesRemaining() < static_cast<juce::int64>(2 * sizeof(int32_t)))
            return {};

        VoiceAssignment assignment(0);
        
        // Read voice number
        assignment.voiceNumber = stream.readInt();
        
        // Read number of events
        const int eventCount = stream.readInt();
        if (assignment.voiceNumber < 1 || assignment.voiceNumber > 16 ||
            eventCount < 0 || eventCount > static_cast<int>(IPCMessage::MAX_EVENTS_PER_MESSAGE) ||
            stream.getNumBytesRemaining() < static_cast<juce::int64>(eventCount) * 12)
            return {};
        
        // Read each event
        for (int j = 0; j < eventCount && !stream.isExhausted(); ++j)
        {
            MIDIEvent event;
            event.timestamp = stream.readInt64();
            event.status = static_cast<uint8_t>(stream.readByte());
            event.note = static_cast<uint8_t>(stream.readByte());
            event.velocity = static_cast<uint8_t>(stream.readByte());
            event.channel = static_cast<uint8_t>(stream.readByte());
            
            assignment.notes.push_back(event);
        }
        
        assignments.push_back(assignment);
    }

    return stream.getNumBytesRemaining() == 0 ? assignments : std::vector<VoiceAssignment>{};
}

inline juce::File IPCManagerVST3_Socket::getIPCDirectory() const
{
    juce::File tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
    juce::String safeSession = sessionId_.removeCharacters("/\\:*?\"<>|");
    return tempDir.getChildFile("MIDI_Splitter_IPC_" + safeSession);
}

inline juce::File IPCManagerVST3_Socket::getVoiceDataFile(int voiceNumber, uint64_t sequence) const
{
    const auto sequenceText = juce::String(static_cast<juce::int64>(sequence)).paddedLeft('0', 20);
    return getIPCDirectory().getChildFile(
        "voice_" + juce::String(voiceNumber) + "_" + sequenceText + ".dat");
}

inline void IPCManagerVST3_Socket::clearSessionFiles() const
{
    juce::File ipcDir = getIPCDirectory();
    if (ipcDir.exists())
    {
        juce::Array<juce::File> files;
        ipcDir.findChildFiles(files, juce::File::findFiles, false, "*.dat");
        for (auto& file : files)
        {
            file.deleteFile();
        }
    }
}

inline bool IPCManagerVST3_Socket::pollForData()
{
    if (isSender_ || !isConnected_)
        return false;

    juce::Array<juce::File> foundFiles;
    getIPCDirectory().findChildFiles(
        foundFiles, juce::File::findFiles, false,
        "voice_" + juce::String(voiceNumber_) + "_*.dat");

    std::vector<juce::File> dataFiles(foundFiles.begin(), foundFiles.end());
    std::sort(dataFiles.begin(), dataFiles.end(), [](const juce::File& left, const juce::File& right) {
        return left.getFileName() < right.getFileName();
    });

    bool receivedData = false;
    for (const auto& dataFile : dataFiles)
    {
        juce::MemoryBlock data;
        if (!dataFile.loadFileAsData(data))
        {
            IPCTraceLogger::getInstance().logError(
                "Receiver", "VST3", "Failed to read IPC data file: " + dataFile.getFullPathName());
            continue;
        }

        if (data.getSize() > 2 * sizeof(int32_t) +
                             IPCMessage::MAX_EVENTS_PER_MESSAGE * 12)
        {
            IPCTraceLogger::getInstance().logError(
                "Receiver", "VST3", "IPC data file exceeds the protocol size limit: " + dataFile.getFullPathName());
            dataFile.deleteFile();
            continue;
        }

        const auto assignments = decodeVoiceData(data);
        for (const auto& assignment : assignments)
        {
            if (assignment.voiceNumber == voiceNumber_ && receiveCallback_)
            {
                receiveCallback_(assignment.notes, static_cast<uint8_t>(assignment.voiceNumber));
                receivedData = true;
            }
        }

        if (!dataFile.deleteFile())
            IPCTraceLogger::getInstance().logError(
                "Receiver", "VST3", "Failed to remove consumed IPC data file: " + dataFile.getFullPathName());
    }

    return receivedData;
}
