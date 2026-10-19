#pragma once

#include <juce_core/juce_core.h>
#include <cstdint>

/**
 * @brief Plugin state for DAW session persistence
 * 
 * This structure holds all plugin configuration that needs to be saved
 * and restored with DAW sessions.
 */
struct PluginState {
    /**
     * @brief Plugin operation mode
     */
    enum Mode : uint8_t {
        Sender = 0,     ///< Sender mode (distributes notes)
        Receiver = 1    ///< Receiver mode (receives assigned voice)
    };
    
    Mode mode;                  ///< Current plugin mode
    uint8_t voiceCount;         ///< Number of voices (2-16, Sender mode only)
    uint8_t voiceNumber;        ///< Assigned voice number (1-16, Receiver mode only)
    uint8_t bufferSizeMs;       ///< Buffer window size in milliseconds (5-10)
    bool modEnabled;            ///< Whether to transmit CC1 (Modulation) data
    bool expEnabled;            ///< Whether to transmit CC11 (Expression) data
    bool volEnabled;            ///< Whether to transmit CC7 (Volume) data
    
    /**
     * @brief Default constructor with sensible defaults
     */
    PluginState() 
        : mode(Sender)
        , voiceCount(4)
        , voiceNumber(1)
        , bufferSizeMs(7)
        , modEnabled(true)
        , expEnabled(true)
        , volEnabled(true) {}
    
    /**
     * @brief Serialize state to memory block for DAW persistence
     * @param destData Memory block to write serialized data
     */
    void serialize(juce::MemoryBlock& destData) const {
        destData.setSize(sizeof(PluginState));
        destData.copyFrom(this, 0, sizeof(PluginState));
    }
    
    /**
     * @brief Deserialize state from memory block
     * @param data Pointer to serialized data
     * @param size Size of data in bytes
     * @return Deserialized PluginState, or default state if invalid
     */
    static PluginState deserialize(const void* data, int size) {
        if (data == nullptr || size != sizeof(PluginState)) {
            return PluginState(); // Return default state if invalid
        }
        
        PluginState state;
        std::memcpy(&state, data, sizeof(PluginState));
        
        // Validate deserialized values
        if (!isValidState(state)) {
            return PluginState(); // Return default state if validation fails
        }
        
        return state;
    }
    
    /**
     * @brief Validate state values are within acceptable ranges
     * @param state State to validate
     * @return true if all values are valid
     */
    static bool isValidState(const PluginState& state) {
        return (state.mode == Sender || state.mode == Receiver) &&
               (state.voiceCount >= 2 && state.voiceCount <= 16) &&
               (state.voiceNumber >= 1 && state.voiceNumber <= 16) &&
               (state.bufferSizeMs >= 5 && state.bufferSizeMs <= 10);
    }
    
    /**
     * @brief Get state as formatted string for debugging
     * @return String representation of state
     */
    juce::String toString() const {
        return juce::String::formatted(
            "Mode: %s, VoiceCount: %d, VoiceNumber: %d, BufferSize: %dms",
            (mode == Sender) ? "Sender" : "Receiver",
            voiceCount,
            voiceNumber,
            bufferSizeMs
        );
    }
};