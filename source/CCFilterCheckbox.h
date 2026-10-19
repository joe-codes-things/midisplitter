#pragma once

#include "UIColors.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

/**
 * @brief Neon-styled checkbox for CC filtering — box only, no label.
 *
 * The label is managed separately by the editor so box and text can be
 * positioned independently with arbitrary spacing between them.
 *
 * The box glows with the assigned color when checked, dims when unchecked.
 * No checkmark icon — color state alone indicates enabled/disabled.
 * No hover highlighting.
 */
class CCFilterCheckbox : public juce::Component {
public:
    using StateChangeCallback = std::function<void(bool isChecked)>;

    // Box dimensions (public so editor can use them for layout)
    static constexpr int BOX_SIZE = 16;

    /**
     * @param glowColor  Color used when the checkbox is checked
     */
    explicit CCFilterCheckbox(juce::Colour glowColor);
    ~CCFilterCheckbox() override = default;

    void setChecked(bool checked,
                    juce::NotificationType sendNotification = juce::sendNotificationAsync);
    bool isChecked() const;
    void setStateChangeCallback(StateChangeCallback callback);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;

private:
    void toggleState();

    bool isChecked_ { true };
    juce::Colour glowColor_;
    StateChangeCallback stateChangeCallback_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CCFilterCheckbox)
};

// ─── Implementation ──────────────────────────────────────────────────────────

inline CCFilterCheckbox::CCFilterCheckbox(juce::Colour glowColor)
    : glowColor_(glowColor)
{
    setSize(BOX_SIZE, BOX_SIZE);
}

inline void CCFilterCheckbox::setChecked(bool checked, juce::NotificationType sendNotification)
{
    if (isChecked_ == checked)
        return;

    isChecked_ = checked;
    repaint();

    if (sendNotification != juce::dontSendNotification && stateChangeCallback_) {
        if (sendNotification == juce::sendNotificationAsync) {
            juce::MessageManager::callAsync([this, checked] {
                if (stateChangeCallback_) stateChangeCallback_(checked);
            });
        } else {
            stateChangeCallback_(checked);
        }
    }
}

inline bool CCFilterCheckbox::isChecked() const { return isChecked_; }

inline void CCFilterCheckbox::setStateChangeCallback(StateChangeCallback callback)
{
    stateChangeCallback_ = callback;
}

inline void CCFilterCheckbox::paint(juce::Graphics& g)
{
    auto boxBounds = getLocalBounds().toFloat();

    if (isChecked_) {
        // Soft halo
        g.setColour(glowColor_.withAlpha(0.2f));
        g.fillRoundedRectangle(boxBounds.expanded(2.0f), 3.0f);
        // Solid fill
        g.setColour(glowColor_);
        g.fillRoundedRectangle(boxBounds, 3.0f);
        // Bright border
        g.setColour(glowColor_.brighter(0.3f));
        g.drawRoundedRectangle(boxBounds, 3.0f, 1.5f);
    } else {
        g.setColour(UIColors::Surface2);
        g.fillRoundedRectangle(boxBounds, 3.0f);
        g.setColour(UIColors::Border);
        g.drawRoundedRectangle(boxBounds, 3.0f, 1.0f);
    }
}

inline void CCFilterCheckbox::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isLeftButtonDown())
        toggleState();
}

inline void CCFilterCheckbox::toggleState()
{
    setChecked(!isChecked_, juce::sendNotificationSync);
}
