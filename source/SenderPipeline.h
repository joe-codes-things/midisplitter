#pragma once

#include "BufferManager.h"
#include "PitchSorter.h"
#include "DistributionAlgorithm.h"
#include "VoiceAssignmentTracker.h"
#include "IPCManagerVST3.h"
#include "IPCManagerVST3_Socket.h"
#include "IPCManagerAU.h"
#include "MIDIProcessor.h"
#include "PluginController.h"
#include "PluginConfiguration.h"
#include "IPCErrorHandler.h"
#include "MIDIParsingErrorHandler.h"
#include "BufferOverflowHandler.h"
#include "LockFreeQueue.h"
#include "InProcessMIDIBus.h"
#include "IPCTraceLogger.h"
#include "VST3DiscoveryManager.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <memory>
#include <atomic>
#include <algorithm>

/**
 * @brief Complete processing pipeline for Sender mode
 * 
 * Integrates all components for the Sender processing flow:
 * MIDI input → buffer → sort → distribute → IPC send
 * Handles Control Change broadcasting and sustained note tracking.
 */
class SenderPipeline {
public:
    /**
     * @brief Processing statistics
     */
    struct ProcessingStats {
        std::atomic<int> totalEventsProcessed{0};
        std::atomic<int> noteOnEvents{0};
        std::atomic<int> noteOffEvents{0};
        std::atomic<int> controlChangeEvents{0};
        std::atomic<int> buffersProcessed{0};
        std::atomic<int> distributionCycles{0};
        std::atomic<int> ipcMessagesSent{0};
        std::atomic<int> processingErrors{0};
        std::atomic<float> averageLatencyMs{0.0f};
        std::atomic<float> peakLatencyMs{0.0f};

        ProcessingStats() = default;

        ProcessingStats(const ProcessingStats& other) {
            *this = other;
        }

        ProcessingStats& operator=(const ProcessingStats& other) {
            totalEventsProcessed = other.totalEventsProcessed.load();
            noteOnEvents = other.noteOnEvents.load();
            noteOffEvents = other.noteOffEvents.load();
            controlChangeEvents = other.controlChangeEvents.load();
            buffersProcessed = other.buffersProcessed.load();
            distributionCycles = other.distributionCycles.load();
            ipcMessagesSent = other.ipcMessagesSent.load();
            processingErrors = other.processingErrors.load();
            averageLatencyMs = other.averageLatencyMs.load();
            peakLatencyMs = other.peakLatencyMs.load();
            return *this;
        }
        
        void reset() {
            totalEventsProcessed = 0;
            noteOnEvents = 0;
            noteOffEvents = 0;
            controlChangeEvents = 0;
            buffersProcessed = 0;
            distributionCycles = 0;
            ipcMessagesSent = 0;
            processingErrors = 0;
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
    SenderPipeline(const PluginConfiguration& config, 
                   PluginController::PluginFormat pluginFormat = PluginController::PluginFormat::VST3);
    
    /**
     * @brief Destructor
     */
    ~SenderPipeline();
    
    /**
     * @brief Initialize the pipeline
     * @param voiceCount Number of voices to distribute to
     * @return true if initialization was successful
     */
    bool initialize(int voiceCount);
    
    /**
     * @brief Process a block of MIDI data
     * @param midiBuffer Input MIDI buffer from host
     * @param numSamples Number of audio samples in this block
     * @param sampleRate Current sample rate
     * @return true if processing was successful
     */
    bool processBlock(const juce::MidiBuffer& midiBuffer, int numSamples, double sampleRate);
    
    /**
     * @brief Process a block of MIDI data and return voice assignments (for VSTi mode)
     * @param midiBuffer Input MIDI buffer from host
     * @param numSamples Number of audio samples in this block
     * @param sampleRate Current sample rate
     * @return Vector of voice assignments for output to MIDI buses
     */
    std::vector<VoiceAssignment> processBlockAndGetAssignments(const juce::MidiBuffer& midiBuffer, int numSamples, double sampleRate);
    
    /**
     * @brief Set voice count for distribution
     * @param count Number of voices (2-8)
     * @return true if voice count was set successfully
     */
    bool setVoiceCount(int count);
    
    /**
     * @brief Get current voice count
     * @return Number of voices
     */
    int getVoiceCount() const;
    
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
     * @brief Check if pipeline is connected to receivers
     * @return true if connected to at least one receiver
     */
    bool isConnected() const;
    
    /**
     * @brief Get number of connected receivers
     * @return Connection count
     */
    int getConnectionCount() const;
    
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
     * @brief Set whether modulation CC (CC1) should be transmitted
     * @param enabled true to transmit CC1
     */
    void setModEnabled(bool enabled);
    
    /**
     * @brief Set whether expression CC (CC11) should be transmitted
     * @param enabled true to transmit CC11
     */
    void setExpEnabled(bool enabled);

    /**
     * @brief Set whether volume CC (CC7) should be transmitted
     * @param enabled true to transmit CC7
     */
    void setVolEnabled(bool enabled);
    
    /**
     * @brief Get current latency in milliseconds
     * @return Processing latency
     */
    float getCurrentLatency() const;
    
    /**
     * @brief Force flush of current buffer
     * @return Number of events flushed
     */
    int flushBuffer();
    
    /**
     * @brief Get buffer usage percentage
     * @return Usage percentage (0.0-1.0)
     */
    float getBufferUsage() const;
    
    /**
     * @brief Get error handler for IPC
     * @return Reference to IPC error handler
     */
    IPCErrorHandler& getIPCErrorHandler();
    
    /**
     * @brief Get error handler for MIDI parsing
     * @return Reference to MIDI parsing error handler
     */
    MIDIParsingErrorHandler& getMIDIErrorHandler();
    
    /**
     * @brief Process discovery (call periodically to maintain connections)
     */
    void processDiscovery();

private:
    /**
     * @brief Process note events (note-on/note-off)
     * @param events MIDI events to process
     * @return true if processing was successful
     */
    bool processNoteEvents(const std::vector<MIDIEvent>& events);
    
    /**
     * @brief Process control change events
     * @param events MIDI events to process
     * @return true if processing was successful
     */
    bool processControlChangeEvents(const std::vector<MIDIEvent>& events);
    
    /**
     * @brief Distribute events to voices
     * @param events Events to distribute
     * @return Voice assignments
     */
    std::vector<VoiceAssignment> distributeEvents(const std::vector<MIDIEvent>& events);
    
    /**
     * @brief Send voice assignments via IPC
     * @param assignments Voice assignments to send
     * @return true if sent successfully
     */
    bool sendVoiceAssignments(const std::vector<VoiceAssignment>& assignments);
    
    /**
     * @brief Broadcast control change to all voices
     * @param controlChangeEvents CC events to broadcast
     * @return true if broadcast was successful
     */
    bool broadcastControlChange(const std::vector<MIDIEvent>& controlChangeEvents);
    
    /**
     * @brief Update processing statistics
     * @param events Events processed
     * @param processingTimeMs Time taken to process
     */
    void updateStatistics(const std::vector<MIDIEvent>& events, float processingTimeMs);
    
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
    std::atomic<int> voiceCount_;
    std::atomic<bool> processingEnabled_;
    std::atomic<bool> modEnabled_;
    std::atomic<bool> expEnabled_;
    std::atomic<bool> volEnabled_;
    
    // Core processing components
    std::unique_ptr<BufferManager> bufferManager_;
    std::unique_ptr<PitchSorter> pitchSorter_;
    std::unique_ptr<DistributionAlgorithm> distributionAlgorithm_;
    std::unique_ptr<VoiceAssignmentTracker> voiceTracker_;
    std::unique_ptr<MIDIProcessor> midiProcessor_;
    
    // IPC communication
    std::unique_ptr<IPCManagerVST3> ipcManagerVST3_;  // Legacy IConnectionPoint
    std::unique_ptr<IPCManagerVST3_Socket> ipcManagerVST3Socket_;  // Socket-based for inter-plugin
    std::unique_ptr<IPCManagerAU> ipcManagerAU_;
    
    // VST3 discovery (for host-less connection)
    std::unique_ptr<VST3DiscoveryManager> vst3Discovery_;
    
    // Error handling
    std::unique_ptr<IPCErrorHandler> ipcErrorHandler_;
    std::unique_ptr<MIDIParsingErrorHandler> midiErrorHandler_;
    std::unique_ptr<BufferOverflowHandler> bufferOverflowHandler_;
    
    // Thread-safe queues for audio thread communication
    std::unique_ptr<LockFreeQueue<MIDIEvent>> inputQueue_;
    std::unique_ptr<LockFreeQueue<VoiceAssignment>> outputQueue_;
    
    // Statistics and monitoring
    ProcessingStats stats_;
    StatsCallback statsCallback_;
    mutable std::mutex statsMutex_;
    
    // Connection tracking (includes InProcessMIDIBus fallback)
    std::atomic<int> successfulSends_{0};
    std::atomic<juce::int64> lastSuccessfulSendTime_{0}; // Time in ms
    static constexpr juce::int64 CONNECTION_TIMEOUT_MS = 5000; // Consider connected if sent within 5 seconds
    
    // Latency tracking
    std::deque<float> latencyHistory_;
    static constexpr size_t MAX_LATENCY_HISTORY = 100;
    
    // Thread safety
    mutable std::mutex configMutex_;
    mutable std::mutex processingMutex_;
    
    // Constants
    static constexpr int DEFAULT_VOICE_COUNT = 4;
    static constexpr float STATS_UPDATE_INTERVAL_MS = 100.0f;
};

// Implementation

inline SenderPipeline::SenderPipeline(const PluginConfiguration& config, 
                                      PluginController::PluginFormat pluginFormat)
    : config_(config)
    , pluginFormat_(pluginFormat)
    , voiceCount_(DEFAULT_VOICE_COUNT)
    , processingEnabled_(true)
    , modEnabled_(true)
    , expEnabled_(true)
    , volEnabled_(true) {
    
    // Initialize core components
    bufferManager_ = std::make_unique<BufferManager>();
    pitchSorter_ = std::make_unique<PitchSorter>();
    distributionAlgorithm_ = std::make_unique<DistributionAlgorithm>();
    voiceTracker_ = std::make_unique<VoiceAssignmentTracker>();
    midiProcessor_ = std::make_unique<MIDIProcessor>();
    
    // Initialize error handlers
    ipcErrorHandler_ = std::make_unique<IPCErrorHandler>();
    midiErrorHandler_ = std::make_unique<MIDIParsingErrorHandler>();
    midiErrorHandler_->setMaxErrorsPerSecond(config.maxErrorsPerSecond);
    bufferOverflowHandler_ = std::make_unique<BufferOverflowHandler>();
    bufferOverflowHandler_->setMaxNotesPerBuffer(static_cast<size_t>(config.maxNotesPerBuffer));
    
    // Initialize queues
    inputQueue_ = std::make_unique<LockFreeQueue<MIDIEvent>>(config.maxNotesPerBuffer);
    outputQueue_ = std::make_unique<LockFreeQueue<VoiceAssignment>>(config.maxVoiceCount);
    
    // Configure components
    bufferManager_->setBufferSize(config.bufferSizeMs);
    bufferManager_->setTimestampTolerance(config.timestampToleranceUs);
    
}

inline SenderPipeline::~SenderPipeline() {
    cleanup();
}

inline bool SenderPipeline::initialize(int voiceCount) {
    std::lock_guard<std::mutex> lock(processingMutex_);
    
    if (!setVoiceCount(voiceCount)) {
        return false;
    }
    
    // Initialize IPC
    if (!initializeIPC()) {
        return false;
    }
    
    return true;
}

inline bool SenderPipeline::processBlock(const juce::MidiBuffer& midiBuffer, int numSamples, double sampleRate) {
    if (!processingEnabled_) {
        return true;
    }
    
    auto& logger = IPCTraceLogger::getInstance();
    
    // Log incoming MIDI
    int midiEventCount = midiBuffer.getNumEvents();
    if (midiEventCount > 0) {
        logger.logEvent(IPCTraceLogger::EventType::Info, "Sender", "VST3", 0,
                        "processBlock: received " + juce::String(midiEventCount) + " MIDI events");
    }
    
    auto startTime = std::chrono::steady_clock::now();
    
    try {
        // Parse MIDI buffer to events
        auto events = midiProcessor_->processBlock(midiBuffer, sampleRate);
        
        if (events.empty()) {
            if (midiEventCount > 0) {
                logger.logEvent(IPCTraceLogger::EventType::Info, "Sender", "VST3", 0,
                                "processBlock: midiProcessor returned 0 events (filtered out?)");
            }
            return true; // No events to process
        }
        
        logger.logEvent(IPCTraceLogger::EventType::Info, "Sender", "VST3", 0,
                        "processBlock: parsed " + juce::String(events.size()) + " events");
        
        // Add events to buffer manager
        for (const auto& event : events) {
            bufferManager_->addEvent(event);
        }
        
        // Check if buffer is ready for processing
        const double blockDurationMs = sampleRate > 0.0
            ? (static_cast<double>(numSamples) / sampleRate) * 1000.0
            : 0.0;

        if (bufferManager_->isBufferReady() || blockDurationMs >= static_cast<double>(config_.bufferSizeMs)) {
            auto bufferedEvents = bufferManager_->flushBuffer();
            stats_.buffersProcessed++;
            
            if (!bufferedEvents.empty()) {
                // Separate note events from control change events
                std::vector<MIDIEvent> noteEvents;
                std::vector<MIDIEvent> controlChangeEvents;
                
                for (const auto& event : bufferedEvents) {
                    if (event.isNoteOn() || event.isNoteOff()) {
                        noteEvents.push_back(event);
                    } else if (event.isControlChange()) {
                        controlChangeEvents.push_back(event);
                    }
                }
                
                // Process note events through distribution pipeline
                if (!noteEvents.empty()) {
                    if (!processNoteEvents(noteEvents)) {
                        handleProcessingError("Failed to process note events");
                        return false;
                    }
                }
                
                // Broadcast control change events to all voices
                if (!controlChangeEvents.empty()) {
                    if (!processControlChangeEvents(controlChangeEvents)) {
                        handleProcessingError("Failed to process control change events");
                        return false;
                    }
                }
                
                // Update statistics
                float processingTime = calculateLatency(startTime);
                updateStatistics(bufferedEvents, processingTime);
            }
        }
        
        return true;
        
    } catch (const std::exception& e) {
        handleProcessingError("Exception in processBlock: " + juce::String(e.what()));
        return false;
    }
}

inline std::vector<VoiceAssignment> SenderPipeline::processBlockAndGetAssignments(const juce::MidiBuffer& midiBuffer, int numSamples, double sampleRate) {
    std::vector<VoiceAssignment> allAssignments;
    
    // Initialize empty assignments for all voices
    for (int voice = 1; voice <= voiceCount_.load(); ++voice) {
        allAssignments.emplace_back(voice);
    }
    
    if (!processingEnabled_)
        return allAssignments;
    
    auto& logger = IPCTraceLogger::getInstance();
    
    // Log incoming MIDI
    int midiEventCount = midiBuffer.getNumEvents();
    if (midiEventCount > 0) {
        logger.logEvent(IPCTraceLogger::EventType::Info, "Sender", "VSTi", 0,
                        "processBlockAndGetAssignments: received " + juce::String(midiEventCount) + " MIDI events");
    }
    
    auto startTime = std::chrono::steady_clock::now();
    
    try {
        // Parse MIDI buffer to events
        auto events = midiProcessor_->processBlock(midiBuffer, sampleRate);
        
        if (events.empty()) {
            if (midiEventCount > 0) {
                logger.logEvent(IPCTraceLogger::EventType::Info, "Sender", "VSTi", 0,
                                "processBlockAndGetAssignments: midiProcessor returned 0 events (filtered out?)");
            }
            return allAssignments; // No events to process
        }
        
        logger.logEvent(IPCTraceLogger::EventType::Info, "Sender", "VSTi", 0,
                        "processBlockAndGetAssignments: parsed " + juce::String(events.size()) + " events");
        
        // Add events to buffer manager
        for (const auto& event : events) {
            bufferManager_->addEvent(event);
        }
        
        // Check if buffer is ready for processing
        const double blockDurationMs = sampleRate > 0.0
            ? (static_cast<double>(numSamples) / sampleRate) * 1000.0
            : 0.0;

        if (bufferManager_->isBufferReady() || blockDurationMs >= static_cast<double>(config_.bufferSizeMs)) {
            auto bufferedEvents = bufferManager_->flushBuffer();
            stats_.buffersProcessed++;
            
            if (!bufferedEvents.empty()) {
                // Separate note events from control change events
                std::vector<MIDIEvent> noteEvents;
                std::vector<MIDIEvent> controlChangeEvents;
                
                for (const auto& event : bufferedEvents) {
                    if (event.isNoteOn() || event.isNoteOff()) {
                        noteEvents.push_back(event);
                    } else if (event.isControlChange()) {
                        controlChangeEvents.push_back(event);
                    }
                }
                
                // Process note events through distribution pipeline
                if (!noteEvents.empty()) {
                    // CRITICAL: Process note-offs BEFORE note-ons to prevent hanging notes
                    // When chords change without gaps, note-offs from the old chord and
                    // note-ons from the new chord arrive in the same buffer.
                    // We must process note-offs first to clear old voice assignments.
                    
                    std::vector<MIDIEvent> noteOnEvents;
                    std::vector<MIDIEvent> noteOffEvents;

                    for (const auto& event : noteEvents) {
                        if (event.isNoteOn()) {
                            noteOnEvents.push_back(event);
                        } else if (event.isNoteOff()) {
                            noteOffEvents.push_back(event);
                        }
                    }

                    // STEP 1: Process note-offs FIRST to clear old voice assignments
                    // Store them temporarily so we can add them after distribution
                    std::vector<std::pair<int, MIDIEvent>> noteOffsToAdd; // voice number, event
                    
                    for (const auto& noteOff : noteOffEvents) {
                        int voiceNumber = voiceTracker_->getVoiceForNoteOff(noteOff.note, noteOff.channel);
                        if (voiceNumber >= 1 && voiceNumber <= voiceCount_.load()) {
                            noteOffsToAdd.push_back({voiceNumber, noteOff});
                        }
                        // Clear from tracker BEFORE distributing new note-ons
                        voiceTracker_->recordNoteOff(noteOff.note, noteOff.channel);
                    }

                    // STEP 2: Now distribute note-ons (after old assignments are cleared from tracker)
                    if (!noteOnEvents.empty()) {
                        auto sortedNoteOns = PitchSorter::sortCopy(noteOnEvents);
                        allAssignments = distributeEvents(sortedNoteOns);

                        // Track the new note-ons
                        for (const auto& assignment : allAssignments) {
                            for (const auto& assignedEvent : assignment.notes) {
                                if (assignedEvent.isNoteOn()) {
                                    voiceTracker_->recordNoteOn(assignedEvent.note,
                                                                assignedEvent.channel,
                                                                static_cast<uint8_t>(assignment.voiceNumber),
                                                                assignedEvent.timestamp);
                                }
                            }
                        }
                    }
                    
                    // STEP 3: Add the note-offs to their assigned voices
                    for (const auto& noteOffPair : noteOffsToAdd) {
                        int voiceNumber = noteOffPair.first;
                        const MIDIEvent& noteOff = noteOffPair.second;
                        allAssignments[static_cast<size_t>(voiceNumber - 1)].notes.push_back(noteOff);
                    }
                }
                
                // Broadcast control change events to all voices — respecting CC filters
                if (!controlChangeEvents.empty()) {
                    // Apply the same CC1/CC7/CC11 filters as broadcastControlChange
                    std::vector<MIDIEvent> filteredCC;
                    bool modEnabled = modEnabled_.load();
                    bool volEnabled = volEnabled_.load();
                    bool expEnabled = expEnabled_.load();
                    for (const auto& ev : controlChangeEvents) {
                        if (ev.isControlChange()) {
                            uint8_t cc = ev.note;
                            if (cc == 1  && !modEnabled) continue;
                            if (cc == 7  && !volEnabled) continue;
                            if (cc == 11 && !expEnabled) continue;
                        }
                        filteredCC.push_back(ev);
                    }
                    if (!filteredCC.empty()) {
                        for (auto& assignment : allAssignments) {
                            assignment.notes.insert(assignment.notes.end(),
                                                    filteredCC.begin(),
                                                    filteredCC.end());
                        }
                        stats_.controlChangeEvents += static_cast<int>(filteredCC.size()) * voiceCount_.load();
                    }
                }
                
                // Update statistics
                float processingTime = calculateLatency(startTime);
                updateStatistics(bufferedEvents, processingTime);
            }
        }
        
        const bool hasVoiceData = std::any_of(
            allAssignments.begin(), allAssignments.end(),
            [](const VoiceAssignment& assignment) { return !assignment.notes.empty(); });
        if (hasVoiceData)
            sendVoiceAssignments(allAssignments);

        return allAssignments;
        
    } catch (const std::exception& e) {
        handleProcessingError("Exception in processBlockAndGetAssignments: " + juce::String(e.what()));
        return allAssignments; // Return empty assignments on error
    }
}

inline bool SenderPipeline::setVoiceCount(int count) {
    if (count < 2 || count > 16) {
        return false;
    }
    
    voiceCount_ = count;
    
    return true;
}

inline int SenderPipeline::getVoiceCount() const {
    return voiceCount_;
}

inline bool SenderPipeline::updateConfiguration(const PluginConfiguration& config) {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    config_ = config;
    
    // Update component configurations
    if (bufferManager_) {
        bufferManager_->setBufferSize(config.bufferSizeMs);
        bufferManager_->setTimestampTolerance(config.timestampToleranceUs);
    }
    
    if (midiErrorHandler_) {
        midiErrorHandler_->setMaxErrorsPerSecond(config.maxErrorsPerSecond);
    }
    
    if (bufferOverflowHandler_) {
        bufferOverflowHandler_->setMaxNotesPerBuffer(static_cast<size_t>(config.maxNotesPerBuffer));
    }
    
    return true;
}

inline const PluginConfiguration& SenderPipeline::getConfiguration() const {
    std::lock_guard<std::mutex> lock(configMutex_);
    return config_;
}

inline bool SenderPipeline::isConnected() const {
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
        else if (vst3Discovery_ && vst3Discovery_->isConnected()) {
            ipcConnected = true;
        }
    } else if (pluginFormat_ == PluginController::PluginFormat::AU && ipcManagerAU_) {
        ipcConnected = ipcManagerAU_->isConnected();
    }
    
    // Also consider "connected" if we've successfully sent messages recently (via ANY method including InProcessMIDIBus)
    juce::int64 currentTime = juce::Time::getCurrentTime().toMilliseconds();
    juce::int64 lastSend = lastSuccessfulSendTime_.load();
    bool recentSendSuccess = (lastSend > 0) && ((currentTime - lastSend) < CONNECTION_TIMEOUT_MS);
    
    return ipcConnected || recentSendSuccess;
}

inline int SenderPipeline::getConnectionCount() const {
    // Count IPC connections
    int ipcCount = 0;
    if (pluginFormat_ == PluginController::PluginFormat::VST3) {
        // Check socket-based connection count first (primary for inter-plugin)
        if (ipcManagerVST3Socket_) {
            ipcCount = ipcManagerVST3Socket_->getConnectionCount();
        }
        // Fallback to legacy methods if no socket connections
        if (ipcCount == 0 && ipcManagerVST3_) {
            int discoveryCount = 0;
            if (vst3Discovery_) {
                discoveryCount = vst3Discovery_->getConnectionCount();
            }
            ipcCount = juce::jmax(discoveryCount, static_cast<int>(ipcManagerVST3_->getConnectionCount()));
        }
    } else if (pluginFormat_ == PluginController::PluginFormat::AU && ipcManagerAU_) {
        ipcCount = ipcManagerAU_->isConnected() ? 1 : 0;
    }
    
    // If we have recent successful sends via InProcessMIDIBus, count that as a connection too
    juce::int64 currentTime = juce::Time::getCurrentTime().toMilliseconds();
    juce::int64 lastSend = lastSuccessfulSendTime_.load();
    bool recentSendSuccess = (lastSend > 0) && ((currentTime - lastSend) < CONNECTION_TIMEOUT_MS);
    
    if (recentSendSuccess && ipcCount == 0) {
        return 1; // InProcessMIDIBus is working
    }
    
    return ipcCount;
}

inline SenderPipeline::ProcessingStats SenderPipeline::getStatistics() const {
    return stats_;
}

inline void SenderPipeline::resetStatistics() {
    stats_.reset();
    
    std::lock_guard<std::mutex> lock(statsMutex_);
    latencyHistory_.clear();
}

inline void SenderPipeline::setStatsCallback(StatsCallback callback) {
    statsCallback_ = callback;
}

inline void SenderPipeline::setProcessingEnabled(bool enabled) {
    processingEnabled_ = enabled;
}

inline void SenderPipeline::setModEnabled(bool enabled) {
    modEnabled_ = enabled;
}

inline void SenderPipeline::setExpEnabled(bool enabled) {
    expEnabled_ = enabled;
}

inline void SenderPipeline::setVolEnabled(bool enabled) {
    volEnabled_ = enabled;
}

inline bool SenderPipeline::isProcessingEnabled() const {
    return processingEnabled_;
}

inline float SenderPipeline::getCurrentLatency() const {
    return stats_.averageLatencyMs;
}

inline int SenderPipeline::flushBuffer() {
    if (!bufferManager_) {
        return 0;
    }
    
    auto events = bufferManager_->flushBuffer();
    return static_cast<int>(events.size());
}

inline float SenderPipeline::getBufferUsage() const {
    if (!bufferManager_) {
        return 0.0f;
    }
    
    // This would need to be implemented in BufferManager
    return 0.0f; // Placeholder
}

inline IPCErrorHandler& SenderPipeline::getIPCErrorHandler() {
    return *ipcErrorHandler_;
}

inline MIDIParsingErrorHandler& SenderPipeline::getMIDIErrorHandler() {
    return *midiErrorHandler_;
}

inline bool SenderPipeline::processNoteEvents(const std::vector<MIDIEvent>& events) {
    std::vector<MIDIEvent> noteOnEvents;
    std::vector<MIDIEvent> noteOffEvents;

    for (const auto& event : events) {
        if (event.isNoteOn()) {
            noteOnEvents.push_back(event);
        } else if (event.isNoteOff()) {
            noteOffEvents.push_back(event);
        }
    }

    std::vector<VoiceAssignment> voiceAssignments;
    for (int voice = 1; voice <= voiceCount_.load(); ++voice) {
        voiceAssignments.emplace_back(voice);
    }

    if (!noteOnEvents.empty()) {
        auto sortedNoteOns = PitchSorter::sortCopy(noteOnEvents);
        voiceAssignments = distributeEvents(sortedNoteOns);

        for (const auto& assignment : voiceAssignments) {
            for (const auto& assignedEvent : assignment.notes) {
                if (assignedEvent.isNoteOn()) {
                    voiceTracker_->recordNoteOn(assignedEvent.note,
                                                assignedEvent.channel,
                                                static_cast<uint8_t>(assignment.voiceNumber),
                                                assignedEvent.timestamp);
                }
            }
        }
    }

    for (const auto& noteOff : noteOffEvents) {
        int voiceNumber = voiceTracker_->getVoiceForNoteOff(noteOff.note, noteOff.channel);
        if (voiceNumber < 1 || voiceNumber > voiceCount_.load()) {
            voiceNumber = 1;
        }

        voiceAssignments[static_cast<size_t>(voiceNumber - 1)].notes.push_back(noteOff);
        voiceTracker_->recordNoteOff(noteOff.note, noteOff.channel);
    }
    
    // Send voice assignments
    return sendVoiceAssignments(voiceAssignments);
}

inline bool SenderPipeline::processControlChangeEvents(const std::vector<MIDIEvent>& events) {
    return broadcastControlChange(events);
}

inline std::vector<VoiceAssignment> SenderPipeline::distributeEvents(const std::vector<MIDIEvent>& events) {
    auto assignments = DistributionAlgorithm::distribute(events, voiceCount_.load());
    stats_.distributionCycles++;
    return assignments;
}

inline bool SenderPipeline::sendVoiceAssignments(const std::vector<VoiceAssignment>& assignments) {
    auto& logger = IPCTraceLogger::getInstance();
    juce::String format = (pluginFormat_ == PluginController::PluginFormat::VST3) ? "VST3" : "AU";
    
    // Count total events for logging
    int totalEvents = 0;
    for (const auto& assignment : assignments) {
        totalEvents += static_cast<int>(assignment.notes.size());
    }
    
    logger.logEvent(IPCTraceLogger::EventType::MessageSent, "Sender", format, 0,
                    "Sending " + juce::String(assignments.size()) + " voice assignments, " +
                    juce::String(totalEvents) + " total events");
    
    std::vector<VoiceAssignment> localAssignments;
    std::vector<VoiceAssignment> remoteAssignments;
    for (const auto& assignment : assignments) {
        if (InProcessMIDIBus::hasSubscriber(pluginFormat_, assignment.voiceNumber)) {
            localAssignments.push_back(assignment);
        } else {
            remoteAssignments.push_back(assignment);
        }
    }

    bool success = !localAssignments.empty() &&
                   InProcessMIDIBus::publish(pluginFormat_, localAssignments);
    if (!success) {
        remoteAssignments.insert(remoteAssignments.end(),
                                 localAssignments.begin(),
                                 localAssignments.end());
    }

    bool remoteSuccess = false;
    if (!remoteAssignments.empty() &&
        pluginFormat_ == PluginController::PluginFormat::VST3 &&
        ipcManagerVST3Socket_) {
        int connectionCount = static_cast<int>(ipcManagerVST3Socket_->getConnectionCount());
        logger.logEvent(IPCTraceLogger::EventType::Info, "Sender", "VST3", 0,
                        "VST3 socket connection count: " + juce::String(connectionCount));
        
        remoteSuccess = ipcManagerVST3Socket_->sendVoiceData(remoteAssignments);
        
        if (remoteSuccess) {
            logger.logEvent(IPCTraceLogger::EventType::MessageSent, "Sender", "VST3", 0,
                            "VST3 socket send successful");
        } else {
            logger.logEvent(IPCTraceLogger::EventType::MessageDropped, "Sender", "VST3", 0,
                            "VST3 socket send failed - falling back");
        }
    }
    // Fallback to legacy VST3 IConnectionPoint (rarely works for inter-plugin)
    else if (!remoteAssignments.empty() &&
             pluginFormat_ == PluginController::PluginFormat::VST3 &&
             ipcManagerVST3_) {
        int connectionCount = static_cast<int>(ipcManagerVST3_->getConnectionCount());
        logger.logEvent(IPCTraceLogger::EventType::Info, "Sender", "VST3", 0,
                        "VST3 legacy connection count: " + juce::String(connectionCount));
        
        remoteSuccess = ipcManagerVST3_->sendVoiceData(remoteAssignments);
        
        if (remoteSuccess) {
            logger.logEvent(IPCTraceLogger::EventType::MessageSent, "Sender", "VST3", 0,
                            "VST3 legacy send successful");
        } else {
            logger.logEvent(IPCTraceLogger::EventType::MessageDropped, "Sender", "VST3", 0,
                            "VST3 legacy send failed - falling back");
        }
    } else if (!remoteAssignments.empty() &&
               pluginFormat_ == PluginController::PluginFormat::AU &&
               ipcManagerAU_) {
        bool isConnected = ipcManagerAU_->isConnected();
        logger.logEvent(IPCTraceLogger::EventType::Info, "Sender", "AU", 0,
                        "AU connected: " + juce::String(isConnected ? "yes" : "no"));
        
        remoteSuccess = ipcManagerAU_->sendVoiceData(remoteAssignments);
        
        if (remoteSuccess) {
            logger.logEvent(IPCTraceLogger::EventType::MessageSent, "Sender", "AU", 0,
                            "AU message sent successfully");
        } else {
            logger.logEvent(IPCTraceLogger::EventType::MessageDropped, "Sender", "AU", 0,
                            "AU send failed - falling back to InProcessMIDIBus");
        }
    }

    success = success || remoteSuccess;
    if (!success) {
        logger.logError("Sender", format, "All IPC methods failed - message dropped");
    }
    
    if (success) {
        stats_.ipcMessagesSent++;
        successfulSends_++;
        lastSuccessfulSendTime_ = juce::Time::getCurrentTime().toMilliseconds();
    }
    
    return success;
}

inline bool SenderPipeline::broadcastControlChange(const std::vector<MIDIEvent>& controlChangeEvents) {
    // Filter CC events based on user preferences
    std::vector<MIDIEvent> filteredEvents;
    bool modEnabled = modEnabled_.load();
    bool expEnabled = expEnabled_.load();
    bool volEnabled = volEnabled_.load();
    
    for (const auto& event : controlChangeEvents) {
        if (event.isControlChange()) {
            uint8_t ccNumber = event.note;  // note field holds CC number for CC events
            
            if (ccNumber == 1  && !modEnabled) continue;  // CC1  = Modulation
            if (ccNumber == 7  && !volEnabled) continue;  // CC7  = Volume
            if (ccNumber == 11 && !expEnabled) continue;  // CC11 = Expression
            
            filteredEvents.push_back(event);
        } else {
            filteredEvents.push_back(event);
        }
    }
    
    if (filteredEvents.empty())
        return true;
    
    std::vector<VoiceAssignment> ccAssignments;
    for (int voice = 1; voice <= voiceCount_; ++voice) {
        VoiceAssignment assignment;
        assignment.voiceNumber = voice;
        assignment.notes = filteredEvents;
        ccAssignments.push_back(assignment);
    }
    
    return sendVoiceAssignments(ccAssignments);
}

inline void SenderPipeline::updateStatistics(const std::vector<MIDIEvent>& events, float processingTimeMs) {
    stats_.totalEventsProcessed += static_cast<int>(events.size());
    
    // Count event types
    for (const auto& event : events) {
        if (event.isNoteOn()) {
            stats_.noteOnEvents++;
        } else if (event.isNoteOff()) {
            stats_.noteOffEvents++;
        } else if (event.isControlChange()) {
            stats_.controlChangeEvents++;
        }
    }
    
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
        stats_.averageLatencyMs =
            totalLatency / static_cast<float>(latencyHistory_.size());
        
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

inline void SenderPipeline::handleProcessingError(const juce::String& errorMessage) {
    stats_.processingErrors++;
    
    // Log error (could be expanded to use a logging system)
    juce::Logger::writeToLog("SenderPipeline Error: " + errorMessage);
}

inline float SenderPipeline::calculateLatency(std::chrono::steady_clock::time_point startTime) const {
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    return static_cast<float>(duration.count()) / 1000.0f; // Convert to milliseconds
}

inline bool SenderPipeline::initializeIPC() {
    auto& logger = IPCTraceLogger::getInstance();
    juce::String format = (pluginFormat_ == PluginController::PluginFormat::VST3) ? "VST3" : "AU";
    
    const juce::String sessionId = IPCManagerVST3_Socket::EXPERIMENT_SESSION_ID;
    logger.initialize(sessionId);
    
    logger.logEvent(IPCTraceLogger::EventType::ConnectionAttempt, "Sender", format, 0,
                    "Initializing IPC for format: " + format);
    
    if (pluginFormat_ == PluginController::PluginFormat::VST3) {
        // Use file-based IPC for inter-plugin communication (works across process boundaries)
        ipcManagerVST3Socket_ = std::make_unique<IPCManagerVST3_Socket>();
        ipcManagerVST3Socket_->setMode(true);
        bool success = ipcManagerVST3Socket_->startServer(sessionId);
        
        // Keep legacy manager for compatibility but don't use for inter-plugin
        ipcManagerVST3_ = std::make_unique<IPCManagerVST3>();
        ipcManagerVST3_->setMode(true);
        
        if (success) {
            logger.logEvent(IPCTraceLogger::EventType::ConnectionSuccess, "Sender", "VST3", 0,
                            "VST3 socket server started - session: " + sessionId);
        } else {
            logger.logError("Sender", "VST3", "Failed to start VST3 socket server");
        }
        return success;
    } else if (pluginFormat_ == PluginController::PluginFormat::AU) {
        ipcManagerAU_ = std::make_unique<IPCManagerAU>();
        ipcManagerAU_->setMode(true);
        bool success = ipcManagerAU_->startServer("default"); // Start as server for AU
        
        if (success) {
            logger.logEvent(IPCTraceLogger::EventType::ConnectionSuccess, "Sender", "AU", 0,
                            "AU server started successfully");
        } else {
            logger.logError("Sender", "AU", "Failed to start AU server");
        }
        return success;
    }
    
    logger.logError("Sender", format, "Unknown plugin format");
    return false;
}

inline void SenderPipeline::cleanup() {
    processingEnabled_ = false;
    
    // Shutdown VST3 discovery manager
    if (vst3Discovery_) {
        vst3Discovery_->shutdown();
        vst3Discovery_.reset();
    }
    
    // Cleanup IPC connections
    if (ipcManagerVST3Socket_) {
        ipcManagerVST3Socket_->shutdown();
        ipcManagerVST3Socket_.reset();
    }
    
    if (ipcManagerVST3_) {
        ipcManagerVST3_->disconnectAll();
    }
    
    if (ipcManagerAU_) {
        ipcManagerAU_->disconnect();
    }
}

inline void SenderPipeline::processDiscovery() {
    // Run discovery process for VST3 to find other instances
    if (vst3Discovery_) {
        vst3Discovery_->processDiscovery();
    }
}
