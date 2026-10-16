#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

/**
 * @brief UI component for selecting voice count in Sender mode
 * 
 * Provides a slider or dropdown interface for selecting the number of voices
 * to distribute MIDI events to (2-16 voices).
 */
class VoiceCountSelector : public juce::Component {
public:
    /**
     * @brief Voice count change callback function type
     * @param newCount The newly selected voice count
     */
    using VoiceCountChangeCallback = std::function<void(int newCount)>;
    
    /**
     * @brief Constructor
     * @param useSlider If true, uses slider; if false, uses dropdown
     */
    explicit VoiceCountSelector(bool useSlider = true);
    
    /**
     * @brief Destructor
     */
    ~VoiceCountSelector() override = default;
    
    /**
     * @brief Set current voice count
     * @param count Voice count to set (2-16)
     * @param sendNotification Whether to trigger callback
     */
    void setVoiceCount(int count, juce::NotificationType sendNotification = juce::sendNotificationAsync);
    
    /**
     * @brief Get current voice count
     * @return Current voice count
     */
    int getVoiceCount() const;
    
    /**
     * @brief Set voice count change callback
     * @param callback Function to call when voice count changes
     */
    void setVoiceCountChangeCallback(VoiceCountChangeCallback callback);
    
    /**
     * @brief Enable/disable the selector
     * @param enabled Whether the selector should be enabled
     */
    void setEnabled(bool enabled);
    
    /**
     * @brief Check if selector is enabled
     * @return true if enabled
     */
    bool isEnabled() const;
    
    /**
     * @brief Set tooltip text
     * @param tooltipText Tooltip to display
     */
    void setTooltip(const juce::String& tooltipText);
    
    /**
     * @brief Set whether to show voice count as text
     * @param showText Whether to display voice count text
     */
    void setShowVoiceCountText(bool showText);
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void lookAndFeelChanged() override;

private:
    /**
     * @brief Handle slider value change
     */
    void handleSliderChange();
    
    /**
     * @brief Handle dropdown selection change
     */
    void handleDropdownChange();
    
    /**
     * @brief Update voice count display text
     */
    void updateVoiceCountText();
    
    /**
     * @brief Validate voice count range
     * @param count Voice count to validate
     * @return Clamped voice count
     */
    int validateVoiceCount(int count) const;
    
    /**
     * @brief Get voice count description text
     * @param count Voice count
     * @return Description string
     */
    juce::String getVoiceCountDescription(int count) const;

private:
    // UI components
    std::unique_ptr<juce::Slider> slider_;
    std::unique_ptr<juce::ComboBox> dropdown_;
    std::unique_ptr<juce::Label> label_;
    std::unique_ptr<juce::Label> valueLabel_;
    
    // Configuration
    bool useSlider_;
    bool showVoiceCountText_;
    int currentVoiceCount_;
    
    // Callback
    VoiceCountChangeCallback voiceCountChangeCallback_;
    
    // Constants
    static constexpr int MIN_VOICE_COUNT = 2;
    static constexpr int MAX_VOICE_COUNT = 16;
    static constexpr int DEFAULT_VOICE_COUNT = 4;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoiceCountSelector)
};

// Implementation

inline VoiceCountSelector::VoiceCountSelector(bool useSlider)
    : useSlider_(useSlider)
    , showVoiceCountText_(true)
    , currentVoiceCount_(DEFAULT_VOICE_COUNT) {
    
    // Create label
    label_ = std::make_unique<juce::Label>("VoiceCountLabel", "Voices:");
    label_->setFont(juce::Font(14.0f, juce::Font::bold));
    label_->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(*label_);
    
    if (useSlider_) {
        // Create slider
        slider_ = std::make_unique<juce::Slider>("VoiceCountSlider");
        slider_->setSliderStyle(juce::Slider::LinearHorizontal);
        slider_->setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 20);
        slider_->setRange(MIN_VOICE_COUNT, MAX_VOICE_COUNT, 1.0);
        slider_->setValue(DEFAULT_VOICE_COUNT, juce::dontSendNotification);
        slider_->onValueChange = [this] { handleSliderChange(); };
        addAndMakeVisible(*slider_);
        
        // Set tooltip
        slider_->setTooltip("Number of voices to distribute MIDI events to (2-16)");
        
    } else {
        // Create dropdown
        dropdown_ = std::make_unique<juce::ComboBox>("VoiceCountDropdown");
        for (int i = MIN_VOICE_COUNT; i <= MAX_VOICE_COUNT; ++i) {
            dropdown_->addItem(juce::String(i) + " voices", i);
        }
        dropdown_->setSelectedId(DEFAULT_VOICE_COUNT, juce::dontSendNotification);
        dropdown_->onChange = [this] { handleDropdownChange(); };
        addAndMakeVisible(*dropdown_);
        
        // Set tooltip
        dropdown_->setTooltip("Select number of voices to distribute MIDI events to");
    }
    
    // Create value label
    if (showVoiceCountText_) {
        valueLabel_ = std::make_unique<juce::Label>("VoiceCountValue", "");
        valueLabel_->setFont(juce::Font(12.0f));
        valueLabel_->setJustificationType(juce::Justification::centred);
        valueLabel_->setColour(juce::Label::textColourId, juce::Colours::darkgrey);
        valueLabel_->setVisible(false); // Hide the label
        addAndMakeVisible(*valueLabel_);
        updateVoiceCountText();
    }
}

inline void VoiceCountSelector::setVoiceCount(int count, juce::NotificationType sendNotification) {
    int validatedCount = validateVoiceCount(count);
    
    if (currentVoiceCount_ == validatedCount) {
        return;
    }
    
    currentVoiceCount_ = validatedCount;
    
    if (useSlider_ && slider_) {
        slider_->setValue(validatedCount, juce::dontSendNotification);
    } else if (!useSlider_ && dropdown_) {
        dropdown_->setSelectedId(validatedCount, juce::dontSendNotification);
    }
    
    updateVoiceCountText();
    
    if (sendNotification != juce::dontSendNotification && voiceCountChangeCallback_) {
        if (sendNotification == juce::sendNotificationAsync) {
            juce::MessageManager::callAsync([this, validatedCount] {
                if (voiceCountChangeCallback_) {
                    voiceCountChangeCallback_(validatedCount);
                }
            });
        } else {
            voiceCountChangeCallback_(validatedCount);
        }
    }
}

inline int VoiceCountSelector::getVoiceCount() const {
    return currentVoiceCount_;
}

inline void VoiceCountSelector::setVoiceCountChangeCallback(VoiceCountChangeCallback callback) {
    voiceCountChangeCallback_ = callback;
}

inline void VoiceCountSelector::setEnabled(bool enabled) {
    Component::setEnabled(enabled);
    
    if (slider_) {
        slider_->setEnabled(enabled);
    }
    
    if (dropdown_) {
        dropdown_->setEnabled(enabled);
    }
    
    if (label_) {
        label_->setEnabled(enabled);
    }
    
    if (valueLabel_) {
        valueLabel_->setEnabled(enabled);
    }
}

inline bool VoiceCountSelector::isEnabled() const {
    return Component::isEnabled();
}

inline void VoiceCountSelector::setTooltip(const juce::String& tooltipText) {
    if (slider_) {
        slider_->setTooltip(tooltipText);
    }
    
    if (dropdown_) {
        dropdown_->setTooltip(tooltipText);
    }
}

inline void VoiceCountSelector::setShowVoiceCountText(bool showText) {
    if (showVoiceCountText_ == showText) {
        return;
    }
    
    showVoiceCountText_ = showText;
    
    if (showText && !valueLabel_) {
        valueLabel_ = std::make_unique<juce::Label>("VoiceCountValue", "");
        valueLabel_->setFont(juce::Font(12.0f));
        valueLabel_->setJustificationType(juce::Justification::centred);
        valueLabel_->setColour(juce::Label::textColourId, juce::Colours::darkgrey);
        valueLabel_->setVisible(false); // Hide the label
        addAndMakeVisible(*valueLabel_);
        updateVoiceCountText();
    } else if (!showText && valueLabel_) {
        valueLabel_.reset();
    }
    
    resized();
}

inline void VoiceCountSelector::paint(juce::Graphics& g) {
    // No background or border - clean minimal look
}

inline void VoiceCountSelector::resized() {
    auto bounds = getLocalBounds();
    bounds.reduce(4, 4);
    
    // Layout label
    auto labelBounds = bounds.removeFromTop(20);
    auto labelWidth = labelBounds.removeFromLeft(60);
    label_->setBounds(labelWidth);
    
    // Value label is hidden, no need to layout
    
    bounds.removeFromTop(4); // Spacing
    
    // Layout control
    if (useSlider_ && slider_) {
        slider_->setBounds(bounds);
    } else if (!useSlider_ && dropdown_) {
        dropdown_->setBounds(bounds);
    }
}

inline void VoiceCountSelector::lookAndFeelChanged() {
    Component::lookAndFeelChanged();
    repaint();
}

inline void VoiceCountSelector::handleSliderChange() {
    if (!slider_) return;
    
    int newCount = static_cast<int>(slider_->getValue());
    setVoiceCount(newCount, juce::sendNotificationSync);
}

inline void VoiceCountSelector::handleDropdownChange() {
    if (!dropdown_) return;
    
    int selectedId = dropdown_->getSelectedId();
    if (selectedId >= MIN_VOICE_COUNT && selectedId <= MAX_VOICE_COUNT) {
        setVoiceCount(selectedId, juce::sendNotificationSync);
    }
}

inline void VoiceCountSelector::updateVoiceCountText() {
    if (valueLabel_) {
        valueLabel_->setText(getVoiceCountDescription(currentVoiceCount_), juce::dontSendNotification);
    }
    
    repaint(); // Repaint to update voice visualization
}

inline int VoiceCountSelector::validateVoiceCount(int count) const {
    return juce::jlimit(MIN_VOICE_COUNT, MAX_VOICE_COUNT, count);
}

inline juce::String VoiceCountSelector::getVoiceCountDescription(int count) const {
    return juce::String(count) + " voices";
}
