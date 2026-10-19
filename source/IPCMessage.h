#pragma once

#include <vector>
#include <cstdint>
#include <cstring>
#include "MIDIEvent.h"

/**
 * IPCMessage structure for inter-process communication between Sender and Receiver plugins.
 * 
 * This structure defines the binary protocol used to transmit voice assignments
 * and MIDI events between plugin instances via VST3 IConnectionPoint or AU sockets.
 * 
 * Requirements: 8.4, 8.7, 9.4
 */
struct IPCMessage
{
    // Protocol constants
    static constexpr uint32_t MAGIC = 0x4D494449;  // "MIDI" in ASCII
    static constexpr uint32_t VERSION = 1;
    static constexpr size_t MAX_EVENTS_PER_MESSAGE = 128;

    struct Header
    {
        uint32_t magic;
        uint32_t version;
        uint32_t voiceNumber;
        uint32_t eventCount;
        uint32_t timestamp;
    };
    
    // Message header
    uint32_t magic;         // Magic number for protocol validation
    uint32_t version;       // Protocol version
    uint32_t voiceNumber;   // Target voice number (1-8)
    uint32_t eventCount;    // Number of MIDI events in this message
    uint32_t timestamp;     // Message timestamp
    
    // Message payload
    std::vector<MIDIEvent> events;  // MIDI events for this voice
    
    /**
     * Default constructor - initializes with protocol constants
     */
    IPCMessage() 
        : magic(MAGIC)
        , version(VERSION)
        , voiceNumber(1)
        , eventCount(0)
        , timestamp(0)
    {
    }
    
    /**
     * Constructor with voice number
     * @param voice Target voice number (1-8)
     */
    explicit IPCMessage(uint32_t voice)
        : magic(MAGIC)
        , version(VERSION)
        , voiceNumber(voice)
        , eventCount(0)
        , timestamp(0)
    {
    }
    
    /**
     * Calculate the total size of this message in bytes for serialization
     * @return Total message size including header and all events
     */
    size_t calculateSize() const
    {
        return sizeof(Header) + (events.size() * sizeof(MIDIEvent));
    }
    
    /**
     * Add a MIDI event to this message
     * @param event The MIDI event to add
     */
    bool addEvent(const MIDIEvent& event)
    {
        if (events.size() >= MAX_EVENTS_PER_MESSAGE)
            return false;

        events.push_back(event);
        eventCount = static_cast<uint32_t>(events.size());
        return true;
    }
    
    /**
     * Clear all events from this message
     */
    void clearEvents()
    {
        events.clear();
        eventCount = 0;
    }
    
    /**
     * Validate message header for protocol compliance
     * @return true if message header is valid
     */
    bool isValid() const
    {
        return (magic == MAGIC) && 
               (version == VERSION) && 
               (voiceNumber >= 1 && voiceNumber <= 8) &&
               (eventCount == events.size()) &&
               (events.size() <= MAX_EVENTS_PER_MESSAGE);
    }

    uint32_t getVoiceNumber() const { return voiceNumber; }
    void setVoiceNumber(uint32_t voice) { voiceNumber = voice; }

    uint32_t getEventCount() const { return eventCount; }
    const std::vector<MIDIEvent>& getEvents() const { return events; }
    bool isEmpty() const { return events.empty(); }

    void setTimestamp(uint32_t newTimestamp) { timestamp = newTimestamp; }
    uint32_t getTimestamp() const { return timestamp; }

    static constexpr size_t getMaxEventsPerMessage()
    {
        return MAX_EVENTS_PER_MESSAGE;
    }

    size_t serialize(void* destination) const
    {
        if (destination == nullptr)
            return 0;

        Header header { magic, version, voiceNumber, eventCount, timestamp };
        auto* out = static_cast<uint8_t*>(destination);
        std::memcpy(out, &header, sizeof(header));

        if (!events.empty())
        {
            std::memcpy(out + sizeof(header),
                        events.data(),
                        events.size() * sizeof(MIDIEvent));
        }

        return calculateSize();
    }

    bool deserialize(const void* source, size_t size)
    {
        if (source == nullptr || size < sizeof(Header))
            return false;

        Header header;
        std::memcpy(&header, source, sizeof(header));

        const size_t expectedSize = sizeof(Header) + (header.eventCount * sizeof(MIDIEvent));
        if (size != expectedSize || header.eventCount > MAX_EVENTS_PER_MESSAGE)
            return false;

        magic = header.magic;
        version = header.version;
        voiceNumber = header.voiceNumber;
        eventCount = header.eventCount;
        timestamp = header.timestamp;

        events.clear();
        events.resize(eventCount);

        if (eventCount > 0)
        {
            std::memcpy(events.data(),
                        static_cast<const uint8_t*>(source) + sizeof(Header),
                        eventCount * sizeof(MIDIEvent));
        }

        return isValid();
    }
};
