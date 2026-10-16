#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>
#include <chrono>
#include <array>

/**
 * @brief Tracks and measures latency across different processing stages
 * 
 * Provides detailed latency measurement for buffer window, distribution algorithm,
 * and IPC transmission stages with statistical analysis.
 */
class LatencyTracker {
public:
    /**
     * @brief Latency measurement stages
     */
    enum class Stage {
        BufferWindow,      ///< Time spent in buffer window collection
        Distribution,      ///< Time spent in distribution algorithm
        IPCTransmission,   ///< Time spent in IPC transmission
        Total             ///< Total end-to-end latency
    };
    
    /**
     * @brief Latency statistics for a stage
     */
    struct LatencyStats {
        std::atomic<float> averageMs{0.0f};     ///< Average latency in milliseconds
        std::atomic<float> peakMs{0.0f};        ///< Peak latency in milliseconds
        std::atomic<float> minimumMs{1000.0f};  ///< Minimum latency in milliseconds
        std::atomic<int> sampleCount{0};        ///< Number of samples collected
        std::atomic<bool> targetExceeded{false}; ///< Whether target was exceeded
    };
    
    /**
     * @brief Constructor
     */
    LatencyTracker();
    
    /**
     * @brief Destructor
     */
    ~LatencyTracker() = default;
    
    /**
     * @brief Start timing measurement for a stage
     * @param stage The processing stage to measure
     * @return Measurement ID for ending the measurement
     */
    int startMeasurement(Stage stage);
    
    /**
     * @brief End timing measurement for a stage
     * @param measurementId The measurement ID returned by startMeasurement
     */
    void endMeasurement(int measurementId);
    
    /**
     * @brief Record a latency measurement directly
     * @param stage The processing stage
     * @param latencyMs Latency in milliseconds
     */
    void recordLatency(Stage stage, float latencyMs);
    
    /**
     * @brief Get statistics for a stage
     * @param stage The processing stage
     * @return Latency statistics
     */
    const LatencyStats& getStats(Stage stage) const;
    
    /**
     * @brief Reset all statistics
     */
    void resetStats();
    
    /**
     * @brief Set target latency for a stage
     * @param stage The processing stage
     * @param targetMs Target latency in milliseconds
     */
    void setTarget(Stage stage, float targetMs);
    
    /**
     * @brief Get target latency for a stage
     * @param stage The processing stage
     * @return Target latency in milliseconds
     */
    float getTarget(Stage stage) const;
    
    /**
     * @brief Check if any stage exceeded its target
     * @return true if any target was exceeded
     */
    bool hasTargetExceeded() const;
    
    /**
     * @brief Get total latency (sum of all stages)
     * @return Total latency in milliseconds
     */
    float getTotalLatency() const;
    
    /**
     * @brief Get latency report as formatted string
     * @return Formatted latency report
     */
    juce::String getLatencyReport() const;
    
    /**
     * @brief Enable/disable latency tracking
     * @param enabled Whether tracking should be enabled
     */
    void setTrackingEnabled(bool enabled);
    
    /**
     * @brief Check if tracking is enabled
     * @return true if tracking is enabled
     */
    bool isTrackingEnabled() const;

private:
    /**
     * @brief Active measurement data
     */
    struct ActiveMeasurement {
        Stage stage;
        std::chrono::high_resolution_clock::time_point startTime;
        bool active;
    };
    
    /**
     * @brief Update statistics with new measurement
     * @param stage The processing stage
     * @param latencyMs New latency measurement
     */
    void updateStats(Stage stage, float latencyMs);
    
    /**
     * @brief Get stage name as string
     * @param stage The processing stage
     * @return Stage name
     */
    static juce::String getStageString(Stage stage);

private:
    // Statistics for each stage
    std::array<LatencyStats, 4> stats_;
    
    // Target latencies for each stage
    std::array<std::atomic<float>, 4> targets_;
    
    // Active measurements
    std::array<ActiveMeasurement, 16> activeMeasurements_;
    std::atomic<int> nextMeasurementId_;
    
    // Configuration
    std::atomic<bool> trackingEnabled_;
    
    // Thread safety
    mutable std::mutex statsMutex_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LatencyTracker)
};

// Implementation

inline LatencyTracker::LatencyTracker()
    : nextMeasurementId_(0)
    , trackingEnabled_(true) {
    
    // Initialize targets based on requirements
    targets_[static_cast<int>(Stage::BufferWindow)] = 10.0f;    // 5-10ms buffer window
    targets_[static_cast<int>(Stage::Distribution)] = 1.0f;     // < 1ms for distribution
    targets_[static_cast<int>(Stage::IPCTransmission)] = 2.0f;  // < 2ms for IPC
    targets_[static_cast<int>(Stage::Total)] = 10.0f;          // < 10ms total
    
    // Initialize active measurements
    for (auto& measurement : activeMeasurements_) {
        measurement.active = false;
    }
}

inline int LatencyTracker::startMeasurement(Stage stage) {
    if (!trackingEnabled_) {
        return -1;
    }
    
    int measurementId = nextMeasurementId_.fetch_add(1) % activeMeasurements_.size();
    
    auto& measurement = activeMeasurements_[measurementId];
    measurement.stage = stage;
    measurement.startTime = std::chrono::high_resolution_clock::now();
    measurement.active = true;
    
    return measurementId;
}

inline void LatencyTracker::endMeasurement(int measurementId) {
    if (!trackingEnabled_ || measurementId < 0 || 
        measurementId >= static_cast<int>(activeMeasurements_.size())) {
        return;
    }
    
    auto& measurement = activeMeasurements_[measurementId];
    if (!measurement.active) {
        return;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        endTime - measurement.startTime);
    
    float latencyMs = static_cast<float>(duration.count()) / 1000.0f;
    
    recordLatency(measurement.stage, latencyMs);
    measurement.active = false;
}

inline void LatencyTracker::recordLatency(Stage stage, float latencyMs) {
    if (!trackingEnabled_) {
        return;
    }
    
    updateStats(stage, latencyMs);
    
    // Check if target was exceeded
    int stageIndex = static_cast<int>(stage);
    float target = targets_[stageIndex].load();
    if (latencyMs > target) {
        stats_[stageIndex].targetExceeded = true;
    }
}

inline const LatencyTracker::LatencyStats& LatencyTracker::getStats(Stage stage) const {
    return stats_[static_cast<int>(stage)];
}

inline void LatencyTracker::resetStats() {
    std::lock_guard<std::mutex> lock(statsMutex_);
    
    for (auto& stat : stats_) {
        stat.averageMs = 0.0f;
        stat.peakMs = 0.0f;
        stat.minimumMs = 1000.0f;
        stat.sampleCount = 0;
        stat.targetExceeded = false;
    }
}

inline void LatencyTracker::setTarget(Stage stage, float targetMs) {
    targets_[static_cast<int>(stage)] = targetMs;
}

inline float LatencyTracker::getTarget(Stage stage) const {
    return targets_[static_cast<int>(stage)].load();
}

inline bool LatencyTracker::hasTargetExceeded() const {
    for (const auto& stat : stats_) {
        if (stat.targetExceeded.load()) {
            return true;
        }
    }
    return false;
}

inline float LatencyTracker::getTotalLatency() const {
    return stats_[static_cast<int>(Stage::Total)].averageMs.load();
}

inline juce::String LatencyTracker::getLatencyReport() const {
    juce::String report;
    
    report << "Latency Report:\n";
    report << "===============\n";
    
    for (int i = 0; i < 4; ++i) {
        Stage stage = static_cast<Stage>(i);
        const auto& stat = stats_[i];
        float target = targets_[i].load();
        
        report << getStageString(stage) << ":\n";
        report << "  Average: " << juce::String(stat.averageMs.load(), 2) << " ms\n";
        report << "  Peak: " << juce::String(stat.peakMs.load(), 2) << " ms\n";
        report << "  Minimum: " << juce::String(stat.minimumMs.load(), 2) << " ms\n";
        report << "  Target: " << juce::String(target, 2) << " ms\n";
        report << "  Samples: " << stat.sampleCount.load() << "\n";
        report << "  Target Exceeded: " << (stat.targetExceeded.load() ? "YES" : "NO") << "\n";
        report << "\n";
    }
    
    return report;
}

inline void LatencyTracker::setTrackingEnabled(bool enabled) {
    trackingEnabled_ = enabled;
}

inline bool LatencyTracker::isTrackingEnabled() const {
    return trackingEnabled_;
}

inline void LatencyTracker::updateStats(Stage stage, float latencyMs) {
    std::lock_guard<std::mutex> lock(statsMutex_);
    
    int stageIndex = static_cast<int>(stage);
    auto& stat = stats_[stageIndex];
    
    // Update sample count
    int currentCount = stat.sampleCount.load();
    stat.sampleCount = currentCount + 1;
    
    // Update average (running average)
    float currentAverage = stat.averageMs.load();
    float newAverage = (currentAverage * currentCount + latencyMs) / (currentCount + 1);
    stat.averageMs = newAverage;
    
    // Update peak
    float currentPeak = stat.peakMs.load();
    if (latencyMs > currentPeak) {
        stat.peakMs = latencyMs;
    }
    
    // Update minimum
    float currentMin = stat.minimumMs.load();
    if (latencyMs < currentMin) {
        stat.minimumMs = latencyMs;
    }
}

inline juce::String LatencyTracker::getStageString(Stage stage) {
    switch (stage) {
        case Stage::BufferWindow:
            return "Buffer Window";
        case Stage::Distribution:
            return "Distribution Algorithm";
        case Stage::IPCTransmission:
            return "IPC Transmission";
        case Stage::Total:
            return "Total End-to-End";
        default:
            return "Unknown";
    }
}
