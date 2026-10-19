#pragma once

#include "VoiceActivityIndicator.h"
#include "UIColors.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <atomic>

/**
 * @brief Panel containing 16 voice activity indicators in a 4×4 grid
 * 
 * Features:
 * - Real-time voice activity visualization
 * - 60 FPS update rate
 * - Efficient dirty rectangle repainting
 */
class VoiceActivityPanel : public juce::Component, private juce::Timer {
public:
    VoiceActivityPanel()
        : activeVoiceCount_(0)
    {
        // Create 16 indicators in a 4×4 grid
        for (int i = 0; i < 16; ++i) {
            auto indicator = std::make_unique<VoiceActivityIndicator>(i + 1);
            indicators_[i] = indicator.get();
            addAndMakeVisible(indicator.get());
            indicatorComponents_.push_back(std::move(indicator));
        }
        
        // Start timer for 60 FPS updates
        startTimerHz(60);
        
        setSize(200, 140);
    }
    
    ~VoiceActivityPanel() override {
        stopTimer();
    }
    
    void setVoiceActive(int voiceNumber, bool active) {
        if (voiceNumber >= 1 && voiceNumber <= 16) {
            voiceActivityStates_[voiceNumber - 1].store(active, std::memory_order_relaxed);
        }
    }
    
    void setActiveVoiceCount(int count) {
        activeVoiceCount_.store(count, std::memory_order_relaxed);
        
        // Enable/disable indicators based on voice count
        for (int i = 0; i < 16; ++i) {
            bool enabled = (i < count);
            if (indicators_[i]) {
                indicators_[i]->setEnabled(enabled);
            }
        }
    }
    
    void paint(juce::Graphics& g) override {
        // Draw title
        auto bounds = getLocalBounds();
        auto titleBounds = bounds.removeFromTop(20);
        
        g.setColour(UIColors::SecondaryText);
        g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
        g.drawText("Voice Activity", titleBounds, juce::Justification::centredLeft);
    }
    
    void resized() override {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(24); // Space for title
        
        // Layout 16 indicators in 4×4 grid
        int indicatorSize = 28;
        int spacing = 4;
        int gridWidth = (indicatorSize + spacing) * 4 - spacing;
        int gridHeight = (indicatorSize + spacing) * 4 - spacing;
        
        auto gridBounds = bounds.withSizeKeepingCentre(gridWidth, gridHeight);
        
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                int index = row * 4 + col;
                int x = gridBounds.getX() + col * (indicatorSize + spacing);
                int y = gridBounds.getY() + row * (indicatorSize + spacing);
                
                if (indicators_[index]) {
                    indicators_[index]->setBounds(x, y, indicatorSize, indicatorSize);
                }
            }
        }
    }

private:
    void timerCallback() override {
        // Update indicators based on atomic state (thread-safe)
        for (int i = 0; i < 16; ++i) {
            bool active = voiceActivityStates_[i].load(std::memory_order_relaxed);
            if (indicators_[i] && indicators_[i]->isActive() != active) {
                indicators_[i]->setActive(active);
            }
        }
    }
    
    std::array<VoiceActivityIndicator*, 16> indicators_;
    std::vector<std::unique_ptr<VoiceActivityIndicator>> indicatorComponents_;
    std::array<std::atomic<bool>, 16> voiceActivityStates_{};
    std::atomic<int> activeVoiceCount_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoiceActivityPanel)
};
