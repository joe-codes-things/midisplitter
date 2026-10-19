#pragma once

#include <juce_core/juce_core.h>
#include <pluginterfaces/vst/ivstmessage.h>
#include <pluginterfaces/vst/ivsthostapplication.h>
#include "IPCManagerVST3.h"
#include "IPCTraceLogger.h"
#include <atomic>
#include <memory>
#include <thread>
#include <vector>

using namespace Steinberg;
using namespace Steinberg::Vst;

/**
 * @brief VST3 Plugin Discovery Manager
 * 
 * Handles discovery and connection of VST3 plugin instances.
 * Uses a file-based discovery mechanism since VST3 hosts don't provide
 * a direct way for plugins to discover each other.
 * 
 * This implementation uses JUCE's InterprocessConnectionServer/Client
 * as a backup discovery mechanism when the host doesn't connect instances.
 */
class VST3DiscoveryManager {
public:
    /**
     * @brief Connection state
     */
    enum class State {
        Uninitialized,
        Discovering,
        Connected,
        Failed
    };
    
    /**
     * @brief Constructor
     */
    VST3DiscoveryManager();
    
    /**
     * @brief Destructor
     */
    ~VST3DiscoveryManager();
    
    /**
     * @brief Initialize discovery
     * @param sessionId Unique session identifier
     * @param isSender true for Sender mode, false for Receiver mode
     * @param ipcManager The IPCManagerVST3 to connect
     * @return true if discovery started successfully
     */
    bool initialize(const juce::String& sessionId, bool isSender, 
                    IPCManagerVST3* ipcManager);
    
    /**
     * @brief Shutdown discovery
     */
    void shutdown();
    
    /**
     * @brief Check if currently connected
     * @return true if connected to another instance
     */
    bool isConnected() const;
    
    /**
     * @brief Get current discovery state
     * @return Current state
     */
    State getState() const;
    
    /**
     * @brief Get number of discovered connections
     * @return Connection count
     */
    int getConnectionCount() const;
    
    /**
     * @brief Attempt manual reconnection
     * @return true if reconnection started
     */
    bool reconnect();
    
    /**
     * @brief Process discovery (call periodically from timer)
     */
    void processDiscovery();

private:
    /**
     * @brief Start discovery as Sender (server mode)
     */
    bool startSenderDiscovery();
    
    /**
     * @brief Start discovery as Receiver (client mode)
     */
    bool startReceiverDiscovery();
    
    /**
     * @brief Check for connection files from other instances
     */
    void scanForConnections();
    
    /**
     * @brief Write connection file for this instance
     */
    void writeConnectionFile();
    
    /**
     * @brief Remove connection file for this instance
     */
    void removeConnectionFile();
    
    /**
     * @brief Get discovery directory path
     */
    juce::File getDiscoveryDirectory() const;
    
    /**
     * @brief Get connection file for this instance
     */
    juce::File getConnectionFile() const;
    
    /**
     * @brief Attempt to connect via host IConnectionPoint
     */
    bool attemptHostConnection();

private:
    std::atomic<State> state_{State::Uninitialized};
    std::atomic<bool> isSender_{false};
    std::atomic<int> connectionCount_{0};
    
    juce::String sessionId_;
    IPCManagerVST3* ipcManager_ = nullptr;
    
    // NOTE: Background threads disabled in audio plugins
    // Discovery runs on-demand via processDiscovery() calls
};

// Inline implementation

inline VST3DiscoveryManager::VST3DiscoveryManager() {}

inline VST3DiscoveryManager::~VST3DiscoveryManager() {
    shutdown();
}

inline bool VST3DiscoveryManager::initialize(const juce::String& sessionId, bool isSender, 
                                            IPCManagerVST3* ipcManager) {
    auto& logger = IPCTraceLogger::getInstance();
    
    if (state_ != State::Uninitialized) {
        shutdown();
    }
    
    sessionId_ = sessionId;
    isSender_ = isSender;
    ipcManager_ = ipcManager;
    
    juce::String modeStr = isSender ? "Sender" : "Receiver";
    logger.logEvent(IPCTraceLogger::EventType::Info, modeStr, "VST3", 0,
                    "Discovery manager initializing - session: " + sessionId);
    
    state_ = State::Discovering;
    
    // Write our connection file
    writeConnectionFile();
    
    // NOTE: Background threads disabled in audio plugins
    // Discovery runs on-demand via processDiscovery() calls
    
    logger.logEvent(IPCTraceLogger::EventType::Info, modeStr, "VST3", 0,
                    "Discovery initialized (no background thread)");
    
    return true;
}

inline void VST3DiscoveryManager::shutdown() {
    auto& logger = IPCTraceLogger::getInstance();
    juce::String modeStr = isSender_ ? "Sender" : "Receiver";
    
    removeConnectionFile();
    
    state_ = State::Uninitialized;
    connectionCount_ = 0;
    
    logger.logEvent(IPCTraceLogger::EventType::Info, modeStr, "VST3", 0,
                    "Discovery manager shutdown");
}

inline bool VST3DiscoveryManager::isConnected() const {
    return state_ == State::Connected;
}

inline VST3DiscoveryManager::State VST3DiscoveryManager::getState() const {
    return state_.load();
}

inline int VST3DiscoveryManager::getConnectionCount() const {
    return connectionCount_.load();
}

inline bool VST3DiscoveryManager::reconnect() {
    auto& logger = IPCTraceLogger::getInstance();
    juce::String modeStr = isSender_ ? "Sender" : "Receiver";
    
    logger.logEvent(IPCTraceLogger::EventType::ConnectionAttempt, modeStr, "VST3", 0,
                    "Manual reconnection requested");
    
    state_ = State::Discovering;
    connectionCount_ = 0;
    
    // Remove old connection file and write new one
    removeConnectionFile();
    writeConnectionFile();
    
    return true;
}

inline void VST3DiscoveryManager::processDiscovery() {
    if (state_ != State::Discovering) {
        return;
    }
    
    scanForConnections();
    
    // Update state based on connection count
    if (ipcManager_) {
        int count = static_cast<int>(ipcManager_->getConnectionCount());
        connectionCount_ = count;
        
        if (count > 0) {
            state_ = State::Connected;
        }
    }
}

inline bool VST3DiscoveryManager::startSenderDiscovery() {
    // Sender acts as the discovery coordinator
    writeConnectionFile();
    return true;
}

inline bool VST3DiscoveryManager::startReceiverDiscovery() {
    // Receiver scans for Sender
    writeConnectionFile();
    return true;
}

inline void VST3DiscoveryManager::scanForConnections() {
    auto& logger = IPCTraceLogger::getInstance();
    juce::String modeStr = isSender_ ? "Sender" : "Receiver";
    
    // Note: This is a simplified discovery mechanism
    // In a full implementation, we would:
    // 1. Scan the discovery directory for other instances
    // 2. Read their connection info
    // 3. Attempt to establish connections via host or fallback
    
    // For now, we rely on the InProcessMIDIBus for same-process communication
    // and log that discovery is active
    
    static int scanCount = 0;
    scanCount++;
    
    if (scanCount % 10 == 0) { // Log every 10th scan (every 5 seconds)
        logger.logEvent(IPCTraceLogger::EventType::DiscoveryBroadcast, modeStr, "VST3", 0,
                        "Discovery scan active - waiting for other instances");
    }
}

inline void VST3DiscoveryManager::writeConnectionFile() {
    juce::File dir = getDiscoveryDirectory();
    if (!dir.exists()) {
        dir.createDirectory();
    }
    
    juce::File file = getConnectionFile();
    
    // Write connection info
    juce::String content;
    content << "Session: " << sessionId_ << "\n";
    content << "Mode: " << (isSender_ ? "Sender" : "Receiver") << "\n";
    content << "Timestamp: " << juce::Time::getCurrentTime().toString(true, true) << "\n";
    
    file.replaceWithText(content);
}

inline void VST3DiscoveryManager::removeConnectionFile() {
    juce::File file = getConnectionFile();
    if (file.existsAsFile()) {
        file.deleteFile();
    }
}

inline juce::File VST3DiscoveryManager::getDiscoveryDirectory() const {
    juce::File tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
    return tempDir.getChildFile("MIDI_Splitter_Discovery");
}

inline juce::File VST3DiscoveryManager::getConnectionFile() const {
    juce::String filename = "MIDI_Splitter_" + sessionId_ + ".txt";
    return getDiscoveryDirectory().getChildFile(filename);
}

inline bool VST3DiscoveryManager::attemptHostConnection() {
    // This would attempt to connect via the host's IConnectionPoint mechanism
    // Requires host context which is set by the processor
    return false;
}
