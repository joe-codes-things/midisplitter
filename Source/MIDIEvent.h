#pragma once

#include <cstdint>

/**
 * @brief Represents a MIDI event with all necessary metadata
 * 
 * This structure encapsulates MIDI note events and control change messages
 * with timing information for the MIDI Splitter Plugin.
 */
struct MIDIEvent {
    uint8_t status;      ///< MIDI status byte (0x80-0x9F for note events, 0xB0-0xBF for CC)
    uint8_t note;        ///< Note number (0-127) or CC number
    uint8_t velocity;    ///< Velocity (0-127) or CC value
    uint8_t channel;     ///< MIDI channel (0-15)
    int64_t timestamp;   ///< Microseconds since buffer start
    
    /**
     * @brief Check if this is a note-on event
     * @return true if status is 0x90-0x9F and velocity > 0
     */
    bool isNoteOn() const {
        return (status >= 0x90 && status <= 0x9F) && velocity > 0;
    }
    
    /**
     * @brief Check if this is a note-off event
     * @return true if status is 0x80-0x8F or (0x90-0x9F with velocity = 0)
     */
    bool isNoteOff() const {
        return (status >= 0x80 && status <= 0x8F) || 
               ((status >= 0x90 && status <= 0x9F) && velocity == 0);
    }
    
    /**
     * @brief Check if this is a control change event
     * @return true if status is 0xB0-0xBF
     */
    bool isControlChange() const {
        return status >= 0xB0 && status <= 0xBF;
    }
    
    /**
     * @brief Extract MIDI channel from status byte
     * @return MIDI channel (0-15)
     */
    uint8_t getChannel() const {
        return status & 0x0F;
    }
    
    /**
     * @brief Default constructor
     */
    MIDIEvent() : status(0), note(0), velocity(0), channel(0), timestamp(0) {}
    
    /**
     * @brief Constructor with all parameters
     */
    MIDIEvent(uint8_t statusByte, uint8_t noteNumber, uint8_t velocityValue, uint8_t channelNumber, int64_t timestampValue)
        : status(statusByte), note(noteNumber), velocity(velocityValue), channel(channelNumber), timestamp(timestampValue) {}
};