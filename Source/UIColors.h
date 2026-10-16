#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * @brief Color palette for MIDI Splitter plugin UI
 * 
 * Futuristic neon-lit aesthetic with synthwave-inspired design language.
 * Combines neon cyberpunk elements with professional audio software conventions.
 */
namespace UIColors {
    // ========================================================================
    // Background System - Layered dark surfaces creating depth
    // ========================================================================
    
    const juce::Colour DeepBackground(0x0A, 0x0E, 0x14);  // #0A0E14
    const juce::Colour Surface1(0x11, 0x16, 0x1D);         // #11161D
    const juce::Colour Surface2(0x18, 0x1E, 0x29);         // #181E29
    const juce::Colour Surface3(0x20, 0x26, 0x34);         // #202634
    const juce::Colour Border(0x2A, 0x33, 0x42);           // #2A3342
    
    // ========================================================================
    // Primary Neon Palette - Core neon colors for active elements
    // ========================================================================
    
    const juce::Colour NeonBlue(0x00, 0xE6, 0xFF);         // #00E6FF - Active controls, meters
    const juce::Colour Cyan(0x00, 0xFF, 0xC6);             // #00FFC6 - Modulation, secondary highlights
    const juce::Colour NeonGreen(0x39, 0xFF, 0x14);        // #39FF14 - Peaks, attention (use sparingly)
    const juce::Colour NeonPurple(0xB3, 0x00, 0xFF);       // #B300FF - FX sections, spatial tools
    const juce::Colour NeonMagenta(0xFF, 0x00, 0xF0);      // #FF00F0 - Accent highlights (use sparingly)
    
    // ========================================================================
    // Secondary Accent Palette - Lower-priority accents and gradients
    // ========================================================================
    
    const juce::Colour DeepBlue(0x1B, 0x2B, 0xFF);         // #1B2BFF
    const juce::Colour BrightBlue(0x29, 0x79, 0xFF);       // #2979FF
    const juce::Colour Teal(0x00, 0xB8, 0xA9);             // #00B8A9
    const juce::Colour LimeGreen(0xA8, 0xFF, 0x00);        // #A8FF00
    const juce::Colour Violet(0x7A, 0x00, 0xFF);           // #7A00FF
    const juce::Colour PinkPurple(0xFF, 0x2D, 0xFF);       // #FF2DFF
    
    // ========================================================================
    // Spotlight Palette - Warm stage-style lighting for important controls
    // ========================================================================
    
    const juce::Colour WarmWhite(0xFF, 0xDC, 0xA8);        // #FFDCA8 - Soft bulb center
    const juce::Colour Amber(0xFF, 0xB8, 0x4D);            // #FFB84D - Hover/focus glow
    const juce::Colour Gold(0xFF, 0xC2, 0x3C);             // #FFC23C - Premium highlights
    const juce::Colour DeepAmber(0xFF, 0x7A, 0x00);        // #FF7A00 - Strong emphasis
    
    // ========================================================================
    // Typography - Text color hierarchy
    // ========================================================================
    
    const juce::Colour PrimaryText(0xE6, 0xF1, 0xFF);      // #E6F1FF - Main text
    const juce::Colour SecondaryText(0xA6, 0xB3, 0xC7);    // #A6B3C7 - Secondary text
    const juce::Colour DimText(0x6B, 0x77, 0x8C);          // #6B778C - Dim text
    const juce::Colour DisabledText(0x3B, 0x44, 0x52);     // #3B4452 - Disabled text
    
    // ========================================================================
    // Helper Functions
    // ========================================================================
    
    /**
     * @brief Create a vertical gradient from top to bottom
     */
    inline juce::ColourGradient createVerticalGradient(
        juce::Colour topColor,
        juce::Colour bottomColor,
        juce::Rectangle<float> bounds)
    {
        return juce::ColourGradient(
            topColor, 0.0f, bounds.getY(),
            bottomColor, 0.0f, bounds.getBottom(),
            false
        );
    }
    
    /**
     * @brief Create a radial gradient from center outward
     */
    inline juce::ColourGradient createRadialGradient(
        juce::Colour centerColor,
        juce::Colour edgeColor,
        juce::Point<float> center,
        float radius)
    {
        return juce::ColourGradient(
            centerColor, center.x, center.y,
            edgeColor, center.x + radius, center.y,
            true
        );
    }
    
    /**
     * @brief Apply soft neon glow effect to a color
     */
    inline juce::Colour withNeonGlow(juce::Colour baseColor, float intensity = 1.0f) {
        return baseColor.withAlpha(juce::jlimit(0.0f, 1.0f, 0.8f * intensity));
    }
    
    /**
     * @brief Get color for voice activity indicator based on state
     */
    inline juce::Colour getVoiceActivityColor(bool isActive, bool isEnabled) {
        if (!isEnabled) {
            return DisabledText;
        }
        return isActive ? NeonBlue : Surface2;
    }
}

