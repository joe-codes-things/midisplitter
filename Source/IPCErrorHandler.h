#pragma once

#include <chrono>
#include <atomic>
#include <thread>
#include <functional>

/**
 * IPCErrorHandler class for handling IPC communication failures.
 * 
 * Provides error handling with automatic reconnection logic, retry attempts,
 * and connection status monitoring. Implements exponential backoff for
 * reconnection attempts.
 * 
 * Requirements: 16.1, 16.2, 16.3, 16.4
 */
class IPCErrorHandler
{
public:
    /**
     * Error types
     */
    enum class ErrorType
    {
        ConnectionLost,
        SendFailed,
        ReceiveFailed,
        Timeout,
        InvalidMessage,
        Unknown
    };
    
    /**
     * Connection status
     */
    enum class ConnectionStatus
    {
        Connected,
        Disconnected,
        Reconnecting,
        Failed
    };
    
    /**
     * Constructor
     */
    IPCErrorHandler()
        : maxRetryAttempts_(3)
        , retryDelayMs_(100)
        , currentRetryAttempt_(0)
        , connectionStatus_(ConnectionStatus::Disconnected)
        , lastErrorTime_(0)
        , errorCount_(0)
    {
    }
    
    /**
     * Handle an IPC error
     * @param errorType Type of error that occurred
     * @param errorMessage Descriptive error message
     * @return true if error was handled successfully
     */
    bool handleError(ErrorType errorType, const std::string& errorMessage = "")
    {
        // Update error statistics
        errorCount_++;
        lastErrorTime_ = getCurrentTimeMs();
        
        // Log error if callback is set
        if (errorCallback_)
        {
            errorCallback_(errorType, errorMessage);
        }
        
        // Handle based on error type
        switch (errorType)
        {
            case ErrorType::ConnectionLost:
                connectionStatus_ = ConnectionStatus::Disconnected;
                return attemptReconnection();
                
            case ErrorType::SendFailed:
            case ErrorType::ReceiveFailed:
                // Try to reconnect if connection seems broken
                if (currentRetryAttempt_ < maxRetryAttempts_)
                {
                    return attemptReconnection();
                }
                break;
                
            case ErrorType::Timeout:
                // Timeout might be temporary, try once
                if (currentRetryAttempt_ == 0)
                {
                    return attemptReconnection();
                }
                break;
                
            case ErrorType::InvalidMessage:
                // Invalid message doesn't require reconnection
                return true;
                
            default:
                break;
        }
        
        connectionStatus_ = ConnectionStatus::Failed;
        return false;
    }
    
    /**
     * Attempt to reconnect with retry logic
     * @return true if reconnection successful or in progress
     */
    bool attemptReconnection()
    {
        if (currentRetryAttempt_ >= maxRetryAttempts_)
        {
            connectionStatus_ = ConnectionStatus::Failed;
            return false;
        }
        
        connectionStatus_ = ConnectionStatus::Reconnecting;
        
        // Calculate delay with exponential backoff
        int delayMs = retryDelayMs_ * (1 << currentRetryAttempt_);  // 100ms, 200ms, 400ms...
        
        // Wait before retry
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        
        // Attempt reconnection via callback
        if (reconnectCallback_)
        {
            bool success = reconnectCallback_();
            
            if (success)
            {
                connectionStatus_ = ConnectionStatus::Connected;
                currentRetryAttempt_ = 0;  // Reset retry counter
                return true;
            }
        }
        
        currentRetryAttempt_++;
        
        // If we haven't exceeded max attempts, we're still trying
        if (currentRetryAttempt_ < maxRetryAttempts_)
        {
            return attemptReconnection();  // Recursive retry
        }
        
        connectionStatus_ = ConnectionStatus::Failed;
        return false;
    }
    
    /**
     * Check if currently connected
     * @return true if connected
     */
    bool isConnected() const
    {
        return connectionStatus_ == ConnectionStatus::Connected;
    }
    
    /**
     * Get current connection status
     * @return Connection status
     */
    ConnectionStatus getConnectionStatus() const
    {
        return connectionStatus_;
    }
    
    /**
     * Set connection status (for external updates)
     * @param status New connection status
     */
    void setConnectionStatus(ConnectionStatus status)
    {
        connectionStatus_ = status;
        
        if (status == ConnectionStatus::Connected)
        {
            currentRetryAttempt_ = 0;  // Reset retry counter on successful connection
        }
    }
    
    /**
     * Set maximum retry attempts
     * @param attempts Maximum number of retry attempts (1-10)
     */
    void setMaxRetryAttempts(int attempts)
    {
        if (attempts >= 1 && attempts <= 10)
        {
            maxRetryAttempts_ = attempts;
        }
    }
    
    /**
     * Set retry delay
     * @param delayMs Delay in milliseconds between retries
     */
    void setRetryDelay(int delayMs)
    {
        if (delayMs >= 50 && delayMs <= 5000)
        {
            retryDelayMs_ = delayMs;
        }
    }
    
    /**
     * Set reconnection callback
     * @param callback Function to call to attempt reconnection
     */
    void setReconnectCallback(std::function<bool()> callback)
    {
        reconnectCallback_ = callback;
    }
    
    /**
     * Set error callback
     * @param callback Function to call when error occurs
     */
    void setErrorCallback(std::function<void(ErrorType, const std::string&)> callback)
    {
        errorCallback_ = callback;
    }
    
    /**
     * Reset error handler state
     */
    void reset()
    {
        currentRetryAttempt_ = 0;
        errorCount_ = 0;
        lastErrorTime_ = 0;
        connectionStatus_ = ConnectionStatus::Disconnected;
    }
    
    /**
     * Get total error count
     * @return Number of errors handled
     */
    int getErrorCount() const
    {
        return errorCount_;
    }
    
    /**
     * Get time since last error
     * @return Milliseconds since last error
     */
    uint64_t getTimeSinceLastError() const
    {
        if (lastErrorTime_ == 0)
            return 0;
            
        return getCurrentTimeMs() - lastErrorTime_;
    }
    
    /**
     * Get current retry attempt number
     * @return Current retry attempt (0 if not retrying)
     */
    int getCurrentRetryAttempt() const
    {
        return currentRetryAttempt_;
    }
    
    /**
     * Get connection status as string
     * @return Status string
     */
    std::string getStatusString() const
    {
        switch (connectionStatus_)
        {
            case ConnectionStatus::Connected: return "Connected";
            case ConnectionStatus::Disconnected: return "Disconnected";
            case ConnectionStatus::Reconnecting: return "Reconnecting";
            case ConnectionStatus::Failed: return "Failed";
            default: return "Unknown";
        }
    }
    
private:
    int maxRetryAttempts_;
    int retryDelayMs_;
    std::atomic<int> currentRetryAttempt_;
    std::atomic<ConnectionStatus> connectionStatus_;
    uint64_t lastErrorTime_;
    std::atomic<int> errorCount_;
    
    std::function<bool()> reconnectCallback_;
    std::function<void(ErrorType, const std::string&)> errorCallback_;
    
    /**
     * Get current time in milliseconds
     * @return Current time in milliseconds
     */
    uint64_t getCurrentTimeMs() const
    {
        auto now = std::chrono::steady_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    }
};