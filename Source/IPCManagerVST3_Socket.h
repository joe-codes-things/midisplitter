#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <functional>
#include <memory>
#include <atomic>
#include "IPCMessage.h"
#include "VoiceAssignment.h"
#include "IPCTraceLogger.h"

/**
 * IPCManagerVST3_Socket class for VST3 inter-process communication via file-based IPC.
 * 
 * Uses JUCE's File and MemoryBlock to enable communication between
 * Sender and Receiver VST3 instances via files in temp directory. This works
 * reliably across process boundaries when Cubase sandboxes plugins.
 * 
 * This replaces the IConnectionPoint-based IPCManagerVST3 which cannot
 * communicate between separate plugin instances.
 */
class IPCManagerVST3_Socket
{
public:
    /**
     * Constructor
     */
    IPCManagerVST3_Socket();
    
    /**
     * Destructor
     */
    ~IPCManagerVST3_Socket();
    
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
    // Configuration
    bool isSender_ = false;
    uint8_t voiceNumber_ = 1;
    juce::String sessionId_;
    
    // Callback for received data
    std::function<void(const std::vector<MIDIEvent>&, uint8_t)> receiveCallback_;
    
    // Connection status
    std::atomic<bool> isConnected_{false};
    
    // File-based IPC helpers
    juce::File getIPCDirectory() const;
    juce::File getVoiceDataFile(int voiceNumber) const;
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
    : isSender_(false)
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
    
    return true;
}

inline bool IPCManagerVST3_Socket::connectToServer(const juce::String& sessionId)
{
    sessionId_ = sessionId;
    isSender_ = false;
    
    // Check if the IPC directory exists (means server is running)
    juce::File ipcDir = getIPCDirectory();
    if (!ipcDir.exists()) {
        return false;  // Server not running yet
    }
    
    isConnected_ = true;
    return true;
}

inline bool IPCManagerVST3_Socket::sendVoiceData(const std::vector<VoiceAssignment>& assignments)
{
    auto& logger = IPCTraceLogger::getInstance();
    
    if (!isConnected_)
    {
        logger.logError("Sender", "VST3", "sendVoiceData: not connected");
        DBG("sendVoiceData: not connected");
        return false;
    }
    
    if (!isSender_)
    {
        logger.logError("Sender", "VST3", "sendVoiceData: not sender");
        DBG("sendVoiceData: not sender");
        return false;  // Only sender can send data
    }
    
    logger.logEvent(IPCTraceLogger::EventType::MessageSent, "Sender", "VST3", 0,
                    "sendVoiceData: sending " + juce::String(assignments.size()) + " voice assignments");
    DBG("sendVoiceData: sending " << assignments.size() << " voice assignments");
    
    // Write each voice assignment to a separate file
    for (const auto& assignment : assignments)
    {
        juce::File dataFile = getVoiceDataFile(assignment.voiceNumber);
        
        logger.logEvent(IPCTraceLogger::EventType::MessageSent, "Sender", "VST3", assignment.voiceNumber,
                        "Writing to file: " + dataFile.getFullPathName() + " (" + juce::String(assignment.notes.size()) + " events)");
        DBG("Writing to file: " << dataFile.getFullPathName() << " (" << assignment.notes.size() << " events)");
        
        juce::MemoryBlock data = encodeVoiceData({assignment});
        
        // Write data to file (atomic write)
        juce::TemporaryFile tempFile(dataFile);
        if (tempFile.getFile().replaceWithData(data.getData(), data.getSize()))
        {
            if (!tempFile.overwriteTargetFileWithTemporary())
            {
                logger.logError("Sender", "VST3", "Failed to overwrite target file: " + dataFile.getFullPathName());
                DBG("Failed to overwrite target file: " << dataFile.getFullPathName());
                return false;
            }
            logger.logEvent(IPCTraceLogger::EventType::MessageSent, "Sender", "VST3", assignment.voiceNumber,
                            "Successfully wrote " + juce::String(data.getSize()) + " bytes");
            DBG("Successfully wrote " << data.getSize() << " bytes to " << dataFile.getFullPathName());
        }
        else
        {
            logger.logError("Sender", "VST3", "Failed to write data to temp file");
            DBG("Failed to write data to temp file");
            return false;
        }
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
    VoiceAssignment assignment(0); // Broadcast to all voices
    assignment.notes.push_back(controlEvent);
    return sendVoiceData({assignment});
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
    
    juce::MemoryInputStream stream(data, false);
    
    // Read number of assignments
    const int count = stream.readInt();
    if (count < 0)
        return assignments;
    
    for (int i = 0; i < count && !stream.isExhausted(); ++i)
    {
        VoiceAssignment assignment(0);
        
        // Read voice number
        assignment.voiceNumber = stream.readInt();
        
        // Read number of events
        const int eventCount = stream.readInt();
        if (eventCount < 0)
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
    
    return assignments;
}

inline juce::File IPCManagerVST3_Socket::getIPCDirectory() const
{
    juce::File tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
    juce::String safeSession = sessionId_.removeCharacters("/\\:*?\"<>|");
    return tempDir.getChildFile("MIDI_Splitter_IPC_" + safeSession);
}

inline juce::File IPCManagerVST3_Socket::getVoiceDataFile(int voiceNumber) const
{
    return getIPCDirectory().getChildFile("voice_" + juce::String(voiceNumber) + ".dat");
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
    if (isSender_)
        return false;  // Only receivers poll for data
    
    if (!isConnected_)
        return false;  // Not connected, can't poll
    
    juce::File dataFile = getVoiceDataFile(voiceNumber_);
    
    DBG("pollForData: checking " << dataFile.getFullPathName());
    
    if (!dataFile.exists())
    {
        DBG("pollForData: file does not exist");
        return false;  // No data yet
    }
    
    // Read the data file
    juce::MemoryBlock data;
    if (!dataFile.loadFileAsData(data))
    {
        DBG("Failed to read data file: " << dataFile.getFullPathName());
        return false;
    }
    
    DBG("pollForData: read " << data.getSize() << " bytes");
    
    // Parse the voice assignments
    auto assignments = decodeVoiceData(data);
    
    DBG("pollForData: decoded " << assignments.size() << " assignments");
    
    // Call the receive callback for matching voice
    for (const auto& assignment : assignments)
    {
        if (assignment.voiceNumber == voiceNumber_ && receiveCallback_)
        {
            DBG("pollForData: calling callback with " << assignment.notes.size() << " events for voice " << voiceNumber_);
            receiveCallback_(assignment.notes, static_cast<uint8_t>(assignment.voiceNumber));
            return true;  // Data received successfully
        }
    }
    
    DBG("pollForData: no matching voice data found");
    return false;
}
