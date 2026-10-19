#pragma once

#include <chrono>
#include <atomic>
#include <optional>
#include <vector>
#include "MIDIEvent.h"
#include "MIDIParser.h"

/**
 * MIDIParsingErrorHandler class for handling MIDI parsing errors with recovery.
 * 
 * Provides error recovery for malformed MIDI data with rate limiting to prevent
 * log flooding. Implements graceful degradation when encountering invalid MIDI.
 * 
 * Requirements: 2.2, 19.2
 */
class MIDIParsingErrorHandler
{
public:
    /**
     * Constructor
     */
    MIDIParsingErrorHandler()
        : maxErrorsPerSecond_(100)
        , errorCount_(0)
        , lastResetTime_(getCurrentTimeMs())
        , totalErrorCount_(0)
    {
    }
    
    /**
     * Parse MIDI bytes with error handling and recovery
     * @param bytes Raw MIDI message bytes
     * @param timestamp Timestamp for the event
     * @return MIDIEvent if parsing successful, nullopt if invalid
     */
    std::optional<MIDIEvent> parseWithValidation(const std::vector<uint8_t>& bytes, int64_t timestamp = 0)
    {
        // Attempt to parse
        auto result = MIDIParser::parse(bytes, timestamp);
        
        if (!result.has_value())
        {
            // Parsing failed - log error with rate limiting
            logParseError("Failed to parse MIDI message", bytes);
            
            // Attempt recovery
            auto recovered = attemptRecovery(bytes, timestamp);
            if (recovered.has_value())
            {
                return recovered;
            }
        }
        
        return result;
    }
    
    /**
     * Log a parsing error with rate limiting
     * @param message Error message
     * @param bytes Raw MIDI bytes that failed to parse
     */
    void logParseError(const std::string& message, const std::vector<uint8_t>& bytes)
    {
        // Check rate limit
        const int64_t currentTime = getCurrentTimeMs();
        const int64_t timeSinceReset = currentTime - lastResetTime_;
        
        // Reset counter every second
        if (timeSinceReset >= 1000)
        {
            errorCount_ = 0;
            lastResetTime_ = currentTime;
        }
        
        // Check if we're within rate limit
        if (errorCount_ < maxErrorsPerSecond_)
        {
            errorCount_++;
            totalErrorCount_++;
            
            // Call error callback if set
            if (errorCallback_)
            {
                std::string bytesStr = formatBytes(bytes);
                errorCallback_(message + ": " + bytesStr);
            }
        }
        else if (errorCount_ == maxErrorsPerSecond_)
        {
            // Log rate limit message once
            errorCount_++;
            if (errorCallback_)
            {
                errorCallback_("MIDI parsing error rate limit reached (100 errors/second)");
            }
        }
    }
    
    /**
     * Set maximum errors per second for rate limiting
     * @param maxErrors Maximum errors per second (10-1000)
     */
    void setMaxErrorsPerSecond(int maxErrors)
    {
        if (maxErrors >= 10 && maxErrors <= 1000)
        {
            maxErrorsPerSecond_ = maxErrors;
        }
    }
    
    /**
     * Set error callback
     * @param callback Function to call when error occurs
     */
    void setErrorCallback(std::function<void(const std::string&)> callback)
    {
        errorCallback_ = callback;
    }
    
    /**
     * Get total error count
     * @return Total number of parsing errors
     */
    int getTotalErrorCount() const
    {
        return totalErrorCount_;
    }
    
    /**
     * Get current error rate (errors per second)
     * @return Current error rate
     */
    int getCurrentErrorRate() const
    {
        const int64_t timeSinceReset = getCurrentTimeMs() - lastResetTime_;
        if (timeSinceReset == 0)
            return 0;
            
        return static_cast<int>((errorCount_ * 1000) / timeSinceReset);
    }
    
    /**
     * Reset error statistics
     */
    void reset()
    {
        errorCount_ = 0;
        totalErrorCount_ = 0;
        lastResetTime_ = getCurrentTimeMs();
    }
    
    /**
     * Check if rate limit is currently active
     * @return true if rate limit exceeded
     */
    bool isRateLimited() const
    {
        return errorCount_ >= maxErrorsPerSecond_;
    }
    
private:
    int maxErrorsPerSecond_;
    std::atomic<int> errorCount_;
    int64_t lastResetTime_;
    std::atomic<int> totalErrorCount_;
    
    std::function<void(const std::string&)> errorCallback_;
    
    /**
     * Attempt to recover from parsing error
     * @param bytes Raw MIDI bytes
     * @param timestamp Event timestamp
     * @return Recovered MIDIEvent if possible, nullopt otherwise
     */
    std::optional<MIDIEvent> attemptRecovery(const std::vector<uint8_t>& bytes, int64_t timestamp)
    {
        if (bytes.empty())
            return std::nullopt;
            
        // Try to identify and fix common issues
        
        // Issue 1: Invalid status byte - try to find valid status in bytes
        for (size_t i = 0; i < bytes.size(); ++i)
        {
            uint8_t status = bytes[i];
            uint8_t messageType = status & 0xF0;
            
            // Check if this looks like a valid status byte
            if (messageType == 0x80 || messageType == 0x90 || messageType == 0xB0)
            {
                // Try to parse from this position
                std::vector<uint8_t> recoveredBytes(
                    bytes.begin() + static_cast<std::ptrdiff_t>(i), bytes.end());
                auto result = MIDIParser::parse(recoveredBytes, timestamp);
                
                if (result.has_value())
                {
                    logParseError("Recovered from invalid status byte", bytes);
                    return result;
                }
            }
        }
        
        // Issue 2: Data bytes out of range - try to clamp them
        if (bytes.size() >= 3)
        {
            std::vector<uint8_t> clampedBytes = bytes;
            
            // Clamp data bytes to valid range (0-127)
            for (size_t i = 1; i < clampedBytes.size(); ++i)
            {
                if (clampedBytes[i] > 127)
                {
                    clampedBytes[i] = 127;
                }
            }
            
            auto result = MIDIParser::parse(clampedBytes, timestamp);
            if (result.has_value())
            {
                logParseError("Recovered by clamping data bytes", bytes);
                return result;
            }
        }
        
        // Issue 3: Wrong message length - try to truncate or pad
        if (!bytes.empty())
        {
            uint8_t status = bytes[0];
            size_t expectedLength = MIDIParser::calculateMessageLength(status);
            
            if (expectedLength > 0 && bytes.size() != expectedLength)
            {
                std::vector<uint8_t> fixedBytes;
                
                if (bytes.size() > expectedLength)
                {
                    // Truncate
                    fixedBytes.assign(bytes.begin(),
                                      bytes.begin() + static_cast<std::ptrdiff_t>(expectedLength));
                }
                else
                {
                    // Pad with zeros
                    fixedBytes = bytes;
                    while (fixedBytes.size() < expectedLength)
                    {
                        fixedBytes.push_back(0);
                    }
                }
                
                auto result = MIDIParser::parse(fixedBytes, timestamp);
                if (result.has_value())
                {
                    logParseError("Recovered by fixing message length", bytes);
                    return result;
                }
            }
        }
        
        // Recovery failed
        return std::nullopt;
    }
    
    /**
     * Format bytes as hex string for logging
     * @param bytes Bytes to format
     * @return Hex string representation
     */
    std::string formatBytes(const std::vector<uint8_t>& bytes) const
    {
        if (bytes.empty())
            return "[]";
            
        std::string result = "[";
        for (size_t i = 0; i < bytes.size(); ++i)
        {
            char hex[4];
            snprintf(hex, sizeof(hex), "%02X", bytes[i]);
            result += hex;
            
            if (i < bytes.size() - 1)
                result += " ";
        }
        result += "]";
        
        return result;
    }
    
    /**
     * Get current time in milliseconds
     * @return Current time in milliseconds
     */
    int64_t getCurrentTimeMs() const
    {
        auto now = std::chrono::steady_clock::now();
        auto duration = now.time_since_epoch();
        return static_cast<int64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
    }
};