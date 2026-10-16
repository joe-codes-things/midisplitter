#pragma once

#include <juce_core/juce_core.h>
#include <atomic>
#include <vector>
#include "IPCMessage.h"

/**
 * LockFreeQueue class for thread-safe message queuing without locks.
 * 
 * Uses JUCE's AbstractFifo for lock-free communication between audio thread
 * and IPC thread. Provides non-blocking push and pop operations suitable
 * for real-time audio processing.
 * 
 * Requirements: 9.6, 10.1, 10.2
 */
template<typename T>
class LockFreeQueue
{
public:
    /**
     * Constructor
     * @param capacity Maximum number of items in queue
     */
    explicit LockFreeQueue(int capacity = 1024)
        : fifo_(capacity)
        , buffer_(capacity)
    {
    }
    
    /**
     * Push an item to the queue (non-blocking)
     * @param item Item to push
     * @return true if item was pushed, false if queue is full
     */
    bool push(const T& item)
    {
        int start1, size1, start2, size2;
        fifo_.prepareToWrite(1, start1, size1, start2, size2);
        
        if (size1 > 0)
        {
            buffer_[start1] = item;
            fifo_.finishedWrite(1);
            return true;
        }
        
        return false;  // Queue is full
    }
    
    /**
     * Pop an item from the queue (non-blocking)
     * @param item Output item
     * @return true if item was popped, false if queue is empty
     */
    bool pop(T& item)
    {
        int start1, size1, start2, size2;
        fifo_.prepareToRead(1, start1, size1, start2, size2);
        
        if (size1 > 0)
        {
            item = buffer_[start1];
            fifo_.finishedRead(1);
            return true;
        }
        
        return false;  // Queue is empty
    }
    
    /**
     * Try to push multiple items at once
     * @param items Vector of items to push
     * @param maxItems Maximum number of items to push
     * @return Number of items actually pushed
     */
    int pushMultiple(const std::vector<T>& items, int maxItems = -1)
    {
        if (items.empty())
            return 0;
            
        int itemsToPush = static_cast<int>(items.size());
        if (maxItems > 0 && itemsToPush > maxItems)
            itemsToPush = maxItems;
            
        int start1, size1, start2, size2;
        fifo_.prepareToWrite(itemsToPush, start1, size1, start2, size2);
        
        int itemsPushed = 0;
        
        // Write to first block
        if (size1 > 0)
        {
            int count1 = std::min(size1, itemsToPush);
            for (int i = 0; i < count1; ++i)
            {
                buffer_[start1 + i] = items[i];
            }
            itemsPushed += count1;
        }
        
        // Write to second block if needed
        if (size2 > 0 && itemsPushed < itemsToPush)
        {
            int count2 = std::min(size2, itemsToPush - itemsPushed);
            for (int i = 0; i < count2; ++i)
            {
                buffer_[start2 + i] = items[itemsPushed + i];
            }
            itemsPushed += count2;
        }
        
        if (itemsPushed > 0)
        {
            fifo_.finishedWrite(itemsPushed);
        }
        
        return itemsPushed;
    }
    
    /**
     * Try to pop multiple items at once
     * @param items Output vector for items
     * @param maxItems Maximum number of items to pop
     * @return Number of items actually popped
     */
    int popMultiple(std::vector<T>& items, int maxItems)
    {
        items.clear();
        
        if (maxItems <= 0)
            return 0;
            
        int start1, size1, start2, size2;
        fifo_.prepareToRead(maxItems, start1, size1, start2, size2);
        
        int itemsPopped = 0;
        
        // Read from first block
        if (size1 > 0)
        {
            for (int i = 0; i < size1; ++i)
            {
                items.push_back(buffer_[start1 + i]);
            }
            itemsPopped += size1;
        }
        
        // Read from second block if available
        if (size2 > 0)
        {
            for (int i = 0; i < size2; ++i)
            {
                items.push_back(buffer_[start2 + i]);
            }
            itemsPopped += size2;
        }
        
        if (itemsPopped > 0)
        {
            fifo_.finishedRead(itemsPopped);
        }
        
        return itemsPopped;
    }
    
    /**
     * Get number of items available to read
     * @return Number of items in queue
     */
    int getNumReady() const
    {
        return fifo_.getNumReady();
    }
    
    /**
     * Get available space for writing
     * @return Number of free slots
     */
    int getFreeSpace() const
    {
        return fifo_.getFreeSpace();
    }
    
    /**
     * Check if queue is empty
     * @return true if queue is empty
     */
    bool isEmpty() const
    {
        return fifo_.getNumReady() == 0;
    }
    
    /**
     * Check if queue is full
     * @return true if queue is full
     */
    bool isFull() const
    {
        return fifo_.getFreeSpace() == 0;
    }
    
    /**
     * Get total capacity of the queue
     * @return Maximum number of items
     */
    int getCapacity() const
    {
        return static_cast<int>(buffer_.size());
    }
    
    /**
     * Reset the queue to empty state
     */
    void reset()
    {
        fifo_.reset();
    }
    
private:
    juce::AbstractFifo fifo_;
    std::vector<T> buffer_;
};

/**
 * Specialized queue for IPC messages
 */
using IPCMessageQueue = LockFreeQueue<IPCMessage>;

/**
 * Specialized queue for MIDI events
 */
using MIDIEventQueue = LockFreeQueue<MIDIEvent>;

/**
 * Queue manager for audio thread to IPC thread communication
 */
class AudioToIPCQueue
{
public:
    /**
     * Constructor
     */
    AudioToIPCQueue() 
        : messageQueue_(512)  // 512 message capacity
    {
    }
    
    /**
     * Send message from audio thread to IPC thread
     * @param message IPC message to send
     * @return true if sent successfully
     */
    bool sendToIPC(const IPCMessage& message)
    {
        return messageQueue_.push(message);
    }
    
    /**
     * Receive message in IPC thread from audio thread
     * @param message Output message
     * @return true if message received
     */
    bool receiveFromAudio(IPCMessage& message)
    {
        return messageQueue_.pop(message);
    }
    
    /**
     * Get number of pending messages
     * @return Number of messages waiting
     */
    int getPendingCount() const
    {
        return messageQueue_.getNumReady();
    }
    
    /**
     * Check if queue has capacity
     * @return true if can accept more messages
     */
    bool hasCapacity() const
    {
        return !messageQueue_.isFull();
    }
    
private:
    IPCMessageQueue messageQueue_;
};

/**
 * Queue manager for IPC thread to audio thread communication
 */
class IPCToAudioQueue
{
public:
    /**
     * Constructor
     */
    IPCToAudioQueue()
        : eventQueue_(2048)  // 2048 event capacity for incoming MIDI
    {
    }
    
    /**
     * Send MIDI events from IPC thread to audio thread
     * @param events Vector of MIDI events
     * @return Number of events actually queued
     */
    int sendToAudio(const std::vector<MIDIEvent>& events)
    {
        return eventQueue_.pushMultiple(events);
    }
    
    /**
     * Receive MIDI events in audio thread from IPC thread
     * @param events Output vector for events
     * @param maxEvents Maximum events to receive
     * @return Number of events received
     */
    int receiveFromIPC(std::vector<MIDIEvent>& events, int maxEvents = 128)
    {
        return eventQueue_.popMultiple(events, maxEvents);
    }
    
    /**
     * Get number of pending events
     * @return Number of events waiting
     */
    int getPendingCount() const
    {
        return eventQueue_.getNumReady();
    }
    
    /**
     * Check if queue has capacity
     * @return true if can accept more events
     */
    bool hasCapacity() const
    {
        return !eventQueue_.isFull();
    }
    
private:
    MIDIEventQueue eventQueue_;
};