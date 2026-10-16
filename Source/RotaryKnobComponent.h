#pragma once

#include "UIColors.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

/**
 * @brief Rotary knob component with neon-lit futuristic aesthetic
 * 
 * Features:
 * - Procedurally drawn circular knob
 * - Warm spotlight gradient effect
 * - Neon blue indicator line
 * - Mouse drag and wheel support
 * - Numeric value display
 */
class RotaryKnobComponent : public juce::Component {
public:
    using ValueChangeCallback = std::function<void(int newValue)>;
    
    RotaryKnobComponent()
        : currentValue_(4)
        , minValue_(2)
        , maxValue_(16)
        , isDragging_(false)
        , dragStartY_(0)
        , dragStartValue_(0)
    {
        setSize(160, 180);  // Proper aspect ratio: square knob + text below
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }
    
    void setValue(int value, bool sendNotification = true) {
        int newValue = juce::jlimit(minValue_, maxValue_, value);
        if (currentValue_ != newValue) {
            currentValue_ = newValue;
            repaint();
            
            if (sendNotification && valueChangeCallback_) {
                valueChangeCallback_(currentValue_);
            }
        }
    }
    
    int getValue() const { return currentValue_; }
    
    void setValueChangeCallback(ValueChangeCallback callback) {
        valueChangeCallback_ = callback;
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Reserve space for text at bottom
        auto textArea = bounds.removeFromBottom(40);
        
        // Knob area - keep it square for proper aspect ratio
        auto knobArea = bounds;
        auto knobBounds = knobArea.reduced(20);  // Room for gradient
        auto knobCenter = knobBounds.getCentre();
        auto knobRadius = juce::jmin(knobBounds.getWidth(), knobBounds.getHeight()) / 2.0f;
        
        // Make knob perfectly circular
        auto circularBounds = juce::Rectangle<float>(
            knobCenter.x - knobRadius,
            knobCenter.y - knobRadius,
            knobRadius * 2,
            knobRadius * 2
        );
        
        // Draw spotlight gradient (warm stage lighting) - now with ample room
        drawSpotlight(g, knobCenter, knobRadius * 1.5f);
        
        // Draw knob base (dark surface)
        g.setColour(UIColors::Surface3);
        g.fillEllipse(circularBounds);
        
        // Draw knob border
        g.setColour(UIColors::Border);
        g.drawEllipse(circularBounds, 2.0f);
        
        // Draw indicator line (neon blue)
        float angle = valueToAngle(currentValue_);
        float indicatorLength = knobRadius * 0.7f;
        juce::Point<float> indicatorEnd(
            knobCenter.x + std::cos(angle) * indicatorLength,
            knobCenter.y + std::sin(angle) * indicatorLength
        );
        
        // Glow effect for indicator
        g.setColour(UIColors::NeonBlue.withAlpha(0.3f));
        g.drawLine(knobCenter.x, knobCenter.y, indicatorEnd.x, indicatorEnd.y, 6.0f);
        
        g.setColour(UIColors::NeonBlue);
        g.drawLine(knobCenter.x, knobCenter.y, indicatorEnd.x, indicatorEnd.y, 3.0f);
        
        // Draw center dot
        g.setColour(UIColors::WarmWhite);
        g.fillEllipse(knobCenter.x - 4, knobCenter.y - 4, 8, 8);
        
        // Draw value text below knob
        g.setColour(UIColors::PrimaryText);
        g.setFont(juce::Font(24.0f, juce::Font::bold));
        g.drawText(juce::String(currentValue_), textArea.removeFromTop(28), juce::Justification::centred);
        
        // Draw "Voices" label
        g.setColour(UIColors::SecondaryText);
        g.setFont(juce::Font(12.0f));
        g.drawText("Voices", textArea, juce::Justification::centred);
    }
    
    void mouseDown(const juce::MouseEvent& event) override {
        if (event.mods.isLeftButtonDown()) {
            isDragging_ = true;
            dragStartY_ = event.y;
            dragStartValue_ = currentValue_;
        }
        
        // Double-click to reset to default
        if (event.getNumberOfClicks() == 2) {
            setValue(4);
        }
    }
    
    void mouseDrag(const juce::MouseEvent& event) override {
        if (isDragging_) {
            float dragDistance = dragStartY_ - event.y;
            float sensitivity = 0.05f;
            int valueChange = static_cast<int>(dragDistance * sensitivity);
            setValue(dragStartValue_ + valueChange);
        }
    }
    
    void mouseUp(const juce::MouseEvent& event) override {
        isDragging_ = false;
    }
    
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override {
        int delta = wheel.deltaY > 0 ? 1 : -1;
        setValue(currentValue_ + delta);
    }
    
    void mouseEnter(const juce::MouseEvent& event) override {
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        repaint();
    }
    
    void mouseExit(const juce::MouseEvent& event) override {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
        repaint();
    }

private:
    void drawSpotlight(juce::Graphics& g, juce::Point<float> center, float radius) {
        // Warm spotlight gradient (stage lighting effect)
        auto gradient = juce::ColourGradient(
            UIColors::WarmWhite.withAlpha(0.15f), center.x, center.y,
            UIColors::Amber.withAlpha(0.0f), center.x, center.y + radius,
            true
        );
        
        g.setGradientFill(gradient);
        g.fillEllipse(center.x - radius, center.y - radius, radius * 2, radius * 2);
    }
    
    float valueToAngle(int value) const {
        // Map value to angle (-135° to +135°, 270° total range)
        float normalizedValue = static_cast<float>(value - minValue_) / static_cast<float>(maxValue_ - minValue_);
        float startAngle = -2.356f; // -135° in radians
        float endAngle = 2.356f;    // +135° in radians
        return startAngle + normalizedValue * (endAngle - startAngle);
    }
    
    int currentValue_;
    int minValue_;
    int maxValue_;
    bool isDragging_;
    int dragStartY_;
    int dragStartValue_;
    ValueChangeCallback valueChangeCallback_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RotaryKnobComponent)
};

