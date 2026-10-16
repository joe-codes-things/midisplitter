#pragma once

#include "UIColors.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * @brief Single LED-style voice activity indicator
 * 
 * Features:
 * - Circular LED design with neon glow
 * - Active/inactive states
 * - Voice number label
 * - Soft, diffused, atmospheric glow effect
 */
class VoiceActivityIndicator : public juce::Component {
public:
    VoiceActivityIndicator(int voiceNumber)
        : voiceNumber_(voiceNumber)
        , isActive_(false)
        , isEnabled_(true)
    {
        setSize(24, 24);
    }
    
    void setActive(bool active) {
        if (isActive_ != active) {
            isActive_ = active;
            repaint();
        }
    }
    
    bool isActive() const { return isActive_; }
    
    void setEnabled(bool enabled) {
        if (isEnabled_ != enabled) {
            isEnabled_ = enabled;
            repaint();
        }
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        auto ledBounds = bounds.reduced(4);
        auto center = ledBounds.getCentre();
        auto radius = ledBounds.getWidth() / 2.0f;
        
        // Get color based on state
        juce::Colour ledColor = UIColors::getVoiceActivityColor(isActive_, isEnabled_);
        
        // Draw glow effect (only when active)
        if (isActive_ && isEnabled_) {
            // Outer glow (soft, diffused)
            g.setColour(ledColor.withAlpha(0.2f));
            g.fillEllipse(center.x - radius * 1.5f, center.y - radius * 1.5f, 
                         radius * 3.0f, radius * 3.0f);
            
            // Inner glow (brighter)
            g.setColour(ledColor.withAlpha(0.4f));
            g.fillEllipse(center.x - radius * 1.2f, center.y - radius * 1.2f, 
                         radius * 2.4f, radius * 2.4f);
        }
        
        // Draw LED circle
        g.setColour(ledColor);
        g.fillEllipse(ledBounds);
        
        // Draw border
        g.setColour(UIColors::Border);
        g.drawEllipse(ledBounds, 1.0f);
        
        // Draw voice number
        g.setColour(isActive_ ? UIColors::DeepBackground : UIColors::SecondaryText);
        g.setFont(juce::Font(10.0f, juce::Font::bold));
        g.drawText(juce::String(voiceNumber_), ledBounds, juce::Justification::centred);
    }

private:
    int voiceNumber_;
    bool isActive_;
    bool isEnabled_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoiceActivityIndicator)
};

