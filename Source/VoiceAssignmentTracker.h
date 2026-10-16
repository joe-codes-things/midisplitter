#pragma once

#include <unordered_map>
#include <vector>
#include <chrono>
#include <algorithm>
#include "MIDIEvent.h"
#include "ActiveNote.h"

/**
 * VoiceAssignmentTracker class for tracking note-on/off pairs across buffers.
 * 
 * Maintains mappings between note-on events and their assigned voices to ensure
 * corresponding note-off events are sent to the same voice. Handles sustained
 * notes that span multiple buffer windows and provides stale note cleanup.
 * 
 * Requirements: 3.5, 6.5, 14.1, 14.2, 14.3, 14.4
 */
class VoiceAssignmentTracker
{
public:
    /**
     * Constructor with default settings
     */
    VoiceAssignmentTracker()
        : maxActiveNotes_(128)  // Maximum polyphony
        , staleNoteTimeoutMs_(30000)  // 30 second timeout for stale notes
    {
    }
    
    /**
     * Record a note-on event and its assigned voice
     * @param note MIDI note number (0-127)
     * @param channel MIDI channel (0-15)
     * @param voiceNumber Assigned voice number (1-8)
     * @param timestamp Event timestamp
     * @return true if recorded successfully
     */
    bool recordNoteOn(uint8_t note, uint8_t channel, uint8_t voiceNumber, uint64_t timestamp)
    {
        // Check capacity
        if (activeNotes_.size() >= maxActiveNotes_)
        {
            // Try to clean up stale notes first
            clearStaleNotes();
            
            if (activeNotes_.size() >= maxActiveNotes_)
            {
                return false;  // Still at capacity
            }
        }
        
        // Create note key for lookup
        NoteKey key = makeNoteKey(note, channel);
        
        // If note is already active, treat this as a new note-on (retrigger)
        if (activeNotes_.find(key) != activeNotes_.end())
        {
            // Update existing entry
            activeNotes_[key].voiceNumber = voiceNumber;
            activeNotes_[key].noteOnTimestamp = timestamp;
        }
        else
        {
            // Create new active note entry
            ActiveNote activeNote;
            activeNote.note = note;
            activeNote.channel = channel;
            activeNote.voiceNumber = voiceNumber;
            activeNote.noteOnTimestamp = timestamp;
            
            activeNotes_[key] = activeNote;
        }
        
        return true;
    }
    
    /**
     * Get the voice number for a note-off event
     * @param note MIDI note number (0-127)
     * @param channel MIDI channel (0-15)
     * @return Voice number (1-8) if found, 0 if not found
     */
    uint8_t getVoiceForNoteOff(uint8_t note, uint8_t channel) const
    {
        NoteKey key = makeNoteKey(note, channel);
        
        auto it = activeNotes_.find(key);
        if (it != activeNotes_.end())
        {
            return it->second.voiceNumber;
        }
        
        return 0;  // Note not found
    }
    
    /**
     * Record a note-off event and remove from active tracking
     * @param note MIDI note number (0-127)
     * @param channel MIDI channel (0-15)
     * @return Voice number that was assigned, or 0 if not found
     */
    uint8_t recordNoteOff(uint8_t note, uint8_t channel)
    {
        NoteKey key = makeNoteKey(note, channel);
        
        auto it = activeNotes_.find(key);
        if (it != activeNotes_.end())
        {
            uint8_t voiceNumber = it->second.voiceNumber;
            activeNotes_.erase(it);
            return voiceNumber;
        }
        
        return 0;  // Note not found
    }
    
    /**
     * Get all currently active notes
     * @return Vector of active note structures
     */
    std::vector<ActiveNote> getActiveNotes() const
    {
        std::vector<ActiveNote> result;
        result.reserve(activeNotes_.size());
        
        for (const auto& pair : activeNotes_)
        {
            result.push_back(pair.second);
        }
        
        // Sort by timestamp for consistent ordering
        std::sort(result.begin(), result.end(),
                 [](const ActiveNote& a, const ActiveNote& b) {
                     return a.noteOnTimestamp < b.noteOnTimestamp;
                 });
        
        return result;
    }
    
    /**
     * Clear stale notes that have been active too long
     * @return Number of stale notes removed
     */
    int clearStaleNotes()
    {
        uint64_t currentTime = getCurrentTimeMs();
        int removedCount = 0;
        
        auto it = activeNotes_.begin();
        while (it != activeNotes_.end())
        {
            uint64_t noteAge = currentTime - it->second.noteOnTimestamp;
            
            if (noteAge > staleNoteTimeoutMs_)
            {
                it = activeNotes_.erase(it);
                removedCount++;
            }
            else
            {
                ++it;
            }
        }
        
        return removedCount;
    }
    
    /**
     * Get the number of currently active notes
     * @return Number of active notes
     */
    size_t getActiveNoteCount() const
    {
        return activeNotes_.size();
    }
    
    /**
     * Check if a specific note is currently active
     * @param note MIDI note number (0-127)
     * @param channel MIDI channel (0-15)
     * @return true if note is active
     */
    bool isNoteActive(uint8_t note, uint8_t channel) const
    {
        NoteKey key = makeNoteKey(note, channel);
        return activeNotes_.find(key) != activeNotes_.end();
    }
    
    /**
     * Clear all active notes (emergency stop)
     */
    void clearAllNotes()
    {
        activeNotes_.clear();
    }
    
    /**
     * Set maximum number of active notes
     * @param maxNotes Maximum active notes (default 128)
     */
    void setMaxActiveNotes(size_t maxNotes)
    {
        maxActiveNotes_ = maxNotes;
    }
    
    /**
     * Set stale note timeout
     * @param timeoutMs Timeout in milliseconds (default 30000)
     */
    void setStaleNoteTimeout(uint64_t timeoutMs)
    {
        staleNoteTimeoutMs_ = timeoutMs;
    }
    
    /**
     * Get notes active for a specific voice
     * @param voiceNumber Voice number (1-8)
     * @return Vector of active notes for the specified voice
     */
    std::vector<ActiveNote> getActiveNotesForVoice(uint8_t voiceNumber) const
    {
        std::vector<ActiveNote> result;
        
        for (const auto& pair : activeNotes_)
        {
            if (pair.second.voiceNumber == voiceNumber)
            {
                result.push_back(pair.second);
            }
        }
        
        return result;
    }
    
private:
    using NoteKey = uint32_t;  // Combines note and channel into single key
    
    std::unordered_map<NoteKey, ActiveNote> activeNotes_;  // Active note tracking
    size_t maxActiveNotes_;                                // Maximum active notes
    uint64_t staleNoteTimeoutMs_;                         // Stale note timeout
    
    /**
     * Create a unique key for note+channel combination
     * @param note MIDI note number (0-127)
     * @param channel MIDI channel (0-15)
     * @return Unique key for lookup
     */
    static NoteKey makeNoteKey(uint8_t note, uint8_t channel)
    {
        return (static_cast<uint32_t>(channel) << 8) | static_cast<uint32_t>(note);
    }
    
    /**
     * Get current time in milliseconds
     * @return Current time in milliseconds since epoch
     */
    uint64_t getCurrentTimeMs() const
    {
        auto now = std::chrono::steady_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    }
};