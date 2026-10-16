#pragma once

#include <algorithm>
#include <cstdint>

/**
 * PluginConfiguration structure for managing plugin settings and defaults.
 * 
 * Defines configuration parameters with validation and default values.
 * Provides centralized configuration management with range enforcement
 * and validation methods.
 * 
 * Requirements: 17.1, 17.2, 17.3
 */
struct PluginConfiguration
{
    // Buffer management settings
    int bufferSizeMs;              // Buffer window size in milliseconds (5-10ms)
    int timestampToleranceUs;      // Timestamp tolerance for simultaneity detection (microseconds)
    
    // Voice assignment settings
    int maxVoiceCount;             // Maximum number of voices (2-16)
    int defaultVoiceCount;         // Default voice count for new instances
    
    // IPC communication settings
    int ipcTimeoutMs;              // IPC connection timeout in milliseconds
    int ipcRetryAttempts;          // Number of retry attempts for failed connections
    int ipcRetryDelayMs;           // Delay between retry attempts
    
    // Performance settings
    int maxNotesPerBuffer;         // Maximum MIDI notes per buffer (overflow protection)
    int maxActiveNotes;            // Maximum simultaneously active notes
    int staleNoteTimeoutMs;        // Timeout for cleaning up stale notes
    
    // Error handling settings
    int maxErrorsPerSecond;        // Rate limit for error logging
    bool enableErrorRecovery;      // Enable automatic error recovery
    
    // Latency settings
    int targetLatencyMs;           // Target total latency (< 10ms)
    int latencyMeasurementWindow;  // Window size for latency averaging
    
    /**
     * Get default configuration
     * @return Default configuration with recommended values
     */
    static PluginConfiguration getDefault()
    {
        PluginConfiguration config;
        
        // Buffer management
        config.bufferSizeMs = 7;                    // 7ms buffer window (balanced)
        config.timestampToleranceUs = 1000;        // 1ms tolerance for simultaneity
        
        // Voice assignment
        config.maxVoiceCount = 16;                  // Maximum 16 voices
        config.defaultVoiceCount = 4;               // Default 4 voices
        
        // IPC communication
        config.ipcTimeoutMs = 5000;                 // 5 second timeout
        config.ipcRetryAttempts = 3;                // 3 retry attempts
        config.ipcRetryDelayMs = 100;               // 100ms between retries
        
        // Performance
        config.maxNotesPerBuffer = 128;             // 128 notes max per buffer
        config.maxActiveNotes = 128;                // 128 active notes max
        config.staleNoteTimeoutMs = 30000;          // 30 second stale note timeout
        
        // Error handling
        config.maxErrorsPerSecond = 100;            // 100 errors/second rate limit
        config.enableErrorRecovery = true;          // Enable recovery by default
        
        // Latency
        config.targetLatencyMs = 10;                // Target < 10ms total latency
        config.latencyMeasurementWindow = 100;      // 100 sample averaging window
        
        return config;
    }
    
    /**
     * Validate and clamp all configuration values to valid ranges
     * @return Reference to this configuration (for chaining)
     */
    PluginConfiguration& validate()
    {
        // Validate buffer settings
        bufferSizeMs = std::clamp(bufferSizeMs, 5, 10);
        timestampToleranceUs = std::clamp(timestampToleranceUs, 100, 10000);  // 0.1ms to 10ms
        
        // Validate voice settings
        maxVoiceCount = std::clamp(maxVoiceCount, 2, 16);
        defaultVoiceCount = std::clamp(defaultVoiceCount, 2, maxVoiceCount);
        
        // Validate IPC settings
        ipcTimeoutMs = std::clamp(ipcTimeoutMs, 1000, 30000);        // 1-30 seconds
        ipcRetryAttempts = std::clamp(ipcRetryAttempts, 1, 10);      // 1-10 attempts
        ipcRetryDelayMs = std::clamp(ipcRetryDelayMs, 50, 1000);     // 50ms-1s delay
        
        // Validate performance settings
        maxNotesPerBuffer = std::clamp(maxNotesPerBuffer, 16, 256);  // 16-256 notes
        maxActiveNotes = std::clamp(maxActiveNotes, 32, 512);        // 32-512 active notes
        staleNoteTimeoutMs = std::clamp(staleNoteTimeoutMs, 5000, 300000);  // 5s-5min
        
        // Validate error handling
        maxErrorsPerSecond = std::clamp(maxErrorsPerSecond, 10, 1000);  // 10-1000 errors/sec
        
        // Validate latency settings
        targetLatencyMs = std::clamp(targetLatencyMs, 5, 50);        // 5-50ms target
        latencyMeasurementWindow = std::clamp(latencyMeasurementWindow, 10, 1000);  // 10-1000 samples
        
        return *this;
    }
    
    /**
     * Check if configuration is valid (within acceptable ranges)
     * @return true if all values are valid
     */
    bool isValid() const
    {
        return (bufferSizeMs >= 5 && bufferSizeMs <= 10) &&
               (timestampToleranceUs >= 100 && timestampToleranceUs <= 10000) &&
               (maxVoiceCount >= 2 && maxVoiceCount <= 16) &&
               (defaultVoiceCount >= 2 && defaultVoiceCount <= maxVoiceCount) &&
               (ipcTimeoutMs >= 1000 && ipcTimeoutMs <= 30000) &&
               (ipcRetryAttempts >= 1 && ipcRetryAttempts <= 10) &&
               (ipcRetryDelayMs >= 50 && ipcRetryDelayMs <= 1000) &&
               (maxNotesPerBuffer >= 16 && maxNotesPerBuffer <= 256) &&
               (maxActiveNotes >= 32 && maxActiveNotes <= 512) &&
               (staleNoteTimeoutMs >= 5000 && staleNoteTimeoutMs <= 300000) &&
               (maxErrorsPerSecond >= 10 && maxErrorsPerSecond <= 1000) &&
               (targetLatencyMs >= 5 && targetLatencyMs <= 50) &&
               (latencyMeasurementWindow >= 10 && latencyMeasurementWindow <= 1000);
    }
    
    /**
     * Get configuration optimized for low latency
     * @return Configuration with minimal latency settings
     */
    static PluginConfiguration getLowLatency()
    {
        auto config = getDefault();
        
        config.bufferSizeMs = 5;                    // Minimum buffer size
        config.timestampToleranceUs = 500;         // Tighter tolerance
        config.ipcTimeoutMs = 2000;                 // Shorter timeout
        config.targetLatencyMs = 5;                 // Aggressive latency target
        
        return config.validate();
    }
    
    /**
     * Get configuration optimized for stability
     * @return Configuration with conservative settings
     */
    static PluginConfiguration getStable()
    {
        auto config = getDefault();
        
        config.bufferSizeMs = 10;                   // Larger buffer for stability
        config.timestampToleranceUs = 2000;        // More tolerant timing
        config.ipcTimeoutMs = 10000;                // Longer timeout
        config.ipcRetryAttempts = 5;                // More retry attempts
        config.targetLatencyMs = 15;                // More relaxed latency
        
        return config.validate();
    }
    
    /**
     * Get configuration for high polyphony scenarios
     * @return Configuration optimized for many simultaneous notes
     */
    static PluginConfiguration getHighPolyphony()
    {
        auto config = getDefault();
        
        config.maxNotesPerBuffer = 256;             // More notes per buffer
        config.maxActiveNotes = 512;                // More active notes
        config.bufferSizeMs = 8;                    // Slightly larger buffer
        config.maxVoiceCount = 16;                  // Maximum voices
        config.defaultVoiceCount = 16;              // Default to max voices
        
        return config.validate();
    }
    
    /**
     * Copy constructor
     */
    PluginConfiguration(const PluginConfiguration& other) = default;
    
    /**
     * Assignment operator
     */
    PluginConfiguration& operator=(const PluginConfiguration& other) = default;
    
    /**
     * Default constructor (uses default values)
     */
    PluginConfiguration()
    {
        bufferSizeMs = 7;
        timestampToleranceUs = 1000;
        maxVoiceCount = 16;
        defaultVoiceCount = 4;
        ipcTimeoutMs = 5000;
        ipcRetryAttempts = 3;
        ipcRetryDelayMs = 100;
        maxNotesPerBuffer = 128;
        maxActiveNotes = 128;
        staleNoteTimeoutMs = 30000;
        maxErrorsPerSecond = 100;
        enableErrorRecovery = true;
        targetLatencyMs = 10;
        latencyMeasurementWindow = 100;
    }
    
    /**
     * Equality comparison
     */
    bool operator==(const PluginConfiguration& other) const
    {
        return bufferSizeMs == other.bufferSizeMs &&
               timestampToleranceUs == other.timestampToleranceUs &&
               maxVoiceCount == other.maxVoiceCount &&
               defaultVoiceCount == other.defaultVoiceCount &&
               ipcTimeoutMs == other.ipcTimeoutMs &&
               ipcRetryAttempts == other.ipcRetryAttempts &&
               ipcRetryDelayMs == other.ipcRetryDelayMs &&
               maxNotesPerBuffer == other.maxNotesPerBuffer &&
               maxActiveNotes == other.maxActiveNotes &&
               staleNoteTimeoutMs == other.staleNoteTimeoutMs &&
               maxErrorsPerSecond == other.maxErrorsPerSecond &&
               enableErrorRecovery == other.enableErrorRecovery &&
               targetLatencyMs == other.targetLatencyMs &&
               latencyMeasurementWindow == other.latencyMeasurementWindow;
    }
    
    /**
     * Inequality comparison
     */
    bool operator!=(const PluginConfiguration& other) const
    {
        return !(*this == other);
    }
};
