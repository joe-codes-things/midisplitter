#pragma once

#include <cstdint>

/**
 * @brief Represents an active (currently sounding) note
 * 
 * This structure tracks notes that have received note-on events but not yet
 * their corresponding note-off events, maintaining voice assignment consistency
 * across buffer boundaries.
 */
struct ActiveNote {
    uint8_t note;               ///< MIDI note number (0-127)
    uint8_t channel;            ///< MIDI channel (0-15)
    int voiceNumber;            ///< Assigned voice (1-16)
    int64_t noteOnTimestamp;    ///< When note-on was received (microseconds)
    int64_t trackingTimestamp;  ///< Monotonic time used for stale-note cleanup (microseconds)
    
    /**
     * @brief Default constructor
     */
    ActiveNote() 
        : note(0)
        , channel(0)
        , voiceNumber(1)
        , noteOnTimestamp(0)
        , trackingTimestamp(0) {}
    
    /**
     * @brief Constructor with all parameters
     * @param note MIDI note number
     * @param channel MIDI channel
     * @param voiceNumber Assigned voice number
     * @param timestamp Note-on timestamp
     */
    ActiveNote(uint8_t noteNumber, uint8_t midiChannel, int assignedVoiceNumber, int64_t timestamp)
        : note(noteNumber)
        , channel(midiChannel)
        , voiceNumber(assignedVoiceNumber)
        , noteOnTimestamp(timestamp)
        , trackingTimestamp(0) {}
    
    /**
     * @brief Check if this active note matches the given note and channel
     * @param n Note number to match
     * @param ch Channel to match
     * @return true if both note and channel match
     */
    bool matches(uint8_t n, uint8_t ch) const {
        return note == n && channel == ch;
    }
    
    /**
     * @brief Check if this note has been active for longer than the given duration
     * @param currentTime Current timestamp in microseconds
     * @param maxAgeMs Maximum age in milliseconds
     * @return true if note is older than maxAgeMs
     */
    bool isStale(int64_t currentTime, int64_t maxAgeMs) const {
        int64_t ageUs = currentTime - noteOnTimestamp;
        int64_t maxAgeUs = maxAgeMs * 1000; // Convert ms to microseconds
        return ageUs > maxAgeUs;
    }
    
    /**
     * @brief Get the age of this note in milliseconds
     * @param currentTime Current timestamp in microseconds
     * @return Age in milliseconds
     */
    int64_t getAgeMs(int64_t currentTime) const {
        return (currentTime - noteOnTimestamp) / 1000; // Convert microseconds to ms
    }
    
    /**
     * @brief Create a unique key for this note (for use in maps/sets)
     * @return 64-bit key combining note and channel
     */
    uint64_t getKey() const {
        return (static_cast<uint64_t>(note) << 8) | channel;
    }
};