#pragma once

#include <vector>
#include <algorithm>
#include "MIDIEvent.h"

/**
 * PitchSorter class for sorting MIDI events by pitch in descending order.
 * 
 * Provides stable sorting of MIDI events based on note pitch, with highest
 * pitches first. Maintains relative order of events with identical pitches
 * to preserve timing relationships.
 * 
 * Requirements: 4.1, 4.2
 */
class PitchSorter
{
public:
    /**
     * Sort MIDI events by pitch in descending order (highest pitch first)
     * Uses stable sort to preserve relative order of events with same pitch
     * @param events Vector of MIDI events to sort (modified in-place)
     */
    static void sort(std::vector<MIDIEvent>& events)
    {
        std::stable_sort(events.begin(), events.end(), compareByPitch);
    }
    
    /**
     * Sort MIDI events and return a new sorted vector
     * @param events Vector of MIDI events to sort (original unchanged)
     * @return New vector with events sorted by pitch (descending)
     */
    static std::vector<MIDIEvent> sortCopy(const std::vector<MIDIEvent>& events)
    {
        std::vector<MIDIEvent> sortedEvents = events;
        sort(sortedEvents);
        return sortedEvents;
    }
    
    /**
     * Check if events are already sorted by pitch (descending)
     * @param events Vector of MIDI events to check
     * @return true if events are sorted in descending pitch order
     */
    static bool isSorted(const std::vector<MIDIEvent>& events)
    {
        if (events.size() <= 1)
            return true;
            
        for (size_t i = 1; i < events.size(); ++i)
        {
            // Only check note-on and note-off events for pitch ordering
            if (isNoteEvent(events[i-1]) && isNoteEvent(events[i]))
            {
                if (events[i-1].note < events[i].note)
                {
                    return false;  // Found ascending order, not sorted
                }
            }
        }
        
        return true;
    }
    
    /**
     * Get the highest pitch from a collection of MIDI events
     * @param events Vector of MIDI events
     * @return Highest note number, or 0 if no note events found
     */
    static uint8_t getHighestPitch(const std::vector<MIDIEvent>& events)
    {
        uint8_t highest = 0;
        
        for (const auto& event : events)
        {
            if (isNoteEvent(event) && event.note > highest)
            {
                highest = event.note;
            }
        }
        
        return highest;
    }
    
    /**
     * Get the lowest pitch from a collection of MIDI events
     * @param events Vector of MIDI events
     * @return Lowest note number, or 127 if no note events found
     */
    static uint8_t getLowestPitch(const std::vector<MIDIEvent>& events)
    {
        uint8_t lowest = 127;
        bool foundNote = false;
        
        for (const auto& event : events)
        {
            if (isNoteEvent(event))
            {
                if (!foundNote || event.note < lowest)
                {
                    lowest = event.note;
                    foundNote = true;
                }
            }
        }
        
        return foundNote ? lowest : 0;
    }
    
private:
    /**
     * Comparator function for sorting by pitch (descending order)
     * @param a First MIDI event
     * @param b Second MIDI event
     * @return true if a should come before b in sorted order
     */
    static bool compareByPitch(const MIDIEvent& a, const MIDIEvent& b)
    {
        // Handle non-note events (Control Changes, etc.)
        bool aIsNote = isNoteEvent(a);
        bool bIsNote = isNoteEvent(b);
        
        // Note events come before non-note events
        if (aIsNote && !bIsNote)
            return true;
        if (!aIsNote && bIsNote)
            return false;
            
        // If both are note events, sort by pitch (descending)
        if (aIsNote && bIsNote)
        {
            if (a.note != b.note)
                return a.note > b.note;  // Higher pitch first
                
            // If same pitch, maintain original order (stable sort)
            // Secondary sort by timestamp to maintain timing
            return a.timestamp <= b.timestamp;
        }
        
        // If both are non-note events, maintain original order
        return a.timestamp <= b.timestamp;
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