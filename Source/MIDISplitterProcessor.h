#pragma once

#include "PluginController.h"
#include "SenderPipeline.h"
#include "ReceiverPipeline.h"
#include "PluginConfiguration.h"
#include "ConfigurationValidator.h"
#include "LatencyTracker.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <atomic>

#ifndef JucePlugin_Name
#define JucePlugin_Name "MIDI Splitter"
#endif

// Forward declaration
class MIDISplitterEditor;

/**
 * @brief Main AudioProcessor implementation for MIDI Splitter Plugin
 * 
 * Handles MIDI input/output processing, parameter management, state persistence,
 * and integration with both Sender and Receiver processing pipelines.
 */
class MIDISplitterProcessor : public juce::AudioProcessor {
public:
    /**
     * @brief Constructor
     */
    MIDISplitterProcessor();
    
    /**
     * @brief Destructor
     */
    ~MIDISplitterProcessor() override;
    
    // AudioProcessor interface implementation
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    
    #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    #endif
    
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    
    const juce::String getName() const override;
    
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;
    
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    
    // Custom interface
    
    /**
     * @brief Get plugin controller
     * @return Reference to plugin controller
     */
    PluginController& getPluginController();
    
    /**
     * @brief Get current configuration
     * @return Current plugin configuration
     */
    const PluginConfiguration& getConfiguration() const;
    
    /**
     * @brief Update configuration
     * @param config New configuration
     * @return true if configuration was updated successfully
     */
    bool updateConfiguration(const PluginConfiguration& config);
    
    /**
     * @brief Get sender pipeline (if in Sender mode)
     * @return Pointer to sender pipeline, nullptr if in Receiver mode
     */
    SenderPipeline* getSenderPipeline() const;
    
    /**
     * @brief Get receiver pipeline (if in Receiver mode)
     * @return Pointer to receiver pipeline, nullptr if in Sender mode
     */
    ReceiverPipeline* getReceiverPipeline() const;
    
    /**
     * @brief Get latency tracker
     * @return Reference to latency tracker
     */
    LatencyTracker& getLatencyTracker();
    
    /**
     * @brief Get current latency in samples
     * @return Latency in samples
     */
    int getLatencySamples() const;
    
    /**
     * @brief Get processing statistics
     * @return Processing statistics string
     */
    juce::String getProcessingStatistics() const;
    
    /**
     * @brief Reset processing statistics
     */
    void resetStatistics();
    
    /**
     * @brief Enable/disable processing
     * @param enabled Whether processing should be enabled
     */
    void setProcessingEnabled(bool enabled);
    
    /**
     * @brief Check if processing is enabled
     * @return true if processing is enabled
     */
    bool isProcessingEnabled() const;
    
    /**
     * @brief Create bus properties for the plugin
     * @return BusesProperties with audio and MIDI buses configured
     */
    static BusesProperties createBusesProperties();

private:
    /**
     * @brief Initialize processing pipeline based on current mode
     * @return true if initialization was successful
     */
    bool initializePipeline();
    
    /**
     * @brief Cleanup current pipeline
     */
    void cleanupPipeline();
    
    /**
     * @brief Handle mode change
     * @param newMode New plugin mode
     */
    void handleModeChange(PluginController::Mode newMode);
    
    /**
     * @brief Handle voice count change (Sender mode)
     * @param newCount New voice count
     */
    void handleVoiceCountChange(int newCount);
    
    /**
     * @brief Handle voice number change (Receiver mode)
     * @param newNumber New voice number
     */
    void handleVoiceNumberChange(int newNumber);
    
    /**
     * @brief Handle buffer size change (Sender mode)
     * @param newSizeMs New buffer size in milliseconds
     */
    void handleBufferSizeChange(int newSizeMs);
    
    /**
     * @brief Update latency reporting
     */
    void updateLatencyReporting();
    
    /**
     * @brief Validate current configuration
     * @return true if configuration is valid
     */
    bool validateConfiguration();
    
    /**
     * @brief Get plugin format from host
     * @return Detected plugin format
     */
    PluginController::PluginFormat detectPluginFormat() const;

private:
    // Core components
    std::unique_ptr<PluginController> pluginController_;
    std::unique_ptr<PluginConfiguration> configuration_;
    std::unique_ptr<ConfigurationValidator> configValidator_;
    std::unique_ptr<LatencyTracker> latencyTracker_;
    
    // Processing pipelines
    std::unique_ptr<SenderPipeline> senderPipeline_;
    std::unique_ptr<ReceiverPipeline> receiverPipeline_;
    
    // Automatable parameters
    juce::AudioParameterBool* bypassParam_;
    juce::AudioParameterBool* modFilterParam_;
    juce::AudioParameterBool* expFilterParam_;
    juce::AudioParameterBool* volFilterParam_;
    juce::AudioParameterInt* voiceCountParam_;
    
    // Audio processing state
    std::atomic<double> currentSampleRate_;
    std::atomic<int> currentBlockSize_;
    std::atomic<bool> processingEnabled_;
    std::atomic<bool> isPrepared_;
    
    // Latency tracking
    std::atomic<int> currentLatencySamples_;
    
    // Periodic discovery (to maintain connections during idle periods)
    std::atomic<int> discoveryCallCounter_{0};
    static constexpr int DISCOVERY_PERIOD_BLOCKS = 100; // Call discovery every ~100 blocks (~2-3 seconds at 44.1kHz)
    
    // Thread safety
    mutable std::mutex processingMutex_;
    mutable std::mutex configurationMutex_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MIDISplitterProcessor)
};

// Implementation

inline MIDISplitterProcessor::BusesProperties MIDISplitterProcessor::createBusesProperties() {
    // Create stereo audio input/output (required for VST Instrument)
    // VSTi outputs MIDI on different channels (1-8) for each voice
    return BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true);
}

inline MIDISplitterProcessor::MIDISplitterProcessor()
    : AudioProcessor(createBusesProperties())
    , currentSampleRate_(44100.0)
    , currentBlockSize_(512)
    , processingEnabled_(true)
    , isPrepared_(false)
    , currentLatencySamples_(0) {
    
    // Add automatable parameters in order: Bypass, Voice Count, Mod, Exp, Vol
    
    // 1. Bypass (Magenta-ish color in Logic)
    addParameter(bypassParam_ = new juce::AudioParameterBool(
        "bypass",                           // parameter ID
        "Bypass",                           // parameter name
        false,                              // default value (not bypassed)
        juce::String(),                     // parameter label
        [](bool value, int) { return value ? "Enabled" : "Disabled"; },  // value to text
        [](const juce::String& text) { return text == "Enabled"; }       // text to value
    ));
    
    // 2. Voice Count (Gold color - was Vol's color)
    addParameter(voiceCountParam_ = new juce::AudioParameterInt(
        "voiceCount",                       // parameter ID
        "Voice Count",                      // parameter name
        2,                                  // minimum value
        16,                                 // maximum value
        4                                   // default value
    ));
    
    // 3. Mod (Deep Amber - was Mod's color, stays the same)
    addParameter(modFilterParam_ = new juce::AudioParameterBool(
        "modFilter",                        // parameter ID
        "Mod",                              // parameter name (removed "Filter")
        true,                               // default value (enabled)
        juce::String(),                     // parameter label
        [](bool value, int) { return value ? "Enabled" : "Disabled"; },
        [](const juce::String& text) { return text == "Enabled"; }
    ));
    
    // 4. Exp (Bright Blue - was Exp's color, swapped with Mod)
    addParameter(expFilterParam_ = new juce::AudioParameterBool(
        "expFilter",                        // parameter ID
        "Exp",                              // parameter name (removed "Filter")
        true,                               // default value (enabled)
        juce::String(),                     // parameter label
        [](bool value, int) { return value ? "Enabled" : "Disabled"; },
        [](const juce::String& text) { return text == "Enabled"; }
    ));
    
    // 5. Vol (Logic default yellow)
    addParameter(volFilterParam_ = new juce::AudioParameterBool(
        "volFilter",                        // parameter ID
        "Vol",                              // parameter name (removed "Filter")
        true,                               // default value (enabled)
        juce::String(),                     // parameter label
        [](bool value, int) { return value ? "Enabled" : "Disabled"; },
        [](const juce::String& text) { return text == "Enabled"; }
    ));
    
    // Initialize core components
    pluginController_ = std::make_unique<PluginController>();
    configuration_ = std::make_unique<PluginConfiguration>();
    configValidator_ = std::make_unique<ConfigurationValidator>();
    latencyTracker_ = std::make_unique<LatencyTracker>();
    
    // Initialize with default configuration
    *configuration_ = PluginConfiguration::getDefault();
    pluginController_->setStateChangeCallback([this]() {
        initializePipeline();
        updateLatencyReporting();
    });
    
    // Initialize pipeline
    initializePipeline();
}

inline MIDISplitterProcessor::~MIDISplitterProcessor() {
    cleanupPipeline();
}

inline void MIDISplitterProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    std::lock_guard<std::mutex> lock(processingMutex_);
    
    currentSampleRate_ = sampleRate;
    currentBlockSize_ = samplesPerBlock;
    
    // Update latency reporting
    updateLatencyReporting();
    
    isPrepared_ = true;
}

inline void MIDISplitterProcessor::releaseResources() {
    std::lock_guard<std::mutex> lock(processingMutex_);
    isPrepared_ = false;
}

#ifndef JucePlugin_PreferredChannelConfigurations
inline bool MIDISplitterProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    // This is a MIDI effect, so we don't need specific audio channel configurations
    // Accept any layout
    return true;
}
#endif

inline void MIDISplitterProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    
    // Check bypass parameter first - if bypassed, pass MIDI through unchanged
    if (*bypassParam_) {
        // Bypass is enabled - pass through MIDI unchanged
        return;
    }
    
    // Update plugin controller from automation parameters
    bool modEnabled = *modFilterParam_;
    bool expEnabled = *expFilterParam_;
    bool volEnabled = *volFilterParam_;
    int voiceCount = *voiceCountParam_;
    
    // Sync to plugin controller if changed
    if (pluginController_->isModEnabled() != modEnabled) {
        pluginController_->setModEnabled(modEnabled);
        if (senderPipeline_) senderPipeline_->setModEnabled(modEnabled);
    }
    if (pluginController_->isExpEnabled() != expEnabled) {
        pluginController_->setExpEnabled(expEnabled);
        if (senderPipeline_) senderPipeline_->setExpEnabled(expEnabled);
    }
    if (pluginController_->isVolEnabled() != volEnabled) {
        pluginController_->setVolEnabled(volEnabled);
        if (senderPipeline_) senderPipeline_->setVolEnabled(volEnabled);
    }
    if (pluginController_->getVoiceCount() != voiceCount) {
        pluginController_->setVoiceCount(voiceCount);
        if (senderPipeline_) senderPipeline_->setVoiceCount(voiceCount);
    }
    
    if (!processingEnabled_ || !isPrepared_) {
        // Clear MIDI output if processing is disabled
        midiMessages.clear();
        return;
    }
    
    // Start total latency measurement
    int totalLatencyId = latencyTracker_->startMeasurement(LatencyTracker::Stage::Total);
    
    std::lock_guard<std::mutex> lock(processingMutex_);
    
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    // Clear audio channels (pass through audio, but we're primarily a MIDI processor)
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        buffer.clear(i, 0, buffer.getNumSamples());
    }
    
    // Process MIDI based on current mode
    auto currentMode = pluginController_->getMode();
    
    if (currentMode == PluginController::Mode::Sender && senderPipeline_) {
        // VSTi mode: process input MIDI and distribute to MIDI channels 1-16
        // Get voice assignments from sender pipeline
        auto voiceAssignments = senderPipeline_->processBlockAndGetAssignments(midiMessages, buffer.getNumSamples(), currentSampleRate_);
        
        // Clear main MIDI input buffer
        midiMessages.clear();
        
        // Output each voice's MIDI on its corresponding MIDI channel (1-16)
        // Voice 1 = MIDI channel 1, Voice 2 = MIDI channel 2, etc.
        for (const auto& assignment : voiceAssignments) {
            int voiceNumber = assignment.voiceNumber;
            if (voiceNumber < 1 || voiceNumber > 16) {
                continue; // Skip invalid voice numbers
            }
            
            // MIDI channel for this voice (1-based, but JUCE uses 1-16 internally)
            int midiChannel = voiceNumber;
            
            // Add all MIDI events for this voice to the output buffer on the voice's channel
            for (const auto& event : assignment.notes) {
                juce::MidiMessage msg;
                
                if (event.isNoteOn()) {
                    msg = juce::MidiMessage::noteOn(midiChannel, event.note, static_cast<uint8_t>(event.velocity));
                } else if (event.isNoteOff()) {
                    msg = juce::MidiMessage::noteOff(midiChannel, event.note, static_cast<uint8_t>(event.velocity));
                } else if (event.isControlChange()) {
                    // For CC events: note field = CC number, velocity field = CC value
                    msg = juce::MidiMessage::controllerEvent(midiChannel, event.note, event.velocity);
                } else {
                    continue; // Skip unsupported event types
                }
                
                // Add message at the appropriate sample offset
                int sampleOffset = static_cast<int>(event.timestamp % buffer.getNumSamples());
                midiMessages.addEvent(msg, sampleOffset);
            }
        }
        
    } else if (currentMode == PluginController::Mode::Receiver && receiverPipeline_) {
        // Receiver mode: NOT RECOMMENDED in VSTi architecture
        // In VSTi mode, other tracks should read from our MIDI output directly
        // However, we keep this for backward compatibility
        midiMessages.clear();
        receiverPipeline_->processBlock(midiMessages, buffer.getNumSamples(), currentSampleRate_);
        
        // midiMessages now contains the output from the receiver pipeline
    } else {
        // No valid pipeline, clear MIDI output
        midiMessages.clear();
    }
    
    // End total latency measurement
    latencyTracker_->endMeasurement(totalLatencyId);
}

#include "MIDISplitterEditor.h"

inline juce::AudioProcessorEditor* MIDISplitterProcessor::createEditor() {
    return new MIDISplitterEditor(*this);
}

inline bool MIDISplitterProcessor::hasEditor() const {
    return true;
}

inline const juce::String MIDISplitterProcessor::getName() const {
    return JucePlugin_Name;
}

inline bool MIDISplitterProcessor::acceptsMidi() const {
    return true;
}

inline bool MIDISplitterProcessor::producesMidi() const {
    return true;
}

inline bool MIDISplitterProcessor::isMidiEffect() const {
    return true;
}

inline double MIDISplitterProcessor::getTailLengthSeconds() const {
    return 0.0; // No tail for MIDI effects
}

inline int MIDISplitterProcessor::getNumPrograms() {
    return 1; // No programs
}

inline int MIDISplitterProcessor::getCurrentProgram() {
    return 0;
}

inline void MIDISplitterProcessor::setCurrentProgram(int index) {
    // No programs to set
}

inline const juce::String MIDISplitterProcessor::getProgramName(int index) {
    return "Default";
}

inline void MIDISplitterProcessor::changeProgramName(int index, const juce::String& newName) {
    // No programs to rename
}

inline void MIDISplitterProcessor::getStateInformation(juce::MemoryBlock& destData) {
    std::lock_guard<std::mutex> lock(configurationMutex_);
    
    // Sync parameter values to PluginController before saving
    pluginController_->setModEnabled(*modFilterParam_);
    pluginController_->setExpEnabled(*expFilterParam_);
    pluginController_->setVolEnabled(*volFilterParam_);
    pluginController_->setVoiceCount(*voiceCountParam_);
    
    pluginController_->saveState(destData);
}

inline void MIDISplitterProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::lock_guard<std::mutex> lock(configurationMutex_);
    
    if (pluginController_->restoreState(data, sizeInBytes)) {
        // Sync restored values to automation parameters
        *modFilterParam_ = pluginController_->isModEnabled();
        *expFilterParam_ = pluginController_->isExpEnabled();
        *volFilterParam_ = pluginController_->isVolEnabled();
        *voiceCountParam_ = pluginController_->getVoiceCount();
        
        initializePipeline();
    }
}

inline PluginController& MIDISplitterProcessor::getPluginController() {
    return *pluginController_;
}

inline const PluginConfiguration& MIDISplitterProcessor::getConfiguration() const {
    std::lock_guard<std::mutex> lock(configurationMutex_);
    return *configuration_;
}

inline bool MIDISplitterProcessor::updateConfiguration(const PluginConfiguration& config) {
    std::lock_guard<std::mutex> lock(configurationMutex_);
    
    // Validate configuration
    auto validationResult = ConfigurationValidator::validateAll(config.defaultVoiceCount, 1, config.bufferSizeMs);
    if (!validationResult.isValid) {
        return false;
    }
    
    *configuration_ = config;
    
    // Update pipelines
    if (senderPipeline_) {
        senderPipeline_->updateConfiguration(*configuration_);
    }
    
    if (receiverPipeline_) {
        receiverPipeline_->updateConfiguration(*configuration_);
    }
    
    return true;
}

inline SenderPipeline* MIDISplitterProcessor::getSenderPipeline() const {
    return senderPipeline_.get();
}

inline ReceiverPipeline* MIDISplitterProcessor::getReceiverPipeline() const {
    return receiverPipeline_.get();
}

inline LatencyTracker& MIDISplitterProcessor::getLatencyTracker() {
    return *latencyTracker_;
}

inline int MIDISplitterProcessor::getLatencySamples() const {
    return currentLatencySamples_;
}

inline juce::String MIDISplitterProcessor::getProcessingStatistics() const {
    juce::String stats;
    
    auto currentMode = pluginController_->getMode();
    
    if (currentMode == PluginController::Mode::Sender && senderPipeline_) {
        auto senderStats = senderPipeline_->getStatistics();
        stats << "Sender Mode Statistics:\n";
        stats << "  Events Processed: " << senderStats.totalEventsProcessed.load() << "\n";
        stats << "  Note On Events: " << senderStats.noteOnEvents.load() << "\n";
        stats << "  Note Off Events: " << senderStats.noteOffEvents.load() << "\n";
        stats << "  Control Change Events: " << senderStats.controlChangeEvents.load() << "\n";
        stats << "  Buffers Processed: " << senderStats.buffersProcessed.load() << "\n";
        stats << "  IPC Messages Sent: " << senderStats.ipcMessagesSent.load() << "\n";
        stats << "  Average Latency: " << senderStats.averageLatencyMs.load() << " ms\n";
        stats << "  Peak Latency: " << senderStats.peakLatencyMs.load() << " ms\n";
        stats << "  Processing Errors: " << senderStats.processingErrors.load() << "\n";
        
    } else if (currentMode == PluginController::Mode::Receiver && receiverPipeline_) {
        auto receiverStats = receiverPipeline_->getStatistics();
        stats << "Receiver Mode Statistics:\n";
        stats << "  Events Received: " << receiverStats.totalEventsReceived.load() << "\n";
        stats << "  Events Filtered: " << receiverStats.eventsFiltered.load() << "\n";
        stats << "  Events Output: " << receiverStats.eventsOutput.load() << "\n";
        stats << "  IPC Messages Received: " << receiverStats.ipcMessagesReceived.load() << "\n";
        stats << "  Reconstruction Errors: " << receiverStats.reconstructionErrors.load() << "\n";
        stats << "  Timing Errors: " << receiverStats.timingErrors.load() << "\n";
        stats << "  Average Latency: " << receiverStats.averageLatencyMs.load() << " ms\n";
        stats << "  Peak Latency: " << receiverStats.peakLatencyMs.load() << " ms\n";
    } else {
        stats = "No active pipeline";
    }
    
    return stats;
}

inline void MIDISplitterProcessor::resetStatistics() {
    if (senderPipeline_) {
        senderPipeline_->resetStatistics();
    }
    
    if (receiverPipeline_) {
        receiverPipeline_->resetStatistics();
    }
}

inline void MIDISplitterProcessor::setProcessingEnabled(bool enabled) {
    processingEnabled_ = enabled;
    
    if (senderPipeline_) {
        senderPipeline_->setProcessingEnabled(enabled);
    }
    
    if (receiverPipeline_) {
        receiverPipeline_->setProcessingEnabled(enabled);
    }
}

inline bool MIDISplitterProcessor::isProcessingEnabled() const {
    return processingEnabled_;
}

inline bool MIDISplitterProcessor::initializePipeline() {
    cleanupPipeline();
    
    auto currentMode = pluginController_->getMode();
    auto pluginFormat = detectPluginFormat();
    
    if (currentMode == PluginController::Mode::Sender) {
        senderPipeline_ = std::make_unique<SenderPipeline>(*configuration_, pluginFormat);
        
        // Sync CC filter state
        senderPipeline_->setModEnabled(pluginController_->isModEnabled());
        senderPipeline_->setExpEnabled(pluginController_->isExpEnabled());
        senderPipeline_->setVolEnabled(pluginController_->isVolEnabled());
        
        return senderPipeline_->initialize(pluginController_->getVoiceCount());
        
    } else if (currentMode == PluginController::Mode::Receiver) {
        receiverPipeline_ = std::make_unique<ReceiverPipeline>(*configuration_, pluginFormat);
        return receiverPipeline_->initialize(pluginController_->getVoiceNumber());
    }
    
    return false;
}

inline void MIDISplitterProcessor::cleanupPipeline() {
    senderPipeline_.reset();
    receiverPipeline_.reset();
}

inline void MIDISplitterProcessor::handleModeChange(PluginController::Mode newMode) {
    std::lock_guard<std::mutex> lock(processingMutex_);
    
    // Reinitialize pipeline for new mode
    initializePipeline();
    updateLatencyReporting();
}

inline void MIDISplitterProcessor::handleVoiceCountChange(int newCount) {
    if (senderPipeline_) {
        senderPipeline_->setVoiceCount(newCount);
    }
}

inline void MIDISplitterProcessor::handleVoiceNumberChange(int newNumber) {
    if (receiverPipeline_) {
        receiverPipeline_->setVoiceNumber(newNumber);
    }
}

inline void MIDISplitterProcessor::handleBufferSizeChange(int newSizeMs) {
    std::lock_guard<std::mutex> lock(configurationMutex_);
    
    configuration_->bufferSizeMs = newSizeMs;
    
    if (senderPipeline_) {
        senderPipeline_->updateConfiguration(*configuration_);
    }
    
    updateLatencyReporting();
}

inline void MIDISplitterProcessor::updateLatencyReporting() {
    // Get measured latency from tracker
    float measuredLatencyMs = latencyTracker_->getTotalLatency();
    
    // If no measurements yet, use estimated latency based on buffer size
    if (measuredLatencyMs <= 0.0f) {
        measuredLatencyMs = static_cast<float>(configuration_->bufferSizeMs) + 1.5f; // Add processing overhead
    }
    
    // Convert to samples
    int latencySamples = static_cast<int>(measuredLatencyMs * currentSampleRate_ / 1000.0);
    currentLatencySamples_ = latencySamples;
    
    // Update latency tracker targets based on current configuration
    latencyTracker_->setTarget(LatencyTracker::Stage::BufferWindow, 
                              static_cast<float>(configuration_->bufferSizeMs));
    
    // Notify host of latency change
    updateHostDisplay();
}

inline bool MIDISplitterProcessor::validateConfiguration() {
    std::lock_guard<std::mutex> lock(configurationMutex_);
    
    auto validationResult = ConfigurationValidator::validateAll(configuration_->defaultVoiceCount, 1, configuration_->bufferSizeMs);
    return validationResult.isValid;
}

inline PluginController::PluginFormat MIDISplitterProcessor::detectPluginFormat() const {
    auto controllerFormat = pluginController_->getPluginFormat();
    if (controllerFormat != PluginController::PluginFormat::Unknown) {
        return controllerFormat;
    }

    return PluginController::PluginFormat::VST3;
}
