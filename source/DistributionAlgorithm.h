#pragma once

#include <vector>
#include <algorithm>
#include "MIDIEvent.h"
#include "VoiceAssignment.h"

/**
 * DistributionAlgorithm class for distributing MIDI events across multiple voices.
 * 
 * Implements ratio-based distribution with round-robin assignment for extra notes.
 * Distributes notes as evenly as possible across the specified number of voices,
 * maintaining pitch order within each voice.
 * 
 * Requirements: 5.1, 5.2, 5.3, 5.4, 5.5, 6.1, 6.2, 6.3, 6.4
 */
class DistributionAlgorithm
{
public:
    /**
     * Distribute MIDI events across multiple voices
     * @param events Vector of MIDI events (should be sorted by pitch)
     * @param voiceCount Number of voices to distribute across (2-16)
     * @return Vector of VoiceAssignment structures, one per voice
     */
    static std::vector<VoiceAssignment> distribute(const std::vector<MIDIEvent>& events, int voiceCount)
    {
        // Validate input parameters
        if (voiceCount < 2 || voiceCount > 16)
        {
            return {};  // Invalid voice count
        }
        
        // Initialize voice assignments
        std::vector<VoiceAssignment> assignments;
        assignments.reserve(static_cast<size_t>(voiceCount));
        
        for (size_t i = 0; i < static_cast<size_t>(voiceCount); ++i)
        {
            assignments.emplace_back(static_cast<int>(i + 1));  // Voice numbers are 1-based
        }
        
        // Handle empty input
        if (events.empty())
        {
            return assignments;
        }
        
        // Filter note events only (ignore Control Changes for distribution)
        std::vector<MIDIEvent> noteEvents;
        std::vector<MIDIEvent> controlEvents;
        
        for (const auto& event : events)
        {
            if (isNoteEvent(event))
            {
                noteEvents.push_back(event);
            }
            else
            {
                controlEvents.push_back(event);
            }
        }
        
        // Distribute note events
        if (!noteEvents.empty())
        {
            distributeNoteEvents(noteEvents, assignments);
        }
        
        // Add control events to all voices (broadcast)
        for (auto& assignment : assignments)
        {
            for (const auto& controlEvent : controlEvents)
            {
                assignment.notes.push_back(controlEvent);
            }
        }
        
        return assignments;
    }
    
    /**
     * Calculate how many notes each voice should get (base amount)
     * @param totalNotes Total number of notes to distribute
     * @param voiceCount Number of voices
     * @return Base number of notes per voice (floor division)
     */
    static int calculateNotesPerVoice(int totalNotes, int voiceCount)
    {
        if (voiceCount <= 0)
            return 0;
            
        return totalNotes / voiceCount;
    }
    
    /**
     * Calculate how many extra notes need round-robin assignment
     * @param totalNotes Total number of notes to distribute
     * @param voiceCount Number of voices
     * @return Number of extra notes (modulo operation)
     */
    static int calculateExtraNotes(int totalNotes, int voiceCount)
    {
        if (voiceCount <= 0)
            return 0;
            
        return totalNotes % voiceCount;
    }
    
    /**
     * Get distribution statistics for a given note count and voice count
     * @param totalNotes Total number of notes
     * @param voiceCount Number of voices
     * @return Vector showing how many notes each voice will get
     */
    static std::vector<int> getDistributionPattern(int totalNotes, int voiceCount)
    {
        if (voiceCount <= 0)
            return {};
            
        const size_t voiceCountSize = static_cast<size_t>(voiceCount);
        std::vector<int> pattern(voiceCountSize, 0);
        
        int baseNotes = calculateNotesPerVoice(totalNotes, voiceCount);
        int extraNotes = calculateExtraNotes(totalNotes, voiceCount);
        
        // Assign base notes to all voices
        for (size_t i = 0; i < voiceCountSize; ++i)
        {
            pattern[i] = baseNotes;
        }
        
        // Assign extra notes using round-robin
        for (size_t i = 0; i < static_cast<size_t>(extraNotes); ++i)
        {
            pattern[i]++;
        }
        
        return pattern;
    }
    
private:
    /**
     * Distribute note events across voice assignments
     * @param noteEvents Vector of note events to distribute
     * @param assignments Vector of voice assignments to populate
     */
    static void distributeNoteEvents(const std::vector<MIDIEvent>& noteEvents, 
                                   std::vector<VoiceAssignment>& assignments)
    {
        const size_t totalNotes = noteEvents.size();
        const size_t voiceCount = assignments.size();
        
        const size_t baseNotes = totalNotes / voiceCount;
        const size_t extraNotes = totalNotes % voiceCount;
        
        // Assign notes using round-robin with ratio-based distribution
        size_t eventIndex = 0;
        
        for (size_t voice = 0; voice < voiceCount && eventIndex < totalNotes; ++voice)
        {
            // Calculate how many notes this voice gets
            size_t notesForThisVoice = baseNotes;
            if (voice < extraNotes)
            {
                notesForThisVoice++;  // This voice gets an extra note
            }
            
            // Assign notes to this voice
            for (size_t note = 0; note < notesForThisVoice && eventIndex < totalNotes; ++note)
            {
                assignments[voice].notes.push_back(noteEvents[eventIndex]);
                eventIndex++;
            }
        }
    }
    
    /**
     * Assign extra notes using round-robin pattern
     * @param extraEvents Vector of extra events to assign
     * @param assignments Vector of voice assignments
     * @param startVoice Voice index to start round-robin from
     */
    static void assignRoundRobin(const std::vector<MIDIEvent>& extraEvents,
                                std::vector<VoiceAssignment>& assignments,
                                int startVoice = 0)
    {
        if (extraEvents.empty() || assignments.empty())
            return;
            
        const size_t voiceCount = assignments.size();
        const size_t currentVoiceStart = static_cast<size_t>(
            ((startVoice % static_cast<int>(voiceCount)) + static_cast<int>(voiceCount)) %
            static_cast<int>(voiceCount));
        size_t currentVoice = currentVoiceStart;
        
        for (const auto& event : extraEvents)
        {
            assignments[currentVoice].notes.push_back(event);
            currentVoice = (currentVoice + 1) % voiceCount;
        }
    }
    
    /**
     * Check if a MIDI event is a note event (note-on or note-off)
     * @param event The MIDI event to check
     * @return true if event is note-on or note-off
     */
    static bool isNoteEvent(const MIDIEvent& event)
    {
        uint8_t messageType = event.status & 0xF0;
        return (messageType == 0x80) || (messageType == 0x90);  // Note Off or Note On
    }
};