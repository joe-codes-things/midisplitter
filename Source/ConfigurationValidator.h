#pragma once

#include <string>
#include <vector>
#include <algorithm>

/**
 * ConfigurationValidator class for validating plugin configuration parameters.
 * 
 * Provides validation and range checking for all configuration parameters
 * including voice count, buffer size, and voice number. Ensures all values
 * are within acceptable ranges.
 * 
 * Requirements: 12.2, 17.1
 */
class ConfigurationValidator
{
public:
    /**
     * Validation result structure
     */
    struct ValidationResult
    {
        bool isValid;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        
        ValidationResult() : isValid(true) {}
        
        void addError(const std::string& error)
        {
            errors.push_back(error);
            isValid = false;
        }
        
        void addWarning(const std::string& warning)
        {
            warnings.push_back(warning);
        }
        
        bool hasErrors() const { return !errors.empty(); }
        bool hasWarnings() const { return !warnings.empty(); }
    };
    
    /**
     * Validate voice count
     * @param voiceCount Voice count to validate
     * @return Validation result
     */
    static ValidationResult validateVoiceCount(int voiceCount)
    {
        ValidationResult result;
        
        if (voiceCount < MIN_VOICE_COUNT)
        {
            result.addError("Voice count (" + std::to_string(voiceCount) + 
                          ") is below minimum (" + std::to_string(MIN_VOICE_COUNT) + ")");
        }
        else if (voiceCount > MAX_VOICE_COUNT)
        {
            result.addError("Voice count (" + std::to_string(voiceCount) + 
                          ") exceeds maximum (" + std::to_string(MAX_VOICE_COUNT) + ")");
        }
        
        // Warnings for suboptimal values
        if (voiceCount == 2)
        {
            result.addWarning("Voice count of 2 provides minimal distribution benefit");
        }
        else if (voiceCount > 6)
        {
            result.addWarning("Voice count above 6 may increase complexity without significant benefit");
        }
        
        return result;
    }
    
    /**
     * Validate buffer size
     * @param bufferSizeMs Buffer size in milliseconds
     * @return Validation result
     */
    static ValidationResult validateBufferSize(int bufferSizeMs)
    {
        ValidationResult result;
        
        if (bufferSizeMs < MIN_BUFFER_SIZE)
        {
            result.addError("Buffer size (" + std::to_string(bufferSizeMs) + 
                          "ms) is below minimum (" + std::to_string(MIN_BUFFER_SIZE) + "ms)");
        }
        else if (bufferSizeMs > MAX_BUFFER_SIZE)
        {
            result.addError("Buffer size (" + std::to_string(bufferSizeMs) + 
                          "ms) exceeds maximum (" + std::to_string(MAX_BUFFER_SIZE) + "ms)");
        }
        
        // Warnings for edge cases
        if (bufferSizeMs == MIN_BUFFER_SIZE)
        {
            result.addWarning("Minimum buffer size (5ms) may cause timing issues with high note density");
        }
        else if (bufferSizeMs == MAX_BUFFER_SIZE)
        {
            result.addWarning("Maximum buffer size (10ms) may introduce noticeable latency");
        }
        
        return result;
    }
    
    /**
     * Validate voice number
     * @param voiceNumber Voice number to validate
     * @return Validation result
     */
    static ValidationResult validateVoiceNumber(int voiceNumber)
    {
        ValidationResult result;
        
        if (voiceNumber < MIN_VOICE_NUMBER)
        {
            result.addError("Voice number (" + std::to_string(voiceNumber) + 
                          ") is below minimum (" + std::to_string(MIN_VOICE_NUMBER) + ")");
        }
        else if (voiceNumber > MAX_VOICE_NUMBER)
        {
            result.addError("Voice number (" + std::to_string(voiceNumber) + 
                          ") exceeds maximum (" + std::to_string(MAX_VOICE_NUMBER) + ")");
        }
        
        return result;
    }
    
    /**
     * Validate voice number against voice count
     * @param voiceNumber Voice number to validate
     * @param voiceCount Total voice count
     * @return Validation result
     */
    static ValidationResult validateVoiceNumberForCount(int voiceNumber, int voiceCount)
    {
        ValidationResult result = validateVoiceNumber(voiceNumber);
        
        if (result.isValid && voiceNumber > voiceCount)
        {
            result.addError("Voice number (" + std::to_string(voiceNumber) + 
                          ") exceeds voice count (" + std::to_string(voiceCount) + ")");
        }
        
        return result;
    }
    
    /**
     * Clamp voice count to valid range
     * @param voiceCount Voice count to clamp
     * @return Clamped value
     */
    static int clampVoiceCount(int voiceCount)
    {
        return std::clamp(voiceCount, MIN_VOICE_COUNT, MAX_VOICE_COUNT);
    }
    
    /**
     * Clamp buffer size to valid range
     * @param bufferSizeMs Buffer size to clamp
     * @return Clamped value
     */
    static int clampBufferSize(int bufferSizeMs)
    {
        return std::clamp(bufferSizeMs, MIN_BUFFER_SIZE, MAX_BUFFER_SIZE);
    }
    
    /**
     * Clamp voice number to valid range
     * @param voiceNumber Voice number to clamp
     * @return Clamped value
     */
    static int clampVoiceNumber(int voiceNumber)
    {
        return std::clamp(voiceNumber, MIN_VOICE_NUMBER, MAX_VOICE_NUMBER);
    }
    
    /**
     * Clamp voice number to voice count
     * @param voiceNumber Voice number to clamp
     * @param voiceCount Maximum voice count
     * @return Clamped value
     */
    static int clampVoiceNumberToCount(int voiceNumber, int voiceCount)
    {
        int clampedCount = clampVoiceCount(voiceCount);
        return std::clamp(voiceNumber, MIN_VOICE_NUMBER, clampedCount);
    }
    
    /**
     * Check if voice count is valid
     * @param voiceCount Voice count to check
     * @return true if valid
     */
    static bool isValidVoiceCount(int voiceCount)
    {
        return voiceCount >= MIN_VOICE_COUNT && voiceCount <= MAX_VOICE_COUNT;
    }
    
    /**
     * Check if buffer size is valid
     * @param bufferSizeMs Buffer size to check
     * @return true if valid
     */
    static bool isValidBufferSize(int bufferSizeMs)
    {
        return bufferSizeMs >= MIN_BUFFER_SIZE && bufferSizeMs <= MAX_BUFFER_SIZE;
    }
    
    /**
     * Check if voice number is valid
     * @param voiceNumber Voice number to check
     * @return true if valid
     */
    static bool isValidVoiceNumber(int voiceNumber)
    {
        return voiceNumber >= MIN_VOICE_NUMBER && voiceNumber <= MAX_VOICE_NUMBER;
    }
    
    /**
     * Get recommended voice count based on use case
     * @param highPolyphony true for high polyphony scenarios
     * @return Recommended voice count
     */
    static int getRecommendedVoiceCount(bool highPolyphony = false)
    {
        return highPolyphony ? 8 : 4;
    }
    
    /**
     * Get recommended buffer size based on latency requirements
     * @param lowLatency true for low latency scenarios
     * @return Recommended buffer size in milliseconds
     */
    static int getRecommendedBufferSize(bool lowLatency = false)
    {
        return lowLatency ? 5 : 7;
    }
    
    /**
     * Validate all configuration parameters
     * @param voiceCount Voice count
     * @param voiceNumber Voice number
     * @param bufferSizeMs Buffer size
     * @return Combined validation result
     */
    static ValidationResult validateAll(int voiceCount, int voiceNumber, int bufferSizeMs)
    {
        ValidationResult result;
        
        // Validate voice count
        auto vcResult = validateVoiceCount(voiceCount);
        result.errors.insert(result.errors.end(), vcResult.errors.begin(), vcResult.errors.end());
        result.warnings.insert(result.warnings.end(), vcResult.warnings.begin(), vcResult.warnings.end());
        
        // Validate voice number
        auto vnResult = validateVoiceNumberForCount(voiceNumber, voiceCount);
        result.errors.insert(result.errors.end(), vnResult.errors.begin(), vnResult.errors.end());
        result.warnings.insert(result.warnings.end(), vnResult.warnings.begin(), vnResult.warnings.end());
        
        // Validate buffer size
        auto bsResult = validateBufferSize(bufferSizeMs);
        result.errors.insert(result.errors.end(), bsResult.errors.begin(), bsResult.errors.end());
        result.warnings.insert(result.warnings.end(), bsResult.warnings.begin(), bsResult.warnings.end());
        
        result.isValid = result.errors.empty();
        
        return result;
    }
    
    /**
     * Get validation ranges as string
     * @return String describing valid ranges
     */
    static std::string getValidationRanges()
    {
        return "Voice Count: " + std::to_string(MIN_VOICE_COUNT) + "-" + std::to_string(MAX_VOICE_COUNT) + ", " +
               "Voice Number: " + std::to_string(MIN_VOICE_NUMBER) + "-" + std::to_string(MAX_VOICE_NUMBER) + ", " +
               "Buffer Size: " + std::to_string(MIN_BUFFER_SIZE) + "-" + std::to_string(MAX_BUFFER_SIZE) + "ms";
    }
    
private:
    // Validation ranges
    static constexpr int MIN_VOICE_COUNT = 2;
    static constexpr int MAX_VOICE_COUNT = 8;
    static constexpr int MIN_VOICE_NUMBER = 1;
    static constexpr int MAX_VOICE_NUMBER = 8;
    static constexpr int MIN_BUFFER_SIZE = 5;   // milliseconds
    static constexpr int MAX_BUFFER_SIZE = 10;  // milliseconds
};