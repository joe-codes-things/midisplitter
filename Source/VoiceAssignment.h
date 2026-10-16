#pragma once

#include "MIDIEvent.h"
#include <vector>

/**
 * @brief Represents notes assigned to a specific voice
 * 
 * This structure holds all MIDI events that have been assigned to a particular
 * voice during the distribution process.
 */
struct VoiceAssignment {
    int voiceNumber;                    ///< Voice number (1-8)
    std::vector<MIDIEvent> notes;       ///< Notes assigned to this voice
    
    /**
     * @brief Get the number of notes assigned to this voice
     * @return Number of notes in the notes vector
     */
    int getNoteCount() const {
        return static_cast<int>(notes.size());
    }
    
    /**
     * @brief Default constructor
     */
    VoiceAssignment() : voiceNumber(1) {}
    
    /**
     * @brief Constructor with voice number
     * @param voiceNum Voice number (1-8)
     */
    explicit VoiceAssignment(int voiceNum) : voiceNumber(voiceNum) {}
    
    /**
     * @brief Add a note to this voice assignment
     * @param event MIDI event to add
     */
    void addNote(const MIDIEvent& event) {
        notes.push_back(event);
    }
    
    /**
     * @brief Clear all notes from this voice assignment
     */
    void clear() {
        notes.clear();
    }
    
    /**
     * @brief Check if this voice assignment is empty
     * @return true if no notes are assigned
     */
    bool isEmpty() const {
        return notes.empty();
    }
};