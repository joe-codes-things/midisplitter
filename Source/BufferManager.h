#pragma once

#include <vector>
#include <chrono>
#include <algorithm>
#include "MIDIEvent.h"

/**
 * BufferManager class for collecting and timing MIDI events.
 * 
 * Collects MIDI events over a configurable time window and groups
 * simultaneous events based on timestamp tolerance. Provides buffer
 * flushing when the time window has elapsed.
 * 
 * Requirements: 3.1, 3.2, 3.3, 3.4, 17.1, 17.2
 */
class BufferManager
{
public:
    /**
     * Constructor with default buffer size
     */
    BufferManager() 
        : bufferSizeMs_(7)  // Default 7ms buffer window
        , timestampToleranceUs_(1000)  // Default 1ms tolerance for simultaneity
        , bufferStartTime_(0)
        , isBufferActive_(false)
    {
    }
    
    /**
     * Set the buffer window size in milliseconds
     * @param sizeMs Buffer window size (5-10ms recommended)
     */
    void setBufferSize(int sizeMs)
    {
        if (sizeMs >= 5 && sizeMs <= 10)
        {
            bufferSizeMs_ = sizeMs;
        }
    }
    
    /**
     * Get the current buffer window size
     * @return Buffer size in milliseconds
     */
    int getBufferSize() const
    {
        return bufferSizeMs_;
    }
    
    /**
     * Set timestamp tolerance for simultaneity detection
     * @param toleranceUs Tolerance in microseconds (default 1000us = 1ms)
     */
    void setTimestampTolerance(int toleranceUs)
    {
        timestampToleranceUs_ = toleranceUs;
    }
    
    /**
     * Add a MIDI event to the buffer
     * @param event The MIDI event to add
     * @return true if event was added successfully
     */
    bool addEvent(const MIDIEvent& event)
    {
        // Start buffer timing on first event
        if (!isBufferActive_)
        {
            bufferStartTime_ = getCurrentTimeMs();
            isBufferActive_ = true;
        }
        
        // Check if we have capacity (prevent buffer overflow)
        if (events_.size() >= MAX_EVENTS_PER_BUFFER)
        {
            return false;  // Buffer full
        }
        
        events_.push_back(event);
        return true;
    }
    
    /**
     * Check if the buffer window has elapsed and buffer is ready to flush
     * @return true if buffer should be flushed
     */
    bool isBufferReady() const
    {
        if (!isBufferActive_ || events_.empty())
            return false;
            
        uint64_t currentTime = getCurrentTimeMs();
        uint64_t elapsedTime = currentTime - bufferStartTime_;
        
        return elapsedTime >= static_cast<uint64_t>(bufferSizeMs_);
    }
    
    /**
     * Flush the buffer and return all collected events
     * Groups simultaneous events based on timestamp tolerance
     * @return Vector of MIDI events, sorted by timestamp
     */
    std::vector<MIDIEvent> flushBuffer()
    {
        if (events_.empty())
        {
            return {};
        }
        
        // Sort events by timestamp for proper simultaneity grouping
        std::sort(events_.begin(), events_.end(), 
                 [](const MIDIEvent& a, const MIDIEvent& b) {
                     return a.timestamp < b.timestamp;
                 });
        
        // Group simultaneous events (within tolerance)
        groupSimultaneousEvents();
        
        // Copy events for return
        std::vector<MIDIEvent> result = events_;
        
        // Clear buffer and reset timing
        events_.clear();
        isBufferActive_ = false;
        bufferStartTime_ = 0;
        
        return result;
    }
    
    /**
     * Get the number of events currently in the buffer
     * @return Number of buffered events
     */
    size_t getEventCount() const
    {
        return events_.size();
    }
    
    /**
     * Check if buffer is currently active (collecting events)
     * @return true if buffer is active
     */
    bool isActive() const
    {
        return isBufferActive_;
    }
    
    /**
     * Force flush the buffer regardless of timing
     * @return Vector of collected events
     */
    std::vector<MIDIEvent> forceFlush()
    {
        std::vector<MIDIEvent> result = events_;
        events_.clear();
        isBufferActive_ = false;
        bufferStartTime_ = 0;
        return result;
    }
    
private:
    static constexpr size_t MAX_EVENTS_PER_BUFFER = 128;  // Maximum events per buffer
    
    int bufferSizeMs_;                    // Buffer window size in milliseconds
    int timestampToleranceUs_;            // Timestamp tolerance for simultaneity
    std::vector<MIDIEvent> events_;       // Collected MIDI events
    uint64_t bufferStartTime_;            // Buffer start time in milliseconds
    bool isBufferActive_;                 // Whether buffer is currently collecting
    
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
    
    /**
     * Group events that occur within the timestamp tolerance
     * Events within tolerance are considered simultaneous
     */
    void groupSimultaneousEvents()
    {
        if (events_.size() <= 1)
            return;
            
        // Convert tolerance from microseconds to timestamp units
        // Assuming timestamp is in microseconds
        uint64_t tolerance = static_cast<uint64_t>(timestampToleranceUs_);
        
        // Group events by finding clusters within tolerance
        for (size_t i = 1; i < events_.size(); ++i)
        {
            uint64_t timeDiff = (events_[i].timestamp > events_[i-1].timestamp) 
                              ? (events_[i].timestamp - events_[i-1].timestamp)
                              : (events_[i-1].timestamp - events_[i].timestamp);
                              
            // If events are within tolerance, use the same timestamp
            if (timeDiff <= tolerance)
            {
                events_[i].timestamp = events_[i-1].timestamp;
            }
        }
    }
};