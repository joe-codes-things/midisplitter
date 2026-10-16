#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include "MIDIEvent.h"
#include "MIDIParser.h"

/**
 * MIDIProcessor class for integrating with JUCE's MIDI processing pipeline.
 * 
 * Handles conversion between JUCE's MidiBuffer format and our internal MIDIEvent
 * structures. Provides parsing of incoming MIDI data and formatting of outgoing
 * MIDI data for the audio processing thread.
 * 
 * Requirements: 2.1, 2.2, 2.3, 11.3
 */
class MIDIProcessor
{
public:
    /**
     * Constructor
     */
    MIDIProcessor() = default;
    
    /**
     * Process a JUCE MidiBuffer and convert to MIDIEvent vector
     * @param midiBuffer Input JUCE MidiBuffer from audio thread
     * @param sampleRate Current sample rate for timestamp conversion
     * @return Vector of parsed MIDIEvent structures
     */
    std::vector<MIDIEvent> processBlock(const juce::MidiBuffer& midiBuffer, double sampleRate)
    {
        std::vector<MIDIEvent> events;
        
        // Parse all MIDI messages in the buffer
        for (const auto metadata : midiBuffer)
        {
            auto message = metadata.getMessage();
            int samplePosition = metadata.samplePosition;
            
            // Convert sample position to timestamp (microseconds)
            uint64_t timestamp = static_cast<uint64_t>((samplePosition / sampleRate) * 1000000.0);
            
            // Parse the MIDI message
            auto parsedEvent = parseMidiMessage(message, timestamp);
            if (parsedEvent.has_value())
            {
                events.push_back(parsedEvent.value());
            }
        }
        
        return events;
    }
    
    /**
     * Parse a JUCE MidiBuffer into MIDIEvent vector
     * @param midiBuffer Input JUCE MidiBuffer
     * @return Vector of parsed MIDIEvent structures
     */
    std::vector<MIDIEvent> parseBuffer(const juce::MidiBuffer& midiBuffer)
    {
        std::vector<MIDIEvent> events;
        
        for (const auto metadata : midiBuffer)
        {
            auto message = metadata.getMessage();
            int samplePosition = metadata.samplePosition;
            
            // Use sample position as timestamp
            uint64_t timestamp = static_cast<uint64_t>(samplePosition);
            
            auto parsedEvent = parseMidiMessage(message, timestamp);
            if (parsedEvent.has_value())
            {
                events.push_back(parsedEvent.value());
            }
        }
        
        return events;
    }
    
    /**
     * Format MIDIEvent vector into JUCE MidiBuffer
     * @param events Vector of MIDIEvent structures
     * @param sampleRate Current sample rate for timestamp conversion
     * @return JUCE MidiBuffer for audio thread output
     */
    juce::MidiBuffer formatOutput(const std::vector<MIDIEvent>& events, double sampleRate)
    {
        juce::MidiBuffer outputBuffer;
        
        for (const auto& event : events)
        {
            // Convert timestamp to sample position
            int samplePosition = static_cast<int>((event.timestamp / 1000000.0) * sampleRate);
            
            // Create JUCE MidiMessage
            auto midiMessage = createMidiMessage(event);
            if (midiMessage.has_value())
            {
                outputBuffer.addEvent(midiMessage.value(), samplePosition);
            }
        }
        
        return outputBuffer;
    }
    
    /**
     * Format MIDIEvent vector into JUCE MidiBuffer with sample positions
     * @param events Vector of MIDIEvent structures
     * @return JUCE MidiBuffer with sample positions from timestamps
     */
    juce::MidiBuffer formatOutputWithSamples(const std::vector<MIDIEvent>& events)
    {
        juce::MidiBuffer outputBuffer;
        
        for (const auto& event : events)
        {
            // Use timestamp directly as sample position
            int samplePosition = static_cast<int>(event.timestamp);
            
            auto midiMessage = createMidiMessage(event);
            if (midiMessage.has_value())
            {
                outputBuffer.addEvent(midiMessage.value(), samplePosition);
            }
        }
        
        return outputBuffer;
    }
    
    /**
     * Check if a MIDIEvent represents a Control Change message
     * @param event The MIDI event to check
     * @return true if event is a Control Change
     */
    static bool isControlChange(const MIDIEvent& event)
    {
        return (event.status & 0xF0) == 0xB0;
    }
    
    /**
     * Check if a MIDIEvent represents a Note On message
     * @param event The MIDI event to check
     * @return true if event is a Note On (with velocity > 0)
     */
    static bool isNoteOn(const MIDIEvent& event)
    {
        return (event.status & 0xF0) == 0x90 && event.velocity > 0;
    }
    
    /**
     * Check if a MIDIEvent represents a Note Off message
     * @param event The MIDI event to check
     * @return true if event is a Note Off (or Note On with velocity 0)
     */
    static bool isNoteOff(const MIDIEvent& event)
    {
        uint8_t messageType = event.status & 0xF0;
        return (messageType == 0x80) || (messageType == 0x90 && event.velocity == 0);
    }
    
    /**
     * Filter events by type (note events only)
     * @param events Input vector of MIDI events
     * @return Vector containing only note-on and note-off events
     */
    static std::vector<MIDIEvent> filterNoteEvents(const std::vector<MIDIEvent>& events)
    {
        std::vector<MIDIEvent> noteEvents;
        
        for (const auto& event : events)
        {
            if (isNoteOn(event) || isNoteOff(event))
            {
                noteEvents.push_back(event);
            }
        }
        
        return noteEvents;
    }
    
    /**
     * Filter events by type (control change events only)
     * @param events Input vector of MIDI events
     * @return Vector containing only control change events
     */
    static std::vector<MIDIEvent> filterControlEvents(const std::vector<MIDIEvent>& events)
    {
        std::vector<MIDIEvent> controlEvents;
        
        for (const auto& event : events)
        {
            if (isControlChange(event))
            {
                controlEvents.push_back(event);
            }
        }
        
        return controlEvents;
    }
    
private:
    /**
     * Parse a JUCE MidiMessage into our MIDIEvent structure
     * @param message JUCE MidiMessage
     * @param timestamp Timestamp for the event
     * @return MIDIEvent if parsing successful, nullopt otherwise
     */
    std::optional<MIDIEvent> parseMidiMessage(const juce::MidiMessage& message, uint64_t timestamp)
    {
        // Convert JUCE message to raw bytes
        std::vector<uint8_t> bytes;
        const uint8_t* rawData = message.getRawData();
        int dataSize = message.getRawDataSize();
        
        for (int i = 0; i < dataSize; ++i)
        {
            bytes.push_back(rawData[i]);
        }
        
        // Use our MIDIParser to parse the bytes
        return MIDIParser::parse(bytes, timestamp);
    }
    
    /**
     * Create a JUCE MidiMessage from our MIDIEvent structure
     * @param event MIDIEvent to convert
     * @return JUCE MidiMessage if successful, nullopt otherwise
     */
    std::optional<juce::MidiMessage> createMidiMessage(const MIDIEvent& event)
    {
        // Format event to raw bytes using MIDIParser
        auto bytes = MIDIParser::format(event);
        if (bytes.empty())
        {
            return std::nullopt;
        }
        
        // Create JUCE MidiMessage from bytes
        return juce::MidiMessage(bytes.data(), static_cast<int>(bytes.size()));
    }
};