#pragma once

#include <vector>
#include <cstdint>
#include <optional>
#include "MIDIEvent.h"

/**
 * MIDIParser class for converting between raw MIDI bytes and MIDIEvent structures.
 * 
 * Handles parsing of MIDI messages from byte streams and formatting MIDIEvent
 * structures back to MIDI byte sequences. Supports note-on, note-off, and
 * control change messages with proper validation.
 * 
 * Requirements: 19.1, 19.2, 19.3, 19.4
 */
class MIDIParser
{
public:
    /**
     * Parse raw MIDI bytes into a MIDIEvent structure
     * @param bytes Raw MIDI message bytes
     * @param timestamp Timestamp for the event (in samples or milliseconds)
     * @return MIDIEvent if parsing successful, nullopt if invalid
     */
    static std::optional<MIDIEvent> parse(const std::vector<uint8_t>& bytes, uint64_t timestamp = 0)
    {
        if (bytes.empty())
            return std::nullopt;
            
        uint8_t status = bytes[0];
        
        // Validate status byte (must be in range 0x80-0xBF for our supported messages)
        if (!isValidStatusByte(status))
            return std::nullopt;
            
        // Calculate expected message length
        size_t expectedLength = calculateMessageLength(status);
        if (bytes.size() != expectedLength)
            return std::nullopt;
            
        // Validate data bytes (must be 0-127)
        for (size_t i = 1; i < bytes.size(); ++i)
        {
            if (bytes[i] > 127)
                return std::nullopt;
        }
        
        MIDIEvent event;
        event.status = status;
        event.channel = status & 0x0F;  // Lower 4 bits
        event.timestamp = timestamp;
        
        // Parse based on message type
        uint8_t messageType = status & 0xF0;  // Upper 4 bits
        
        switch (messageType)
        {
            case 0x80:  // Note Off
            case 0x90:  // Note On
                if (bytes.size() >= 3)
                {
                    event.note = bytes[1];
                    event.velocity = bytes[2];
                    
                    // Note On with velocity 0 is treated as Note Off
                    if (messageType == 0x90 && event.velocity == 0)
                    {
                        event.status = (event.status & 0x0F) | 0x80;  // Convert to Note Off
                    }
                }
                break;
                
            case 0xB0:  // Control Change
                if (bytes.size() >= 3)
                {
                    event.note = bytes[1];      // Controller number
                    event.velocity = bytes[2];  // Controller value
                }
                break;
                
            default:
                return std::nullopt;  // Unsupported message type
        }
        
        return event;
    }
    
    /**
     * Validate a MIDI event for correctness
     * @param event The MIDI event to validate
     * @return true if event is valid
     */
    static bool validate(const MIDIEvent& event)
    {
        // Check status byte
        if (!isValidStatusByte(event.status))
            return false;
            
        // Check channel (0-15)
        if (event.channel > 15)
            return false;
            
        // Check note/controller number (0-127)
        if (event.note > 127)
            return false;
            
        // Check velocity/value (0-127)
        if (event.velocity > 127)
            return false;
            
        // Verify channel matches status byte
        if ((event.status & 0x0F) != event.channel)
            return false;
            
        return true;
    }
    
    /**
     * Format a MIDIEvent structure into raw MIDI bytes
     * @param event The MIDI event to format
     * @return Vector of MIDI bytes, empty if event is invalid
     */
    static std::vector<uint8_t> format(const MIDIEvent& event)
    {
        if (!validate(event))
            return {};
            
        std::vector<uint8_t> bytes;
        
        uint8_t messageType = event.status & 0xF0;
        
        switch (messageType)
        {
            case 0x80:  // Note Off
            case 0x90:  // Note On
            case 0xB0:  // Control Change
                bytes.push_back(event.status);
                bytes.push_back(event.note);
                bytes.push_back(event.velocity);
                break;
                
            default:
                return {};  // Unsupported message type
        }
        
        return bytes;
    }
    
    /**
     * Calculate the expected length of a MIDI message based on status byte
     * @param status The status byte
     * @return Expected message length in bytes
     */
    static size_t calculateMessageLength(uint8_t status)
    {
        uint8_t messageType = status & 0xF0;
        
        switch (messageType)
        {
            case 0x80:  // Note Off
            case 0x90:  // Note On
            case 0xB0:  // Control Change
                return 3;  // Status + 2 data bytes
                
            default:
                return 0;  // Unknown/unsupported message type
        }
    }
    
private:
    /**
     * Check if a status byte is valid for our supported message types
     * @param status The status byte to check
     * @return true if valid
     */
    static bool isValidStatusByte(uint8_t status)
    {
        uint8_t messageType = status & 0xF0;
        
        // We support Note Off (0x80), Note On (0x90), and Control Change (0xB0)
        return (messageType == 0x80) || (messageType == 0x90) || (messageType == 0xB0);
    }
};