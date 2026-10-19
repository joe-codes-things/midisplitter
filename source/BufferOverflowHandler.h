#pragma once

#include <vector>
#include <atomic>
#include "MIDIEvent.h"

/**
 * BufferOverflowHandler class for managing buffer capacity and overflow.
 * 
 * Provides capacity checking and FIFO dropping strategy when buffer is full.
 * Tracks overflow statistics and implements graceful degradation under
 * high load conditions.
 * 
 * Requirements: 3.1
 */
class BufferOverflowHandler
{
public:
    /**
     * Constructor
     */
    BufferOverflowHandler()
        : maxNotesPerBuffer_(MAX_NOTES_PER_BUFFER)
        , overflowCount_(0)
        , totalDroppedNotes_(0)
    {
    }
    
    /**
     * Check if a note can be added to the buffer
     * @param currentSize Current number of notes in buffer
     * @return true if note can be added
     */
    bool canAddNote(size_t currentSize) const
    {
        return currentSize < maxNotesPerBuffer_;
    }
    
    /**
     * Handle buffer overflow by dropping oldest notes (FIFO)
     * @param buffer Current buffer of MIDI events
     * @param newEvent New event to add
     * @return true if event was added (possibly after dropping old events)
     */
    bool handleOverflow(std::vector<MIDIEvent>& buffer, const MIDIEvent& newEvent)
    {
        if (buffer.size() < maxNotesPerBuffer_)
        {
            // No overflow, just add
            buffer.push_back(newEvent);
            return true;
        }
        
        // Buffer is full - implement FIFO dropping
        overflowCount_++;
        
        // Remove oldest event (first in vector)
        if (!buffer.empty())
        {
            buffer.erase(buffer.begin());
            totalDroppedNotes_++;
            
            // Call overflow callback if set
            if (overflowCallback_)
            {
                overflowCallback_(buffer.size(), maxNotesPerBuffer_);
            }
        }
        
        // Add new event
        buffer.push_back(newEvent);
        return true;
    }
    
    /**
     * Handle overflow with priority-based dropping
     * Drops control changes before note events
     * @param buffer Current buffer of MIDI events
     * @param newEvent New event to add
     * @return true if event was added
     */
    bool handleOverflowWithPriority(std::vector<MIDIEvent>& buffer, const MIDIEvent& newEvent)
    {
        if (buffer.size() < maxNotesPerBuffer_)
        {
            buffer.push_back(newEvent);
            return true;
        }
        
        overflowCount_++;
        
        // Try to find and remove a control change event first
        for (auto it = buffer.begin(); it != buffer.end(); ++it)
        {
            if ((it->status & 0xF0) == 0xB0)  // Control Change
            {
                buffer.erase(it);
                totalDroppedNotes_++;
                buffer.push_back(newEvent);
                
                if (overflowCallback_)
                {
                    overflowCallback_(buffer.size(), maxNotesPerBuffer_);
                }
                
                return true;
            }
        }
        
        // No control changes found, drop oldest note event (FIFO)
        if (!buffer.empty())
        {
            buffer.erase(buffer.begin());
            totalDroppedNotes_++;
        }
        
        buffer.push_back(newEvent);
        
        if (overflowCallback_)
        {
            overflowCallback_(buffer.size(), maxNotesPerBuffer_);
        }
        
        return true;
    }
    
    /**
     * Set maximum notes per buffer
     * @param maxNotes Maximum capacity (16-256)
     */
    void setMaxNotesPerBuffer(size_t maxNotes)
    {
        if (maxNotes >= 16 && maxNotes <= 256)
        {
            maxNotesPerBuffer_ = maxNotes;
        }
    }
    
    /**
     * Get maximum notes per buffer
     * @return Maximum capacity
     */
    size_t getMaxNotesPerBuffer() const
    {
        return maxNotesPerBuffer_;
    }
    
    /**
     * Get overflow count
     * @return Number of times overflow occurred
     */
    int getOverflowCount() const
    {
        return overflowCount_;
    }
    
    /**
     * Get total dropped notes
     * @return Total number of notes dropped due to overflow
     */
    int getTotalDroppedNotes() const
    {
        return totalDroppedNotes_;
    }
    
    /**
     * Set overflow callback
     * @param callback Function to call when overflow occurs
     */
    void setOverflowCallback(std::function<void(size_t, size_t)> callback)
    {
        overflowCallback_ = callback;
    }
    
    /**
     * Reset overflow statistics
     */
    void reset()
    {
        overflowCount_ = 0;
        totalDroppedNotes_ = 0;
    }
    
    /**
     * Check if buffer is near capacity
     * @param currentSize Current buffer size
     * @param threshold Threshold percentage (0.0-1.0)
     * @return true if buffer is above threshold
     */
    bool isNearCapacity(size_t currentSize, double threshold = 0.8) const
    {
        return static_cast<double>(currentSize) >=
               (static_cast<double>(maxNotesPerBuffer_) * threshold);
    }
    
    /**
     * Get available capacity
     * @param currentSize Current buffer size
     * @return Number of available slots
     */
    size_t getAvailableCapacity(size_t currentSize) const
    {
        if (currentSize >= maxNotesPerBuffer_)
            return 0;
            
        return maxNotesPerBuffer_ - currentSize;
    }
    
    /**
     * Get capacity utilization percentage
     * @param currentSize Current buffer size
     * @return Utilization as percentage (0-100)
     */
    double getCapacityUtilization(size_t currentSize) const
    {
        if (maxNotesPerBuffer_ == 0)
            return 0.0;
            
        return (static_cast<double>(currentSize) /
                static_cast<double>(maxNotesPerBuffer_)) * 100.0;
    }
    
    /**
     * Compact buffer by removing duplicate note-offs
     * @param buffer Buffer to compact
     * @return Number of events removed
     */
    int compactBuffer(std::vector<MIDIEvent>& buffer)
    {
        if (buffer.size() <= 1)
            return 0;
            
        int removedCount = 0;
        
        // Track seen note-offs to remove duplicates
        std::vector<bool> toRemove(buffer.size(), false);
        
        for (size_t i = 0; i < buffer.size(); ++i)
        {
            if ((buffer[i].status & 0xF0) == 0x80)  // Note Off
            {
                // Check if we've seen this note-off before
                for (size_t j = i + 1; j < buffer.size(); ++j)
                {
                    if ((buffer[j].status & 0xF0) == 0x80 &&
                        buffer[j].note == buffer[i].note &&
                        buffer[j].channel == buffer[i].channel)
                    {
                        // Duplicate note-off found, mark for removal
                        toRemove[j] = true;
                        removedCount++;
                    }
                }
            }
        }
        
        // Remove marked events
        if (removedCount > 0)
        {
            auto newEnd = std::remove_if(buffer.begin(), buffer.end(),
                [&toRemove, &buffer](const MIDIEvent& event) {
                    size_t index = static_cast<size_t>(&event - buffer.data());
                    return index < toRemove.size() && toRemove[index];
                });
            buffer.erase(newEnd, buffer.end());
        }
        
        return removedCount;
    }
    
private:
    static constexpr size_t MAX_NOTES_PER_BUFFER = 128;
    
    size_t maxNotesPerBuffer_;
    std::atomic<int> overflowCount_;
    std::atomic<int> totalDroppedNotes_;
    
    std::function<void(size_t, size_t)> overflowCallback_;
};