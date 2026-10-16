#pragma once

#include "IPCManagerVST3.h"
#include "IPCManagerVST3_Socket.h"
#include "IPCManagerAU.h"
#include "MIDIProcessor.h"
#include "PluginController.h"
#include "PluginConfiguration.h"
#include "IPCErrorHandler.h"
#include "LockFreeQueue.h"
#include "VoiceAssignment.h"
#include "ActiveNote.h"
#include "InProcessMIDIBus.h"
#include "IPCTraceLogger.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <memory>
#include <atomic>
#include <map>

/**
 * @brief Complete processing pipeline for Receiver mode
 * 
 * Integrates all components for the Receiver processing flow:
 * IPC receive → filter → reconstruct → MIDI output
 * Handles voice filtering and note-on/off pair reconstruction.
 */
class ReceiverPipeline {
public:
    /**
     * @brief Processing statistics
     */
    struct ProcessingStats {
        std::atomic<int> totalEventsReceived{0};
        std::atomic<int> eventsFiltered{0};
        std::atomic<int> eventsOutput{0};
        std::atomic<int> noteOnEvents{0};
        std::atomic<int> noteOffEvents{0};
        std::atomic<int> controlChangeEvents{0};
        std::atomic<int> ipcMessagesReceived{0};
        std::atomic<int> reconstructionErrors{0};
        std::atomic<int> timingErrors{0};
        std::atomic<float> averageLatencyMs{0.0f};
        std::atomic<float> peakLatencyMs{0.0f};

        ProcessingStats() = default;

        ProcessingStats(const ProcessingStats& other) {
            *this = other;
        }

        ProcessingStats& operator=(const ProcessingStats& other) {
            totalEventsReceived = other.totalEventsReceived.load();
            eventsFiltered = other.eventsFiltered.load();
            eventsOutput = other.eventsOutput.load();
            noteOnEvents = other.noteOnEvents.load();
            noteOffEvents = other.noteOffEvents.load();
            controlChangeEvents = other.controlChangeEvents.load();
            ipcMessagesReceived = other.ipcMessagesReceived.load();
            reconstructionErrors = other.reconstructionErrors.load();
            timingErrors = other.timingErrors.load();
            averageLatencyMs = other.averageLatencyMs.load();
            peakLatencyMs = other.peakLatencyMs.load();
            return *this;
        }
        
        void reset() {
            totalEventsReceived = 0;
            eventsFiltered = 0;
            eventsOutput = 0;
            noteOnEvents = 0;
            noteOffEvents = 0;
            controlChangeEvents = 0;
            ipcMessagesReceived = 0;
            reconstructionErrors = 0;
            timingErrors = 0;
            averageLatencyMs = 0.0f;
            peakLatencyMs = 0.0f;
        }
    };
    
    /**
     * @brief Processing callback function type
     * @param stats Current processing statistics
     */
    using StatsCallback = std::function<void(const ProcessingStats& stats)>;
    
    /**
     * @brief Constructor
     * @param config Plugin configuration
     * @param pluginFormat Plugin format (VST3 or AU)
     */
    ReceiverPipeline(const PluginConfiguration& config, 
                     PluginController::PluginFormat pluginFormat = PluginController::PluginFormat::VST3);
    
    /**
     * @brief Destructor
     */
    ~ReceiverPipeline();
    
    /**
     * @brief Initialize the pipeline
     * @param voiceNumber Voice number to receive (1-8)
     * @return true if initialization was successful
     */
    bool initialize(int voiceNumber);
    
    /**
     * @brief Process a block and generate MIDI output
     * @param outputBuffer Output MIDI buffer to fill
     * @param numSamples Number of audio samples in this block
     * @param sampleRate Current sample rate
     * @return true if processing was successful
     */
    bool processBlock(juce::MidiBuffer& outputBuffer, int numSamples, double sampleRate);
    
    /**
     * @brief Set voice number to receive
     * @param voiceNumber Voice number (1-8)
     * @return true if voice number was set successfully
     */
    bool setVoiceNumber(int voiceNumber);
    
    /**
     * @brief Get current voice number
     * @return Voice number
     */
    int getVoiceNumber() const;
    
    /**
     * @brief Update configuration
     * @param config New configuration
     * @return true if configuration was updated successfully
     */
    bool updateConfiguration(const PluginConfiguration& config);
    
    /**
     * @brief Get current configuration
     * @return Current configuration
     */
    const PluginConfiguration& getConfiguration() const;
    
    /**
     * @brief Check if pipeline is connected to sender
     * @return true if connected
     */
    bool isConnected() const;
    
    /**
     * @brief Connect to sender
     * @param hostName Sender hostname (for AU)
     * @param portNumber Sender port (for AU)
     * @return true if connection was successful
     */
    bool connectToSender(const juce::String& hostName = "localhost", int portNumber = 0);
    
    /**
     * @brief Disconnect from sender
     */
    void disconnect();
    
    /**
     * @brief Get processing statistics
     * @return Current statistics
     */
    ProcessingStats getStatistics() const;
    
    /**
     * @brief Reset processing statistics
     */
    void resetStatistics();
    
    /**
     * @brief Set statistics callback
     * @param callback Function to call with updated statistics
     */
    void setStatsCallback(StatsCallback callback);
    
    /**
     * @brief Enable/disable processing
     * @param enabled Whether processing is enabled
     */
    void setProcessingEnabled(bool enabled);
    
    /**
     * @brief Check if processing is enabled
     * @return true if processing is enabled
     */
    bool isProcessingEnabled() const;
    
    /**
     * @brief Get current latency in milliseconds
     * @return Processing latency
     */
    float getCurrentLatency() const;
    
    /**
     * @brief Get number of active notes
     * @return Active note count
     */
    int getActiveNoteCount() const;
    
    /**
     * @brief Get error handler for IPC
     * @return Reference to IPC error handler
     */
    IPCErrorHandler& getIPCErrorHandler();
    
    /**
     * @brief Process discovery (call periodically to maintain connections)
     */
    void processDiscovery();

private:
    /**
     * @brief Handle received voice data from IPC
     * @param voiceAssignments Voice assignments received
     */
    void handleReceivedVoiceData(const std::vector<VoiceAssignment>& voiceAssignments);
    
    /**
     * @brief Filter events for assigned voice
     * @param events All received events
     * @return Events for this receiver's voice
     */
    std::vector<MIDIEvent> filterEventsForVoice(const std::vector<MIDIEvent>& events);
    
    /**
     * @brief Reconstruct note-on/off pairs
     * @param events Filtered events
     * @return Events with proper note-on/off pairing
     */
    std::vector<MIDIEvent> reconstructNotePairs(const std::vector<MIDIEvent>& events);
    
    /**
     * @brief Preserve timing synchronization
     * @param events Events to process
     * @return Events with preserved timing
     */
    std::vector<MIDIEvent> preserveTiming(const std::vector<MIDIEvent>& events);
    
    /**
     * @brief Update active note tracking
     * @param event MIDI event to process
     */
    void updateActiveNotes(const MIDIEvent& event);
    
    /**
     * @brief Clean up stale active notes
     * @param currentTime Current timestamp
     */
    void cleanupStaleNotes(int64_t currentTime);
    
    /**
     * @brief Update processing statistics
     * @param eventsReceived Events received
     * @param eventsOutput Events output
     * @param processingTimeMs Time taken to process
     */
    void updateStatistics(int eventsReceived, int eventsOutput, float processingTimeMs);
    
    /**
     * @brief Handle processing error
     * @param errorMessage Error description
     */
    void handleProcessingError(const juce::String& errorMessage);
    
    /**
     * @brief Calculate processing latency
     * @param startTime Processing start time
     * @return Latency in milliseconds
     */
    float calculateLatency(std::chrono::steady_clock::time_point startTime) const;
    
    /**
     * @brief Initialize IPC manager based on plugin format
     * @return true if initialization was successful
     */
    bool initializeIPC();
    
    /**
     * @brief Cleanup resources
     */
    void cleanup();

private:
    // Configuration
    PluginConfiguration config_;
    PluginController::PluginFormat pluginFormat_;
    std::atomic<int> voiceNumber_;
    std::atomic<bool> processingEnabled_;
    
    // Core processing components
    std::unique_ptr<MIDIProcessor> midiProcessor_;
    
    // IPC communication
    std::unique_ptr<IPCManagerVST3> ipcManagerVST3_;  // Legacy IConnectionPoint
    std::unique_ptr<IPCManagerVST3_Socket> ipcManagerVST3Socket_;  // Socket-based for inter-plugin
    std::unique_ptr<IPCManagerAU> ipcManagerAU_;
    
    // Error handling
    std::unique_ptr<IPCErrorHandler> ipcErrorHandler_;
    
    // Thread-safe queues for audio thread communication
    std::unique_ptr<LockFreeQueue<MIDIEvent>> inputQueue_;
    std::unique_ptr<LockFreeQueue<MIDIEvent>> outputQueue_;
    size_t busSubscriptionId_ = 0;
    
    // Active note tracking
    mutable std::mutex activeNotesMutex_;
    std::map<std::pair<uint8_t, uint8_t>, ActiveNote> activeNotes_; // (note, channel) -> ActiveNote
    
    // Timing synchronization
    mutable std::mutex timingMutex_;
    int64_t lastEventTimestamp_;
    std::deque<std::pair<int64_t, MIDIEvent>> pendingEvents_; // (timestamp, event)
    
    // Statistics and monitoring
    ProcessingStats stats_;
    StatsCallback statsCallback_;
    mutable std::mutex statsMutex_;
    
    // Connection tracking (includes InProcessMIDIBus fallback)
    std::atomic<juce::int64> lastSuccessfulReceiveTime_{0}; // Time in ms
    static constexpr juce::int64 CONNECTION_TIMEOUT_MS = 5000; // Consider connected if received within 5 seconds
    
    // Latency tracking
    std::deque<float> latencyHistory_;
    static constexpr size_t MAX_LATENCY_HISTORY = 100;
    
    // Thread safety
    mutable std::mutex configMutex_;
    mutable std::mutex processingMutex_;
    
    // Constants
    static constexpr int DEFAULT_VOICE_NUMBER = 1;
    static constexpr int64_t STALE_NOTE_TIMEOUT_US = 10000000; // 10 seconds
    static constexpr int64_t TIMING_TOLERANCE_US = 1000; // 1ms
};

// Implementation

inline ReceiverPipeline::ReceiverPipeline(const PluginConfiguration& config, 
                                          PluginController::PluginFormat pluginFormat)
    : config_(config)
    , pluginFormat_(pluginFormat)
    , voiceNumber_(DEFAULT_VOICE_NUMBER)
    , processingEnabled_(true)
    , lastEventTimestamp_(0) {
    
    // Initialize core components
    midiProcessor_ = std::make_unique<MIDIProcessor>();
    
    // Initialize error handlers
    ipcErrorHandler_ = std::make_unique<IPCErrorHandler>();
    
    // Initialize queues
    inputQueue_ = std::make_unique<LockFreeQueue<MIDIEvent>>(config.maxNotesPerBuffer);
    outputQueue_ = std::make_unique<LockFreeQueue<MIDIEvent>>(config.maxNotesPerBuffer);
}

inline ReceiverPipeline::~ReceiverPipeline() {
    cleanup();
}

inline bool ReceiverPipeline::initialize(int voiceNumber) {
    std::lock_guard<std::mutex> lock(processingMutex_);
    
    if (!setVoiceNumber(voiceNumber)) {
        return false;
    }
    
    // Initialize IPC
    if (!initializeIPC()) {
        return false;
    }
    
    return true;
}

inline bool ReceiverPipeline::processBlock(juce::MidiBuffer& outputBuffer, int numSamples, double sampleRate) {
    if (!processingEnabled_) {
        outputBuffer.clear();
        return true;
    }
    
    auto startTime = std::chrono::steady_clock::now();
    
    try {
        outputBuffer.clear();
        
        // Process events from input queue
        std::vector<MIDIEvent> eventsToProcess;
        MIDIEvent event;
        
        // Drain input queue
        while (inputQueue_->pop(event)) {
            eventsToProcess.push_back(event);
        }
        
        if (eventsToProcess.empty()) {
            return true; // No events to process
        }
        
        stats_.totalEventsReceived += static_cast<int>(eventsToProcess.size());
        
        // Filter events for this voice
        auto filteredEvents = filterEventsForVoice(eventsToProcess);
        stats_.eventsFiltered += static_cast<int>(filteredEvents.size());
        
        if (filteredEvents.empty()) {
            return true; // No events for this voice
        }
        
        // Reconstruct note pairs
        auto reconstructedEvents = reconstructNotePairs(filteredEvents);
        
        // Preserve timing synchronization
        auto timedEvents = preserveTiming(reconstructedEvents);
        
        // Convert to JUCE MIDI buffer
        outputBuffer = midiProcessor_->formatOutput(timedEvents, sampleRate);
        
        // Update active note tracking
        for (const auto& evt : timedEvents) {
            updateActiveNotes(evt);
        }
        
        // Clean up stale notes
        int64_t currentTime = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        cleanupStaleNotes(currentTime);
        
        // Update statistics
        float processingTime = calculateLatency(startTime);
        updateStatistics(static_cast<int>(eventsToProcess.size()), 
                        static_cast<int>(timedEvents.size()), 
                        processingTime);
        
        stats_.eventsOutput += static_cast<int>(timedEvents.size());
        
        return true;
        
    } catch (const std::exception& e) {
        handleProcessingError("Exception in processBlock: " + juce::String(e.what()));
        outputBuffer.clear();
        return false;
    }
}

inline bool ReceiverPipeline::setVoiceNumber(int voiceNumber) {
    if (voiceNumber < 1 || voiceNumber > 8) {
        return false;
    }
    
    voiceNumber_ = voiceNumber;
    return true;
}

inline int ReceiverPipeline::getVoiceNumber() const {
    return voiceNumber_;
}

inline bool ReceiverPipeline::updateConfiguration(const PluginConfiguration& config) {
    std::lock_guard<std::mutex> lock(configMutex_);
    config_ = config;
    return true;
}

inline const PluginConfiguration& ReceiverPipeline::getConfiguration() const {
    std::lock_guard<std::mutex> lock(configMutex_);
    return config_;
}

inline bool ReceiverPipeline::isConnected() const {
    // Check if any IPC method reports connection
    bool ipcConnected = false;
    if (pluginFormat_ == PluginController::PluginFormat::VST3) {
        // Check socket-based connection first (primary for inter-plugin)
        if (ipcManagerVST3Socket_ && ipcManagerVST3Socket_->isConnected()) {
            ipcConnected = true;
        }
        // Fallback to legacy methods
        else if (ipcManagerVST3_ && ipcManagerVST3_->isConnected()) {
            ipcConnected = true;
        }
    } else if (pluginFormat_ == PluginController::PluginFormat::AU && ipcManagerAU_) {
        ipcConnected = ipcManagerAU_->isConnected();
    }
    
    // Also consider "connected" if we've successfully received messages recently (via ANY method including InProcessMIDIBus)
    juce::int64 currentTime = juce::Time::getCurrentTime().toMilliseconds();
    juce::int64 lastReceive = lastSuccessfulReceiveTime_.load();
    bool recentReceiveSuccess = (lastReceive > 0) && ((currentTime - lastReceive) < CONNECTION_TIMEOUT_MS);
    
    return ipcConnected || recentReceiveSuccess;
}

inline bool ReceiverPipeline::connectToSender(const juce::String& hostName, int portNumber) {
    if (pluginFormat_ == PluginController::PluginFormat::AU && ipcManagerAU_) {
        juce::ignoreUnused(portNumber);
        return ipcManagerAU_->connectToServer(hostName);
    }
    // VST3 connections are managed by the host
    return true;
}

inline void ReceiverPipeline::disconnect() {
    if (ipcManagerVST3Socket_) {
        ipcManagerVST3Socket_->shutdown();
    }
    
    if (ipcManagerVST3_) {
        ipcManagerVST3_->disconnectAll();
    }
    
    if (ipcManagerAU_) {
        ipcManagerAU_->disconnect();
    }
}

inline ReceiverPipeline::ProcessingStats ReceiverPipeline::getStatistics() const {
    return stats_;
}

inline void ReceiverPipeline::resetStatistics() {
    stats_.reset();
    
    std::lock_guard<std::mutex> lock(statsMutex_);
    latencyHistory_.clear();
}

inline void ReceiverPipeline::setStatsCallback(StatsCallback callback) {
    statsCallback_ = callback;
}

inline void ReceiverPipeline::setProcessingEnabled(bool enabled) {
    processingEnabled_ = enabled;
}

inline bool ReceiverPipeline::isProcessingEnabled() const {
    return processingEnabled_;
}

inline float ReceiverPipeline::getCurrentLatency() const {
    return stats_.averageLatencyMs;
}

inline int ReceiverPipeline::getActiveNoteCount() const {
    std::lock_guard<std::mutex> lock(activeNotesMutex_);
    return static_cast<int>(activeNotes_.size());
}

inline IPCErrorHandler& ReceiverPipeline::getIPCErrorHandler() {
    return *ipcErrorHandler_;
}

inline void ReceiverPipeline::handleReceivedVoiceData(const std::vector<VoiceAssignment>& voiceAssignments) {
    stats_.ipcMessagesReceived++;
    
    // Track successful receive time (for InProcessMIDIBus connection status)
    lastSuccessfulReceiveTime_ = juce::Time::getCurrentTime().toMilliseconds();
    
    // Add all events to input queue for processing
    for (const auto& assignment : voiceAssignments) {
        for (const auto& event : assignment.notes) {
            if (!inputQueue_->push(event)) {
                // Queue is full, handle overflow
                handleProcessingError("Input queue overflow");
                break;
            }
        }
    }
}

inline std::vector<MIDIEvent> ReceiverPipeline::filterEventsForVoice(const std::vector<MIDIEvent>& events) {
    std::vector<MIDIEvent> filteredEvents;
    int targetVoice = voiceNumber_;
    
    for (const auto& event : events) {
        // For now, we assume all events are for this voice
        // In a real implementation, voice information would be embedded in the event
        // or we would receive voice assignments that specify which events belong to which voice
        filteredEvents.push_back(event);
    }
    
    return filteredEvents;
}

inline std::vector<MIDIEvent> ReceiverPipeline::reconstructNotePairs(const std::vector<MIDIEvent>& events) {
    std::vector<MIDIEvent> reconstructedEvents;
    
    for (const auto& event : events) {
        if (event.isNoteOn() || event.isNoteOff()) {
            // Check for proper note-on/off pairing
            auto noteKey = std::make_pair(event.note, event.channel);
            
            std::lock_guard<std::mutex> lock(activeNotesMutex_);
            
            if (event.isNoteOn()) {
                // Record note-on
                ActiveNote activeNote;
                activeNote.note = event.note;
                activeNote.channel = event.channel;
                activeNote.voiceNumber = voiceNumber_;
                activeNote.noteOnTimestamp = event.timestamp;
                
                activeNotes_[noteKey] = activeNote;
                reconstructedEvents.push_back(event);
                stats_.noteOnEvents++;
                
            } else if (event.isNoteOff()) {
                // Check if we have a matching note-on
                auto it = activeNotes_.find(noteKey);
                if (it != activeNotes_.end()) {
                    // Valid note-off for active note
                    activeNotes_.erase(it);
                    reconstructedEvents.push_back(event);
                    stats_.noteOffEvents++;
                } else {
                    // Orphaned note-off, still output it but log error
                    reconstructedEvents.push_back(event);
                    stats_.reconstructionErrors++;
                }
            }
        } else {
            // Non-note events (CC, etc.) pass through
            reconstructedEvents.push_back(event);
            if (event.isControlChange()) {
                stats_.controlChangeEvents++;
            }
        }
    }
    
    return reconstructedEvents;
}

inline std::vector<MIDIEvent> ReceiverPipeline::preserveTiming(const std::vector<MIDIEvent>& events) {
    std::lock_guard<std::mutex> lock(timingMutex_);
    
    std::vector<MIDIEvent> timedEvents;
    
    for (const auto& event : events) {
        // Check timing consistency
        if (event.timestamp < lastEventTimestamp_ - TIMING_TOLERANCE_US) {
            // Event is significantly out of order
            stats_.timingErrors++;
            
            // Adjust timestamp to maintain order
            MIDIEvent adjustedEvent = event;
            adjustedEvent.timestamp = lastEventTimestamp_ + 1;
            timedEvents.push_back(adjustedEvent);
        } else {
            timedEvents.push_back(event);
            lastEventTimestamp_ = event.timestamp;
        }
    }
    
    return timedEvents;
}

inline void ReceiverPipeline::updateActiveNotes(const MIDIEvent& event) {
    // This is handled in reconstructNotePairs
}

inline void ReceiverPipeline::cleanupStaleNotes(int64_t currentTime) {
    std::lock_guard<std::mutex> lock(activeNotesMutex_);
    
    auto it = activeNotes_.begin();
    while (it != activeNotes_.end()) {
        if (currentTime - it->second.noteOnTimestamp > STALE_NOTE_TIMEOUT_US) {
            it = activeNotes_.erase(it);
        } else {
            ++it;
        }
    }
}

inline void ReceiverPipeline::updateStatistics(int eventsReceived, int eventsOutput, float processingTimeMs) {
    // Update latency statistics
    {
        std::lock_guard<std::mutex> lock(statsMutex_);
        latencyHistory_.push_back(processingTimeMs);
        if (latencyHistory_.size() > MAX_LATENCY_HISTORY) {
            latencyHistory_.pop_front();
        }
        
        // Calculate average latency
        float totalLatency = 0.0f;
        for (float latency : latencyHistory_) {
            totalLatency += latency;
        }
        stats_.averageLatencyMs = totalLatency / latencyHistory_.size();
        
        // Update peak latency
        float currentPeak = stats_.peakLatencyMs;
        if (processingTimeMs > currentPeak) {
            stats_.peakLatencyMs = processingTimeMs;
        }
    }
    
    // Call statistics callback
    if (statsCallback_) {
        statsCallback_(stats_);
    }
}

inline void ReceiverPipeline::handleProcessingError(const juce::String& errorMessage) {
    // Log error (could be expanded to use a logging system)
    DBG("ReceiverPipeline Error: " << errorMessage);
}

inline float ReceiverPipeline::calculateLatency(std::chrono::steady_clock::time_point startTime) const {
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    return static_cast<float>(duration.count()) / 1000.0f; // Convert to milliseconds
}

inline bool ReceiverPipeline::initializeIPC() {
    auto& logger = IPCTraceLogger::getInstance();
    juce::String format = (pluginFormat_ == PluginController::PluginFormat::VST3) ? "VST3" : "AU";
    int voiceNum = voiceNumber_.load();
    
    logger.logEvent(IPCTraceLogger::EventType::ConnectionAttempt, "Receiver", format, voiceNum,
                    "Initializing IPC for voice " + juce::String(voiceNum));
    
    if (pluginFormat_ == PluginController::PluginFormat::VST3) {
        // Use socket-based IPC for inter-plugin communication (works across process boundaries)
        ipcManagerVST3Socket_ = std::make_unique<IPCManagerVST3_Socket>();
        ipcManagerVST3Socket_->setMode(false);
        ipcManagerVST3Socket_->setVoiceNumber(static_cast<uint8_t>(voiceNum));
        
        // Set callback for receiving voice data
        ipcManagerVST3Socket_->setReceiveCallback([this, &logger](const std::vector<MIDIEvent>& events, uint8_t voiceNumber) {
            logger.logEvent(IPCTraceLogger::EventType::MessageReceived, "Receiver", "VST3", voiceNumber,
                            "Received " + juce::String(static_cast<int>(events.size())) + " events via VST3 socket");
            
            VoiceAssignment assignment(static_cast<int>(voiceNumber));
            assignment.notes = events;
            handleReceivedVoiceData({ assignment });
        });
        
        // Try to connect to socket server (will be created by Sender)
        // We need a common session ID - use a default or discover it
        // For now, try to connect to a default session
        bool connected = ipcManagerVST3Socket_->connectToServer("default");
        
        // Keep legacy manager for compatibility
        ipcManagerVST3_ = std::make_unique<IPCManagerVST3>();
        ipcManagerVST3_->setMode(false);
        ipcManagerVST3_->setVoiceNumber(static_cast<uint8_t>(voiceNum));
        
        ipcManagerVST3_->setReceiveCallback([this, &logger](const std::vector<MIDIEvent>& events, uint8_t voiceNumber) {
            logger.logEvent(IPCTraceLogger::EventType::MessageReceived, "Receiver", "VST3", voiceNumber,
                            "Received " + juce::String(static_cast<int>(events.size())) + " events via VST3 legacy IPC");
            
            VoiceAssignment assignment(static_cast<int>(voiceNumber));
            assignment.notes = events;
            handleReceivedVoiceData({ assignment });
        });

        busSubscriptionId_ = InProcessMIDIBus::subscribe(pluginFormat_, static_cast<uint8_t>(voiceNum),
            [this, &logger](const std::vector<MIDIEvent>& events, uint8_t voiceNumber) {
                logger.logEvent(IPCTraceLogger::EventType::MessageReceived, "Receiver", "InProcess", voiceNumber,
                                "Received " + juce::String(static_cast<int>(events.size())) + " events via InProcessBus");
                
                VoiceAssignment assignment(static_cast<int>(voiceNumber));
                assignment.notes = events;
                handleReceivedVoiceData({ assignment });
            });
        
        if (connected) {
            logger.logEvent(IPCTraceLogger::EventType::ConnectionSuccess, "Receiver", "VST3", voiceNum,
                            "VST3 socket connected to server");
        } else {
            logger.logEvent(IPCTraceLogger::EventType::Info, "Receiver", "VST3", voiceNum,
                            "VST3 socket not connected yet - will retry in processDiscovery");
        }

        return true;
    } else if (pluginFormat_ == PluginController::PluginFormat::AU) {
        ipcManagerAU_ = std::make_unique<IPCManagerAU>();
        ipcManagerAU_->setMode(false);
        ipcManagerAU_->setVoiceNumber(static_cast<uint8_t>(voiceNum));
        
        // Set callback for receiving voice data
        ipcManagerAU_->setReceiveCallback([this, &logger](const std::vector<MIDIEvent>& events, uint8_t voiceNumber) {
            logger.logEvent(IPCTraceLogger::EventType::MessageReceived, "Receiver", "AU", voiceNumber,
                            "Received " + juce::String(static_cast<int>(events.size())) + " events via AU IPC");
            
            VoiceAssignment assignment(static_cast<int>(voiceNumber));
            assignment.notes = events;
            handleReceivedVoiceData({ assignment });
        });

        busSubscriptionId_ = InProcessMIDIBus::subscribe(pluginFormat_, static_cast<uint8_t>(voiceNum),
            [this, &logger](const std::vector<MIDIEvent>& events, uint8_t voiceNumber) {
                logger.logEvent(IPCTraceLogger::EventType::MessageReceived, "Receiver", "InProcess", voiceNumber,
                                "Received " + juce::String(static_cast<int>(events.size())) + " events via InProcessBus");
                
                VoiceAssignment assignment(static_cast<int>(voiceNumber));
                assignment.notes = events;
                handleReceivedVoiceData({ assignment });
            });
        
        logger.logEvent(IPCTraceLogger::EventType::Info, "Receiver", "AU", voiceNum,
                        "AU IPC initialized - subscribed to InProcessMIDIBus");

        return true;
    }
    
    logger.logError("Receiver", format, "Unknown plugin format during IPC initialization");
    return false;
}

inline void ReceiverPipeline::cleanup() {
    processingEnabled_ = false;

    if (busSubscriptionId_ != 0) {
        InProcessMIDIBus::unsubscribe(busSubscriptionId_);
        busSubscriptionId_ = 0;
    }
    
    // Cleanup socket-based VST3 IPC
    if (ipcManagerVST3Socket_) {
        ipcManagerVST3Socket_->shutdown();
        ipcManagerVST3Socket_.reset();
    }
    
    // Cleanup IPC connections
    disconnect();
    
    // Clear active notes
    {
        std::lock_guard<std::mutex> lock(activeNotesMutex_);
        activeNotes_.clear();
    }
    
    // Clear pending events
    {
        std::lock_guard<std::mutex> lock(timingMutex_);
        pendingEvents_.clear();
    }
}

inline void ReceiverPipeline::processDiscovery() {
    // For VST3: check connection and poll for data
    if (pluginFormat_ == PluginController::PluginFormat::VST3 && ipcManagerVST3Socket_) {
        if (!ipcManagerVST3Socket_->isConnected()) {
            // Try to reconnect to server
            ipcManagerVST3Socket_->connectToServer("default");
        } else {
            // Poll for incoming file-based data
            ipcManagerVST3Socket_->pollForData();
        }
    }
    
    // For AU: check connection health and attempt reconnection if needed
    if (pluginFormat_ == PluginController::PluginFormat::AU && ipcManagerAU_) {
        if (!ipcManagerAU_->isConnected()) {
            // Try to reconnect to server if disconnected
            ipcManagerAU_->connectToServer("localhost");
        }
    }
}
