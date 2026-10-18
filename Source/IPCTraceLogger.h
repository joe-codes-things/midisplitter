#pragma once

#include <juce_core/juce_core.h>
#include <mutex>
#include <fstream>
#include <queue>
#include <string>
#include <atomic>
#include <chrono>
#include <ctime>

/**
 * @brief File-based IPC trace logging system
 * 
 * Logs IPC events to a file for debugging inter-plugin communication.
 * Includes timestamps, event types, and detailed payload information.
 * Thread-safe for audio thread usage.
 */
class IPCTraceLogger {
public:
    /**
     * @brief Log event types
     */
    enum class EventType {
        ConnectionAttempt,
        ConnectionSuccess,
        ConnectionFailed,
        ConnectionLost,
        MessageSent,
        MessageReceived,
        MessageDropped,
        DiscoveryBroadcast,
        DiscoveryResponse,
        Error,
        Info
    };
    
    /**
     * @brief Get singleton instance
     */
    static IPCTraceLogger& getInstance();
    
    /**
     * @brief Initialize logging to file
     * @param sessionId Unique session identifier for log file naming
     * @return true if logging initialized successfully
     */
    bool initialize(const juce::String& sessionId);
    
    /**
     * @brief Shutdown logging
     */
    void shutdown();
    
    /**
     * @brief Log an event
     * @param type Event type
     * @param source "Sender" or "Receiver"
     * @param format "VST3" or "AU"
     * @param voiceNumber Voice number (1-8, 0 for broadcast)
     * @param message Event description
     */
    void logEvent(EventType type, 
                  const juce::String& source,
                  const juce::String& format,
                  int voiceNumber,
                  const juce::String& message);
    
    /**
     * @brief Log a message payload (for debugging)
     * @param source "Sender" or "Receiver"
     * @param format "VST3" or "AU"
     * @param voiceNumber Voice number
     * @param payloadSize Size of payload in bytes
     * @param eventCount Number of MIDI events in payload
     */
    void logMessagePayload(const juce::String& source,
                           const juce::String& format,
                           int voiceNumber,
                           size_t payloadSize,
                           int eventCount);
    
    /**
     * @brief Log connection status change
     * @param source "Sender" or "Receiver"
     * @param format "VST3" or "AU"
     * @param isConnected New connection state
     * @param connectionCount Number of connections (for Sender)
     */
    void logConnectionStatus(const juce::String& source,
                             const juce::String& format,
                             bool isConnected,
                             int connectionCount);
    
    /**
     * @brief Log IPC error
     * @param source "Sender" or "Receiver"
     * @param format "VST3" or "AU"
     * @param errorMessage Error description
     */
    void logError(const juce::String& source,
                  const juce::String& format,
                  const juce::String& errorMessage);
    
    /**
     * @brief Check if logging is active
     * @return true if logging to file
     */
    bool isLogging() const;
    
    /**
     * @brief Get log file path
     * @return Path to current log file
     */
    juce::String getLogFilePath() const;
    
    /**
     * @brief Set log level (0=errors only, 1=normal, 2=verbose)
     * @param level Log level
     */
    void setLogLevel(int level);
    
    /**
     * @brief Get current log level
     * @return Log level
     */
    int getLogLevel() const;
    
    /**
     * @brief Flush pending log entries to disk
     */
    void flush();

private:
    IPCTraceLogger();
    ~IPCTraceLogger();
    
    // Prevent copying
    IPCTraceLogger(const IPCTraceLogger&) = delete;
    IPCTraceLogger& operator=(const IPCTraceLogger&) = delete;
    
    /**
     * @brief Get timestamp string
     */
    juce::String getTimestamp() const;

    static bool getLocalTime(std::time_t time, std::tm& localTime);
    
    /**
     * @brief Get event type string
     */
    juce::String getEventTypeString(EventType type) const;
    
    /**
     * @brief Write log entry to file
     */
    void writeToFile(const juce::String& entry);
    
    /**
     * @brief Background thread function for async logging
     */
    void runLoggingThread();

private:
    std::atomic<bool> initialized_{false};
    std::atomic<int> logLevel_{1}; // 0=errors, 1=normal, 2=verbose
    
    mutable std::mutex logMutex_;
    std::ofstream logFile_;
    juce::File logFilePath_;
};

// Inline implementation

inline IPCTraceLogger& IPCTraceLogger::getInstance() {
    static IPCTraceLogger instance;
    return instance;
}

inline IPCTraceLogger::IPCTraceLogger() {}

inline IPCTraceLogger::~IPCTraceLogger() {
    shutdown();
}

inline bool IPCTraceLogger::getLocalTime(std::time_t time, std::tm& localTime) {
#if defined(_WIN32)
    return localtime_s(&localTime, &time) == 0;
#else
    return localtime_r(&time, &localTime) != nullptr;
#endif
}

inline bool IPCTraceLogger::initialize(const juce::String& sessionId) {
    std::lock_guard<std::mutex> lock(logMutex_);
    
    if (initialized_) {
        return true;
    }
    
    // Create log file in user's documents folder
    juce::File documentsDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    juce::File logDir = documentsDir.getChildFile("MIDI Splitter Logs");
    
    if (!logDir.exists()) {
        logDir.createDirectory();
    }
    
    // Create log filename with timestamp and session ID
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
    if (!getLocalTime(time, localTime)) {
        juce::Logger::writeToLog("Failed to convert local time while initializing IPC logging.");
        return false;
    }
    std::stringstream filename;
    filename << "MIDI_Splitter_IPC_" << sessionId.toStdString() << "_" 
             << std::put_time(&localTime, "%Y%m%d_%H%M%S") << ".log";
    
    logFilePath_ = logDir.getChildFile(juce::String(filename.str()));
    
    logFile_.open(logFilePath_.getFullPathName().toStdString(), std::ios::out | std::ios::app);
    
    if (!logFile_.is_open()) {
        DBG("Failed to open IPC log file: " << logFilePath_.getFullPathName());
        return false;
    }
    
    initialized_ = true;
    
    // Write header directly (no background thread in plugins)
    juce::String header = getTimestamp() + " [INFO] [System] [All] [Voice 0] IPC Trace Logger initialized - Session: " + sessionId;
    logFile_ << header.toStdString() << std::endl;
    logFile_.flush();
    
    DBG("IPC Trace Logger initialized: " << logFilePath_.getFullPathName());
    return true;
}

inline void IPCTraceLogger::shutdown() {
    std::lock_guard<std::mutex> lock(logMutex_);
    
    if (!initialized_) {
        return;
    }
    
    initialized_ = false;
    
    if (logFile_.is_open()) {
        logFile_.flush();
        logFile_.close();
    }
    
    DBG("IPC Trace Logger shutdown");
}

inline void IPCTraceLogger::logEvent(EventType type, 
                                       const juce::String& source,
                                       const juce::String& format,
                                       int voiceNumber,
                                       const juce::String& message) {
    // Filter by log level
    if (logLevel_ == 0 && type != EventType::Error) {
        return;
    }
    
    juce::String entry = getTimestamp() + " [" + getEventTypeString(type) + "] " +
                         "[" + source + "] [" + format + "] " +
                         "[Voice " + juce::String(voiceNumber) + "] " +
                         message;
    
    writeToFile(entry);
}

inline void IPCTraceLogger::logMessagePayload(const juce::String& source,
                                               const juce::String& format,
                                               int voiceNumber,
                                               size_t payloadSize,
                                               int eventCount) {
    if (logLevel_ < 2) { // Only in verbose mode
        return;
    }
    
    juce::String message = "Payload: " + juce::String(static_cast<int>(payloadSize)) + 
                          " bytes, " + juce::String(eventCount) + " events";
    
    logEvent(EventType::Info, source, format, voiceNumber, message);
}

inline void IPCTraceLogger::logConnectionStatus(const juce::String& source,
                                                 const juce::String& format,
                                                 bool isConnected,
                                                 int connectionCount) {
    juce::String message = isConnected ? 
                          "Connected (" + juce::String(connectionCount) + " connections)" :
                          "Disconnected";
    
    logEvent(isConnected ? EventType::ConnectionSuccess : EventType::ConnectionLost,
             source, format, 0, message);
}

inline void IPCTraceLogger::logError(const juce::String& source,
                                      const juce::String& format,
                                      const juce::String& errorMessage) {
    logEvent(EventType::Error, source, format, 0, "ERROR: " + errorMessage);
}

inline bool IPCTraceLogger::isLogging() const {
    return initialized_.load();
}

inline juce::String IPCTraceLogger::getLogFilePath() const {
    std::lock_guard<std::mutex> lock(logMutex_);
    return logFilePath_.getFullPathName();
}

inline void IPCTraceLogger::setLogLevel(int level) {
    logLevel_ = juce::jlimit(0, 2, level);
}

inline int IPCTraceLogger::getLogLevel() const {
    return logLevel_.load();
}

inline void IPCTraceLogger::flush() {
    std::lock_guard<std::mutex> lock(logMutex_);
    if (logFile_.is_open()) {
        logFile_.flush();
    }
}

inline juce::String IPCTraceLogger::getTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::tm localTime{};
    if (!getLocalTime(time, localTime)) {
        juce::Logger::writeToLog("Failed to convert local time while formatting an IPC log timestamp.");
        return {};
    }

    std::stringstream ss;
    ss << std::put_time(&localTime, "%H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    return juce::String(ss.str());
}

inline juce::String IPCTraceLogger::getEventTypeString(EventType type) const {
    switch (type) {
        case EventType::ConnectionAttempt: return "CONN_ATTEMPT";
        case EventType::ConnectionSuccess: return "CONN_SUCCESS";
        case EventType::ConnectionFailed: return "CONN_FAILED";
        case EventType::ConnectionLost: return "CONN_LOST";
        case EventType::MessageSent: return "MSG_SENT";
        case EventType::MessageReceived: return "MSG_RECV";
        case EventType::MessageDropped: return "MSG_DROP";
        case EventType::DiscoveryBroadcast: return "DISCOVERY";
        case EventType::DiscoveryResponse: return "DISCOVERY_RSP";
        case EventType::Error: return "ERROR";
        case EventType::Info: return "INFO";
        default: return "UNKNOWN";
    }
}

inline void IPCTraceLogger::writeToFile(const juce::String& entry) {
    std::lock_guard<std::mutex> lock(logMutex_);
    
    if (!initialized_ || !logFile_.is_open()) {
        return;
    }
    
    // Write directly to file (no background thread in plugins)
    logFile_ << entry.toStdString() << std::endl;
}

inline void IPCTraceLogger::runLoggingThread() {
    // Disabled - no background threads in audio plugins
}
