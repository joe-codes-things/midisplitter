#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <limits>
#include <memory>
#include <atomic>
#include "PluginState.h"

/**
 * PluginController class for managing plugin configuration and state.
 * 
 * Provides centralized control over plugin mode (Sender/Receiver), voice settings,
 * and state persistence. Handles validation of configuration parameters and
 * detection of plugin format (VST3 vs AU).
 * 
 * Requirements: 1.1, 1.2, 1.3, 1.4, 12.1, 12.2, 13.1
 */
class PluginController
{
public:
    /**
     * Plugin operating modes
     */
    enum class Mode
    {
        Sender,    // Distributes MIDI across voices
        Receiver   // Receives MIDI for specific voice
    };
    
    /**
     * Plugin format detection
     */
    enum class PluginFormat
    {
        Unknown,
        VST3,
        AudioUnit,
        AU = AudioUnit
    };
    
    /**
     * Constructor
     */
    PluginController();
    
    /**
     * Destructor
     */
    ~PluginController();
    
    /**
     * Set plugin operating mode
     * @param mode Sender or Receiver mode
     */
    void setMode(Mode mode);
    
    /**
     * Get current plugin operating mode
     * @return Current mode
     */
    Mode getMode() const;
    
    /**
     * Set voice count for Sender mode
     * @param count Number of voices (2-16)
     * @return true if value was valid and set
     */
    bool setVoiceCount(int count);
    
    /**
     * Get voice count for Sender mode
     * @return Number of voices (2-16)
     */
    int getVoiceCount() const;
    
    /**
     * Set voice number for Receiver mode
     * @param number Voice number to receive (1-16)
     * @return true if value was valid and set
     */
    bool setVoiceNumber(int number);
    
    /**
     * Get voice number for Receiver mode
     * @return Voice number (1-16)
     */
    int getVoiceNumber() const;
    
    /**
     * Set buffer size in milliseconds
     * @param sizeMs Buffer size (5-10ms)
     * @return true if value was valid and set
     */
    bool setBufferSize(int sizeMs);
    
    /**
     * Get buffer size in milliseconds
     * @return Buffer size (5-10ms)
     */
    int getBufferSize() const;
    
    /**
     * Set whether modulation CC (CC1) should be transmitted
     * @param enabled true to transmit CC1
     */
    void setModEnabled(bool enabled);
    
    /**
     * Get whether modulation CC (CC1) is enabled
     * @return true if CC1 is transmitted
     */
    bool isModEnabled() const;
    
    /**
     * Set whether expression CC (CC11) should be transmitted
     * @param enabled true to transmit CC11
     */
    void setExpEnabled(bool enabled);
    
    /**
     * Get whether expression CC (CC11) is enabled
     * @return true if CC11 is transmitted
     */
    bool isExpEnabled() const;

    /**
     * Set whether volume CC (CC7) should be transmitted
     * @param enabled true to transmit CC7
     */
    void setVolEnabled(bool enabled);

    /**
     * Get whether volume CC (CC7) is enabled
     * @return true if CC7 is transmitted
     */
    bool isVolEnabled() const;
    
    /**
     * Save current state to memory block
     * @param destData Output memory block for state data
     */
    void saveState(juce::MemoryBlock& destData);
    
    /**
     * Restore state from memory block
     * @param data Input memory block containing state data
     * @return true if state was restored successfully
     */
    bool restoreState(const void* data, int sizeInBytes);
    
    /**
     * Get current plugin format
     * @return Detected plugin format
     */
    PluginFormat getPluginFormat() const;
    
    /**
     * Set plugin format (for manual override)
     * @param format Plugin format to set
     */
    void setPluginFormat(PluginFormat format);
    
    /**
     * Get current plugin state structure
     * @return Current state
     */
    PluginState getCurrentState() const;
    
    /**
     * Set plugin state from structure
     * @param state State to apply
     * @return true if state was valid and applied
     */
    bool setCurrentState(const PluginState& state);
    
    /**
     * Check if current configuration is valid
     * @return true if configuration is valid
     */
    bool isConfigurationValid() const;
    
    /**
     * Get configuration validation errors
     * @return Vector of error messages
     */
    std::vector<juce::String> getValidationErrors() const;
    
    /**
     * Reset to default configuration
     */
    void resetToDefaults();
    
    /**
     * Set state change callback
     * @param callback Function to call when state changes
     */
    void setStateChangeCallback(std::function<void()> callback);
    
    /**
     * Get mode as string
     * @return "Sender" or "Receiver"
     */
    juce::String getModeString() const;
    
    /**
     * Set mode from string
     * @param modeString "Sender" or "Receiver"
     * @return true if valid mode string
     */
    bool setModeFromString(const juce::String& modeString);
    
    /**
     * Get plugin format as string
     * @return "VST3", "AU", or "Unknown"
     */
    juce::String getPluginFormatString() const;
    
    /**
     * Check if plugin is in Sender mode
     * @return true if Sender mode
     */
    bool isSender() const;
    
    /**
     * Check if plugin is in Receiver mode
     * @return true if Receiver mode
     */
    bool isReceiver() const;
    
private:
    // Current state
    std::atomic<Mode> currentMode_;
    std::atomic<int> voiceCount_;
    std::atomic<int> voiceNumber_;
    std::atomic<int> bufferSizeMs_;
    std::atomic<PluginFormat> pluginFormat_;
    std::atomic<bool> modEnabled_;
    std::atomic<bool> expEnabled_;
    std::atomic<bool> volEnabled_;
    
    // State change notification
    std::function<void()> stateChangeCallback_;
    mutable juce::CriticalSection callbackLock_;
    
    // Default values
    static constexpr int DEFAULT_VOICE_COUNT = 4;
    static constexpr int DEFAULT_VOICE_NUMBER = 1;
    static constexpr int DEFAULT_BUFFER_SIZE = 7;  // 7ms
    
    // Validation ranges
    static constexpr int MIN_VOICE_COUNT = 2;
    static constexpr int MAX_VOICE_COUNT = 16;
    static constexpr int MIN_VOICE_NUMBER = 1;
    static constexpr int MAX_VOICE_NUMBER = 16;
    static constexpr int MIN_BUFFER_SIZE = 5;
    static constexpr int MAX_BUFFER_SIZE = 10;
    
    /**
     * Validate voice count
     * @param count Voice count to validate
     * @return true if valid
     */
    bool isValidVoiceCount(int count) const;
    
    /**
     * Validate voice number
     * @param number Voice number to validate
     * @return true if valid
     */
    bool isValidVoiceNumber(int number) const;
    
    /**
     * Validate buffer size
     * @param sizeMs Buffer size to validate
     * @return true if valid
     */
    bool isValidBufferSize(int sizeMs) const;
    
    /**
     * Notify state change
     */
    void notifyStateChange();
    
    /**
     * Auto-detect plugin format
     * @return Detected format
     */
    PluginFormat detectPluginFormat() const;
};

// Implementation

inline PluginController::PluginController()
    : currentMode_(Mode::Sender)
    , voiceCount_(DEFAULT_VOICE_COUNT)
    , voiceNumber_(DEFAULT_VOICE_NUMBER)
    , bufferSizeMs_(DEFAULT_BUFFER_SIZE)
    , pluginFormat_(PluginFormat::Unknown)
    , modEnabled_(true)
    , expEnabled_(true)
    , volEnabled_(true)
{
    // Auto-detect plugin format
    pluginFormat_ = detectPluginFormat();
}

inline PluginController::~PluginController() = default;

inline void PluginController::setMode(Mode mode)
{
    if (currentMode_ != mode)
    {
        currentMode_ = mode;
        notifyStateChange();
    }
}

inline PluginController::Mode PluginController::getMode() const
{
    return currentMode_;
}

inline bool PluginController::setVoiceCount(int count)
{
    if (isValidVoiceCount(count))
    {
        if (voiceCount_ != count)
        {
            voiceCount_ = count;
            notifyStateChange();
        }
        return true;
    }
    return false;
}

inline int PluginController::getVoiceCount() const
{
    return voiceCount_;
}

inline bool PluginController::setVoiceNumber(int number)
{
    if (isValidVoiceNumber(number))
    {
        if (voiceNumber_ != number)
        {
            voiceNumber_ = number;
            notifyStateChange();
        }
        return true;
    }
    return false;
}

inline int PluginController::getVoiceNumber() const
{
    return voiceNumber_;
}

inline bool PluginController::setBufferSize(int sizeMs)
{
    if (isValidBufferSize(sizeMs))
    {
        if (bufferSizeMs_ != sizeMs)
        {
            bufferSizeMs_ = sizeMs;
            notifyStateChange();
        }
        return true;
    }
    return false;
}

inline int PluginController::getBufferSize() const
{
    return bufferSizeMs_;
}

inline void PluginController::setModEnabled(bool enabled)
{
    if (modEnabled_ != enabled)
    {
        modEnabled_ = enabled;
        notifyStateChange();
    }
}

inline bool PluginController::isModEnabled() const
{
    return modEnabled_;
}

inline void PluginController::setExpEnabled(bool enabled)
{
    if (expEnabled_ != enabled)
    {
        expEnabled_ = enabled;
        notifyStateChange();
    }
}

inline bool PluginController::isExpEnabled() const
{
    return expEnabled_;
}

inline void PluginController::setVolEnabled(bool enabled)
{
    if (volEnabled_ != enabled)
    {
        volEnabled_ = enabled;
        notifyStateChange();
    }
}

inline bool PluginController::isVolEnabled() const
{
    return volEnabled_;
}

inline void PluginController::saveState(juce::MemoryBlock& destData)
{
    PluginState state = getCurrentState();
    
    // Serialize state to memory block
    juce::MemoryOutputStream stream(destData, false);
    
    stream.writeInt(static_cast<int>(state.mode));
    stream.writeInt(state.voiceCount);
    stream.writeInt(state.voiceNumber);
    stream.writeInt(state.bufferSizeMs);
    stream.writeInt(static_cast<int>(pluginFormat_.load()));
    stream.writeBool(state.modEnabled);
    stream.writeBool(state.expEnabled);
    stream.writeBool(state.volEnabled);
    
    // Write version for future compatibility
    stream.writeInt(3);  // Version 3 (added Vol filter)
}

inline bool PluginController::restoreState(const void* data, int sizeInBytes)
{
    if (!data || sizeInBytes < 24)  // Minimum size for all fields
        return false;
        
    juce::MemoryInputStream stream(data, static_cast<size_t>(sizeInBytes), false);
    
    try
    {
        int mode = stream.readInt();
        int voiceCount = stream.readInt();
        int voiceNumber = stream.readInt();
        int bufferSize = stream.readInt();
        int format = stream.readInt();
        
        // Read CC filter settings if available (version 2+)
        bool modEnabled = true;
        bool expEnabled = true;
        bool volEnabled = true;
        
        if (!stream.isExhausted())
        {
            modEnabled = stream.readBool();
            expEnabled = stream.readBool();
        }
        
        if (!stream.isExhausted())
        {
            volEnabled = stream.readBool();
        }
        
        int version = 1;
        if (!stream.isExhausted())
        {
            version = stream.readInt();
        }
        
        // Validate version (support v1, v2, v3)
        if (version < 1 || version > 3)
            return false;

        if (voiceCount < 0 || voiceCount > std::numeric_limits<uint8_t>::max() ||
            voiceNumber < 0 || voiceNumber > std::numeric_limits<uint8_t>::max() ||
            bufferSize < 0 || bufferSize > std::numeric_limits<uint8_t>::max())
            return false;
            
        // Create and validate state
        PluginState state;
        state.mode = static_cast<PluginState::Mode>(mode);
        state.voiceCount = static_cast<uint8_t>(voiceCount);
        state.voiceNumber = static_cast<uint8_t>(voiceNumber);
        state.bufferSizeMs = static_cast<uint8_t>(bufferSize);
        state.modEnabled = modEnabled;
        state.expEnabled = expEnabled;
        state.volEnabled = volEnabled;
        
        if (setCurrentState(state))
        {
            pluginFormat_ = static_cast<PluginFormat>(format);
            return true;
        }
    }
    catch (...)
    {
        return false;
    }
    
    return false;
}

inline PluginController::PluginFormat PluginController::getPluginFormat() const
{
    return pluginFormat_;
}

inline void PluginController::setPluginFormat(PluginFormat format)
{
    pluginFormat_ = format;
}

inline PluginState PluginController::getCurrentState() const
{
    PluginState state;
    
    Mode mode = currentMode_.load();
    state.mode = (mode == Mode::Sender) ? PluginState::Mode::Sender : PluginState::Mode::Receiver;
    state.voiceCount = static_cast<uint8_t>(voiceCount_.load());
    state.voiceNumber = static_cast<uint8_t>(voiceNumber_.load());
    state.bufferSizeMs = static_cast<uint8_t>(bufferSizeMs_.load());
    state.modEnabled = modEnabled_.load();
    state.expEnabled = expEnabled_.load();
    state.volEnabled = volEnabled_.load();
    
    return state;
}

inline bool PluginController::setCurrentState(const PluginState& state)
{
    // Validate all parameters
    if (!isValidVoiceCount(state.voiceCount) ||
        !isValidVoiceNumber(state.voiceNumber) ||
        !isValidBufferSize(state.bufferSizeMs))
    {
        return false;
    }
    
    // Apply state
    Mode newMode = (state.mode == PluginState::Mode::Sender) ? Mode::Sender : Mode::Receiver;
    
    currentMode_ = newMode;
    voiceCount_ = state.voiceCount;
    voiceNumber_ = state.voiceNumber;
    bufferSizeMs_ = state.bufferSizeMs;
    modEnabled_ = state.modEnabled;
    expEnabled_ = state.expEnabled;
    volEnabled_ = state.volEnabled;
    
    notifyStateChange();
    return true;
}

inline bool PluginController::isConfigurationValid() const
{
    return isValidVoiceCount(voiceCount_) &&
           isValidVoiceNumber(voiceNumber_) &&
           isValidBufferSize(bufferSizeMs_);
}

inline std::vector<juce::String> PluginController::getValidationErrors() const
{
    std::vector<juce::String> errors;
    
    if (!isValidVoiceCount(voiceCount_))
    {
        errors.push_back("Voice count must be between " + 
                        juce::String(MIN_VOICE_COUNT) + " and " + 
                        juce::String(MAX_VOICE_COUNT));
    }
    
    if (!isValidVoiceNumber(voiceNumber_))
    {
        errors.push_back("Voice number must be between " + 
                        juce::String(MIN_VOICE_NUMBER) + " and " + 
                        juce::String(MAX_VOICE_NUMBER));
    }
    
    if (!isValidBufferSize(bufferSizeMs_))
    {
        errors.push_back("Buffer size must be between " + 
                        juce::String(MIN_BUFFER_SIZE) + "ms and " + 
                        juce::String(MAX_BUFFER_SIZE) + "ms");
    }
    
    return errors;
}

inline void PluginController::resetToDefaults()
{
    currentMode_ = Mode::Sender;
    voiceCount_ = DEFAULT_VOICE_COUNT;
    voiceNumber_ = DEFAULT_VOICE_NUMBER;
    bufferSizeMs_ = DEFAULT_BUFFER_SIZE;
    
    notifyStateChange();
}

inline void PluginController::setStateChangeCallback(std::function<void()> callback)
{
    juce::ScopedLock lock(callbackLock_);
    stateChangeCallback_ = callback;
}

inline juce::String PluginController::getModeString() const
{
    return (currentMode_ == Mode::Sender) ? "Sender" : "Receiver";
}

inline bool PluginController::setModeFromString(const juce::String& modeString)
{
    if (modeString.equalsIgnoreCase("Sender"))
    {
        setMode(Mode::Sender);
        return true;
    }
    else if (modeString.equalsIgnoreCase("Receiver"))
    {
        setMode(Mode::Receiver);
        return true;
    }
    
    return false;
}

inline juce::String PluginController::getPluginFormatString() const
{
    switch (pluginFormat_.load())
    {
        case PluginFormat::VST3: return "VST3";
        case PluginFormat::AudioUnit: return "AU";
        case PluginFormat::Unknown: 
        default: return "Unknown";
    }
}

inline bool PluginController::isSender() const
{
    return currentMode_ == Mode::Sender;
}

inline bool PluginController::isReceiver() const
{
    return currentMode_ == Mode::Receiver;
}

inline bool PluginController::isValidVoiceCount(int count) const
{
    return count >= MIN_VOICE_COUNT && count <= MAX_VOICE_COUNT;
}

inline bool PluginController::isValidVoiceNumber(int number) const
{
    return number >= MIN_VOICE_NUMBER && number <= MAX_VOICE_NUMBER;
}

inline bool PluginController::isValidBufferSize(int sizeMs) const
{
    return sizeMs >= MIN_BUFFER_SIZE && sizeMs <= MAX_BUFFER_SIZE;
}

inline void PluginController::notifyStateChange()
{
    juce::ScopedLock lock(callbackLock_);
    if (stateChangeCallback_)
    {
        stateChangeCallback_();
    }
}

inline PluginController::PluginFormat PluginController::detectPluginFormat() const
{
    // This would typically be set by the plugin wrapper
    // For now, return Unknown - actual implementation would check
    // JUCE's plugin format detection or compiler defines
    
#if JucePlugin_Build_VST3
    return PluginFormat::VST3;
#elif JucePlugin_Build_AU
    return PluginFormat::AudioUnit;
#else
    return PluginFormat::Unknown;
#endif
}
