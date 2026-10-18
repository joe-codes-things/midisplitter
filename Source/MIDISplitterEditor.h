#pragma once

#include "MIDISplitterProcessor.h"
#include "UIColors.h"
#include "BuildVersion.h"
#include "CCFilterCheckbox.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

/**
 * @brief Futuristic neon-lit editor for MIDI Splitter Plugin
 *
 * 400x400 square window.
 * Top bar: gear icon (left), title (centre), Nine Lives Audio label (right, hover-lit).
 * Middle: voice slider + editable count + Voices label.
 * Bottom-left: Mod / Exp / Vol CC filter checkboxes.
 */
class MIDISplitterEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit MIDISplitterEditor(MIDISplitterProcessor& splitterProcessor);
    ~MIDISplitterEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    // ── Gear icon button ──────────────────────────────────────────────────
    /**
     * @brief Gear icon drawn with a toothed path. Dims at rest, brightens on hover.
     */
    class GearButton : public juce::Component {
    public:
        GearButton(juce::Colour normalColor, juce::Colour hoverColor)
            : normalColor_(normalColor), hoverColor_(hoverColor) {}

        void paint(juce::Graphics& g) override {
            auto c = isHovered_ ? hoverColor_ : normalColor_;
            auto b = getLocalBounds().toFloat().reduced(2.0f);
            float cx = b.getCentreX(), cy = b.getCentreY();
            float r  = juce::jmin(b.getWidth(), b.getHeight()) * 0.5f;

            g.setColour(c);

            float outerR = r, innerR = r * 0.55f;
            int teeth = 8;
            const float toothCount = static_cast<float>(teeth);
            juce::Path gear;
            for (int i = 0; i < teeth; ++i) {
                const float toothIndex = static_cast<float>(i);
                float a0 = juce::MathConstants<float>::twoPi * toothIndex / toothCount;
                float a1 = a0 + juce::MathConstants<float>::twoPi / toothCount * 0.4f;
                float a2 = a0 + juce::MathConstants<float>::twoPi / toothCount * 0.6f;
                float a3 = a0 + juce::MathConstants<float>::twoPi / toothCount;
                auto pt = [&](float angle, float rad) {
                    return juce::Point<float>(cx + rad * std::cos(angle),
                                              cy + rad * std::sin(angle));
                };
                if (i == 0) gear.startNewSubPath(pt(a0, innerR));
                gear.lineTo(pt(a0, outerR));
                gear.lineTo(pt(a1, outerR));
                gear.lineTo(pt(a2, outerR));
                gear.lineTo(pt(a3, innerR));
            }
            gear.closeSubPath();
            g.fillPath(gear);

            // Centre hole punched out
            g.setColour(isHovered_ ? UIColors::Surface3 : UIColors::Surface2);
            g.fillEllipse(cx - r * 0.28f, cy - r * 0.28f, r * 0.56f, r * 0.56f);
        }

        void mouseEnter(const juce::MouseEvent&) override { isHovered_ = true;  repaint(); }
        void mouseExit (const juce::MouseEvent&) override { isHovered_ = false; repaint(); }
        
        void mouseDown(const juce::MouseEvent&) override {
            if (onClick) onClick();
        }
        
        std::function<void()> onClick;

    private:
        juce::Colour normalColor_, hoverColor_;
        bool isHovered_ { false };
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GearButton)
    };

    // ── Hover label ───────────────────────────────────────────────────────
    /**
     * @brief Text label that dims at rest and brightens on hover,
     *        matching the gear icon's hover behaviour.
     */
    class HoverLabel : public juce::Component {
    public:
        HoverLabel(const juce::String& text,
                   float fontSize,
                   juce::Colour normalColor,
                   juce::Colour hoverColor,
                   juce::Justification justification = juce::Justification::centredRight)
            : text_(text), fontSize_(fontSize),
              normalColor_(normalColor), hoverColor_(hoverColor),
              justification_(justification) {}

        void paint(juce::Graphics& g) override {
            g.setColour(isHovered_ ? hoverColor_ : normalColor_);
            g.setFont(juce::Font(juce::FontOptions(fontSize_, juce::Font::plain)));
            g.drawText(text_, getLocalBounds(), justification_, true);
        }

        void mouseEnter(const juce::MouseEvent&) override { isHovered_ = true;  repaint(); }
        void mouseExit (const juce::MouseEvent&) override { isHovered_ = false; repaint(); }
        
        void mouseDown(const juce::MouseEvent&) override {
            if (onClick) onClick();
        }
        
        std::function<void()> onClick;

    private:
        juce::String text_;
        float fontSize_;
        juce::Colour normalColor_, hoverColor_;
        juce::Justification justification_;
        bool isHovered_ { false };
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HoverLabel)
    };

    // ── Plugin Info Popup Menu ────────────────────────────────────────────
    /**
     * @brief Popup menu that appears when gear icon is clicked.
     *        Shows "Plugin Info" title and menu items with hover effects.
     */
    class PluginInfoPopup : public juce::Component {
    public:
        PluginInfoPopup() {
            setSize(220, 90);
        }

        void paint(juce::Graphics& g) override {
            auto bounds = getLocalBounds().toFloat();
            
            // Background with subtle gradient
            auto gradient = juce::ColourGradient(
                UIColors::Surface2, 0, 0,
                UIColors::Surface3, 0, bounds.getBottom(), false);
            g.setGradientFill(gradient);
            g.fillRoundedRectangle(bounds, 4.0f);
            
            // Border
            g.setColour(UIColors::Border);
            g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
            
            // Title "Plugin Info" - bold, no hover
            g.setColour(UIColors::SecondaryText);
            g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
            g.drawText("Plugin Info", 12, 10, getWidth() - 24, 20, 
                      juce::Justification::centredLeft, true);
            
            // Menu items with hover effect (indented - 3 spaces worth)
            g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::plain)));
            
            // "About MIDI Splitter"
            g.setColour(aboutHovered_ ? UIColors::SecondaryText : UIColors::DimText);
            g.drawText("About MIDI Splitter", 18, 35, getWidth() - 30, 20,
                      juce::Justification::centredLeft, true);
            
            // "Setup Guide"
            g.setColour(setupHovered_ ? UIColors::SecondaryText : UIColors::DimText);
            g.drawText("Setup Guide", 18, 60, getWidth() - 30, 20,
                      juce::Justification::centredLeft, true);
        }

        void mouseMove(const juce::MouseEvent& e) override {
            bool wasAboutHovered = aboutHovered_;
            bool wasSetupHovered = setupHovered_;
            
            // Check if mouse is over "About MIDI Splitter" area
            aboutHovered_ = e.y >= 35 && e.y < 55;
            
            // Check if mouse is over "Setup Guide" area
            setupHovered_ = e.y >= 60 && e.y < 80;
            
            if (wasAboutHovered != aboutHovered_ || wasSetupHovered != setupHovered_) {
                repaint();
            }
        }

        void mouseExit(const juce::MouseEvent&) override {
            if (aboutHovered_ || setupHovered_) {
                aboutHovered_ = false;
                setupHovered_ = false;
                repaint();
            }
        }

        void mouseDown(const juce::MouseEvent& e) override {
            if (e.y >= 35 && e.y < 55) {
                // "About MIDI Splitter" clicked
                if (onAboutClicked) onAboutClicked();
            } else if (e.y >= 60 && e.y < 80) {
                // "Setup Guide" clicked
                if (onSetupGuideClicked) onSetupGuideClicked();
            }
        }
        
        std::function<void()> onAboutClicked;
        std::function<void()> onSetupGuideClicked;

    private:
        bool aboutHovered_ { false };
        bool setupHovered_ { false };
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginInfoPopup)
    };

    // ── About Popup ───────────────────────────────────────────────────────
    /**
     * @brief Centered popup showing plugin version and build information.
     */
    class AboutPopup : public juce::Component {
    public:
        AboutPopup(const juce::String& pluginFormat, 
                   const juce::String& architecture,
                   const juce::String& version,
                   const juce::String& buildDate) {
            setSize(250, 140);
            
            // Build the info text lines
            line1_ = pluginFormat + " " + architecture;
            line2_ = "Version " + version;
            line3_ = "Built on " + buildDate;
        }

        void paint(juce::Graphics& g) override {
            auto bounds = getLocalBounds().toFloat();
            
            // Background with subtle gradient
            auto gradient = juce::ColourGradient(
                UIColors::Surface2, 0, 0,
                UIColors::Surface3, 0, bounds.getBottom(), false);
            g.setGradientFill(gradient);
            g.fillRoundedRectangle(bounds, 4.0f);
            
            // Border
            g.setColour(UIColors::Border);
            g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
            
            // X button in top-right corner
            float xSize = 20.0f;
            float xPadding = 8.0f;
            juce::Rectangle<float> xBounds(bounds.getWidth() - xSize - xPadding, 
                                          xPadding, xSize, xSize);
            g.setColour(xHovered_ ? UIColors::SecondaryText : UIColors::DimText);
            float xInset = 6.0f;
            g.drawLine(xBounds.getX() + xInset, xBounds.getY() + xInset,
                      xBounds.getRight() - xInset, xBounds.getBottom() - xInset, 2.0f);
            g.drawLine(xBounds.getRight() - xInset, xBounds.getY() + xInset,
                      xBounds.getX() + xInset, xBounds.getBottom() - xInset, 2.0f);
            
            // Layout constants
            // Equal padding above title and below last info line.
            // Content = titleH + gap + (3 info lines)
            // padding = (totalH - contentH) / 2
            static constexpr int titleH    = 22;  // title row height
            static constexpr int titleGap  = 6;   // gap between title and first info line
            static constexpr int infoLineH = 18;  // height per info line
            static constexpr int infoLines = 3;
            int contentH = titleH + titleGap + infoLineH * infoLines;
            int padding  = (getHeight() - contentH) / 2;
            
            int titleY = padding;
            int infoY  = titleY + titleH + titleGap;
            
            // Title
            g.setColour(UIColors::PrimaryText);
            g.setFont(juce::Font(juce::FontOptions(16.0f, juce::Font::bold)));
            g.drawText("About MIDI Splitter", 20, titleY, getWidth() - 40, titleH,
                      juce::Justification::centred, true);
            
            // Info lines
            g.setColour(UIColors::SecondaryText);
            g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::plain)));
            g.drawText(line1_, 20, infoY,                    getWidth() - 40, infoLineH, juce::Justification::centred, true);
            g.drawText(line2_, 20, infoY + infoLineH,        getWidth() - 40, infoLineH, juce::Justification::centred, true);
            g.drawText(line3_, 20, infoY + infoLineH * 2,    getWidth() - 40, infoLineH, juce::Justification::centred, true);
        }

        void mouseMove(const juce::MouseEvent& e) override {
            float xSize = 20.0f;
            float xPadding = 8.0f;
            juce::Rectangle<float> xBounds(static_cast<float>(getWidth()) - xSize - xPadding,
                                          xPadding, xSize, xSize);
            bool wasHovered = xHovered_;
            xHovered_ = xBounds.contains(e.position);
            if (wasHovered != xHovered_) repaint();
        }

        void mouseExit(const juce::MouseEvent&) override {
            if (xHovered_) { xHovered_ = false; repaint(); }
        }

        void mouseDown(const juce::MouseEvent& e) override {
            float xSize = 20.0f;
            float xPadding = 8.0f;
            juce::Rectangle<float> xBounds(static_cast<float>(getWidth()) - xSize - xPadding,
                                          xPadding, xSize, xSize);
            if (xBounds.contains(e.position)) {
                if (onCloseClicked) onCloseClicked();
            }
        }
        
        std::function<void()> onCloseClicked;

    private:
        juce::String line1_, line2_, line3_;
        bool xHovered_ { false };
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AboutPopup)
    };

    // ── Nine Lives Audio Popup ────────────────────────────────────────────
    /**
     * @brief Small popup showing developer and copyright information.
     */
    class NLAPopup : public juce::Component {
    public:
        NLAPopup() {
            setSize(200, 50);  // Reduced height for 2 lines instead of 3
        }

        void paint(juce::Graphics& g) override {
            auto bounds = getLocalBounds().toFloat();
            
            // Background with subtle gradient
            auto gradient = juce::ColourGradient(
                UIColors::Surface2, 0, 0,
                UIColors::Surface3, 0, bounds.getBottom(), false);
            g.setGradientFill(gradient);
            g.fillRoundedRectangle(bounds, 4.0f);
            
            // Border
            g.setColour(UIColors::Border);
            g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
            
            // Text - vertically centered, 14pt font to match gear popup
            g.setColour(UIColors::SecondaryText);
            g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::plain)));
            
            // Calculate vertical centering for 2 lines of text
            int lineHeight = 18;
            int totalTextHeight = lineHeight * 2;
            int startY = (getHeight() - totalTextHeight) / 2;
            
            g.drawText("Developed by Joe Sanders", 10, startY, getWidth() - 20, lineHeight,
                      juce::Justification::centred, true);
            g.drawText(juce::CharPointer_UTF8("\xC2\xA9 2026 Nine Lives Audio"), 10, startY + lineHeight, getWidth() - 20, lineHeight,
                      juce::Justification::centred, true);
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NLAPopup)
    };

    // ── Private methods ───────────────────────────────────────────────────
    void initializeComponents();
    void handleVoiceCountChange(int newCount);
    void handleModFilterChange(bool enabled);
    void handleExpFilterChange(bool enabled);
    void handleVolFilterChange(bool enabled);
    void timerCallback() override;
    void togglePluginInfoPopup();
    void hidePluginInfoPopup();
    void showAboutPopup();
    void hideAboutPopup();
    void showNLAPopup();
    void hideNLAPopup();
    void mouseDown(const juce::MouseEvent& e) override;

    // ── Members ───────────────────────────────────────────────────────────
    MIDISplitterProcessor& processor_;

    std::unique_ptr<juce::Slider>     slider_;
    std::unique_ptr<juce::Label>      titleLabel_;
    std::unique_ptr<juce::Label>      voiceCountLabel_;
    std::unique_ptr<juce::Label>      voicesLabel_;
    std::unique_ptr<HoverLabel>       nlaLabel_;
    std::unique_ptr<CCFilterCheckbox> modCheckbox_;
    std::unique_ptr<CCFilterCheckbox> expCheckbox_;
    std::unique_ptr<CCFilterCheckbox> volCheckbox_;
    // Separate labels for each checkbox (positioned independently from the boxes)
    std::unique_ptr<juce::Label>      modLabel_;
    std::unique_ptr<juce::Label>      expLabel_;
    std::unique_ptr<juce::Label>      volLabel_;
    std::unique_ptr<GearButton>       gearButton_;
    std::unique_ptr<PluginInfoPopup>  pluginInfoPopup_;
    std::unique_ptr<AboutPopup>       aboutPopup_;
    std::unique_ptr<NLAPopup>         nlaPopup_;

    juce::Image cachedBackground_;

    // ── Layout constants ──────────────────────────────────────────────────
    static constexpr int WINDOW_WIDTH  = 400;
    static constexpr int WINDOW_HEIGHT = 400;
    static constexpr int MARGIN        = 16;
    static constexpr int HEADER_HEIGHT = 20;   // compact header row
    static constexpr int SLIDER_HEIGHT = 60;
    static constexpr int SLIDER_WIDTH  = 270;  // 90% of original 300px
    static constexpr int ICON_SIZE     = 18;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MIDISplitterEditor)
};

// ─── Implementation ───────────────────────────────────────────────────────────

inline MIDISplitterEditor::MIDISplitterEditor(MIDISplitterProcessor& splitterProcessor)
    : AudioProcessorEditor(&splitterProcessor), processor_(splitterProcessor)
{
    initializeComponents();
    setSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    startTimerHz(30);
}

inline MIDISplitterEditor::~MIDISplitterEditor() { stopTimer(); }

inline void MIDISplitterEditor::paint(juce::Graphics& g)
{
    g.fillAll(UIColors::DeepBackground);

    auto bounds = getLocalBounds().toFloat();
    auto gradient = juce::ColourGradient(
        UIColors::Surface1.withAlpha(0.3f), 0, 0,
        UIColors::DeepBackground, 0, bounds.getBottom(), false);
    g.setGradientFill(gradient);
    g.fillRect(bounds);

    g.setColour(UIColors::Border);
    g.drawRect(bounds, 1.0f);
}

inline void MIDISplitterEditor::resized()
{
    if (!titleLabel_ || !slider_ || !voiceCountLabel_ || !voicesLabel_)
        return;

    // ── Header row ────────────────────────────────────────────────────────
    // Gear icon — top-left
    gearButton_->setBounds(MARGIN, MARGIN, ICON_SIZE, ICON_SIZE);

    // Title — centred, vertically aligned with slider group
    titleLabel_->setBounds(0, MARGIN + HEADER_HEIGHT + 25, WINDOW_WIDTH, HEADER_HEIGHT + 4);

    // "Nine Lives Audio" — top-right, nudged up 3px relative to MARGIN
    {
        int nlaW = 110;
        int nlaH = 16;
        nlaLabel_->setBounds(WINDOW_WIDTH - MARGIN - nlaW,
                             MARGIN - 3,
                             nlaW, nlaH);
    }

    // ── Slider — sits 6px below the header row ────────────────────────────
    int sliderTop = MARGIN + HEADER_HEIGHT + 40;  // halfway between original and previous push
    slider_->setBounds(
        (WINDOW_WIDTH - SLIDER_WIDTH) / 2,
        sliderTop + (SLIDER_HEIGHT - 40) / 2,
        SLIDER_WIDTH, 40);

    // ── Voice count + Voices label — 4px below slider, shifted up 10px ────
    int y = sliderTop + SLIDER_HEIGHT - 21;

    voiceCountLabel_->setBounds((WINDOW_WIDTH - 60) / 2, y, 60, 20);
    y += 20;
    voicesLabel_->setBounds(0, y, WINDOW_WIDTH, 20);

    // ── CC filter checkboxes — boxes near bottom-left, labels at very bottom ─
    // Labels anchor to the bottom edge (MARGIN from bottom).
    // Boxes sit 15px above the labels with a gap between them.
    if (modCheckbox_ && expCheckbox_ && volCheckbox_) {
        static constexpr int LABEL_H    = 20;
        static constexpr int GAP        = 7;   // ~half of original 15px gap
        static constexpr int BOT_MARGIN = 16;

        int labelY = WINDOW_HEIGHT - BOT_MARGIN - LABEL_H;
        int boxY   = labelY - GAP - CCFilterCheckbox::BOX_SIZE;

        // Column x positions (centred within a 44px slot)
        int col0 = MARGIN + (44 - CCFilterCheckbox::BOX_SIZE) / 2;
        int col1 = MARGIN + 50 + (44 - CCFilterCheckbox::BOX_SIZE) / 2;
        int col2 = MARGIN + 100 + (44 - CCFilterCheckbox::BOX_SIZE) / 2;

        modCheckbox_->setBounds(col0, boxY, CCFilterCheckbox::BOX_SIZE, CCFilterCheckbox::BOX_SIZE);
        expCheckbox_->setBounds(col1, boxY, CCFilterCheckbox::BOX_SIZE, CCFilterCheckbox::BOX_SIZE);
        volCheckbox_->setBounds(col2, boxY, CCFilterCheckbox::BOX_SIZE, CCFilterCheckbox::BOX_SIZE);

        modLabel_->setBounds(MARGIN,       labelY, 44, LABEL_H);
        expLabel_->setBounds(MARGIN + 50,  labelY, 44, LABEL_H);
        volLabel_->setBounds(MARGIN + 100, labelY, 44, LABEL_H);
    }
}

inline void MIDISplitterEditor::initializeComponents()
{
    // ── Title ─────────────────────────────────────────────────────────────
    titleLabel_ = std::make_unique<juce::Label>("Title", "MIDI Splitter");
    titleLabel_->setFont(juce::Font(juce::FontOptions(20.0f, juce::Font::bold)));
    titleLabel_->setJustificationType(juce::Justification::centred);
    titleLabel_->setColour(juce::Label::textColourId, UIColors::PrimaryText);
    addAndMakeVisible(*titleLabel_);

    // ── Nine Lives Audio (hover-lit, same dim/bright pair as gear) ────────
    nlaLabel_ = std::make_unique<HoverLabel>(
        "Nine Lives Audio",
        12.0f,                    // 12pt — one step up from 11pt
        UIColors::DimText,        // dim at rest
        UIColors::SecondaryText,  // brightens on hover
        juce::Justification::centredRight);
    nlaLabel_->setMouseCursor(juce::MouseCursor::PointingHandCursor);
    nlaLabel_->onClick = [this]() {
        // Don't allow opening if About popup is visible
        if (aboutPopup_ && aboutPopup_->isVisible()) {
            return;
        }
        
        // Close gear popup if it's open
        if (pluginInfoPopup_ && pluginInfoPopup_->isVisible()) {
            hidePluginInfoPopup();
        }
        
        // Toggle NLA popup
        if (nlaPopup_ && nlaPopup_->isVisible()) {
            hideNLAPopup();
        } else {
            showNLAPopup();
        }
    };
    addAndMakeVisible(*nlaLabel_);

    // ── Gear icon ─────────────────────────────────────────────────────────
    gearButton_ = std::make_unique<GearButton>(UIColors::DimText, UIColors::SecondaryText);
    gearButton_->onClick = [this]() { 
        // Close NLA popup if it's open
        if (nlaPopup_ && nlaPopup_->isVisible()) {
            hideNLAPopup();
        }
        togglePluginInfoPopup(); 
    };
    addAndMakeVisible(*gearButton_);

    // ── Slider ────────────────────────────────────────────────────────────
    slider_ = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal,
                                              juce::Slider::NoTextBox);
    slider_->setRange(2.0, 16.0, 1.0);
    slider_->setColour(juce::Slider::trackColourId,      UIColors::Surface2);
    slider_->setColour(juce::Slider::backgroundColourId, UIColors::Surface3);
    slider_->setColour(juce::Slider::thumbColourId,      UIColors::NeonBlue);
    addAndMakeVisible(*slider_);

    // ── Voice count label (editable) ──────────────────────────────────────
    voiceCountLabel_ = std::make_unique<juce::Label>("VoiceCount", "4");
    voiceCountLabel_->setFont(juce::Font(juce::FontOptions(17.0f, juce::Font::plain)));
    voiceCountLabel_->setJustificationType(juce::Justification::centred);
    voiceCountLabel_->setColour(juce::Label::textColourId,            UIColors::PrimaryText);
    voiceCountLabel_->setColour(juce::Label::backgroundColourId,      juce::Colours::transparentBlack);
    voiceCountLabel_->setColour(juce::Label::outlineColourId,         juce::Colours::transparentBlack);
    voiceCountLabel_->setColour(juce::Label::textWhenEditingColourId, UIColors::PrimaryText);
    voiceCountLabel_->setColour(juce::TextEditor::backgroundColourId,     juce::Colours::transparentBlack);
    voiceCountLabel_->setColour(juce::TextEditor::outlineColourId,        juce::Colours::transparentBlack);
    voiceCountLabel_->setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    voiceCountLabel_->setEditable(true, true, false);
    voiceCountLabel_->setMinimumHorizontalScale(1.0f);
    voiceCountLabel_->onEditorShow = [this]() {
        if (auto* ed = voiceCountLabel_->getCurrentTextEditor()) {
            ed->setBorder(juce::BorderSize<int>(0));
            ed->setIndents(0, 0);
            ed->setJustification(juce::Justification::centred);
            ed->setFont(juce::Font(juce::FontOptions(17.0f, juce::Font::plain)));
            ed->moveCaretToEnd();
            ed->setHighlightedRegion({ed->getTotalNumChars(), ed->getTotalNumChars()});
        }
    };
    addAndMakeVisible(*voiceCountLabel_);

    // ── Voices label ──────────────────────────────────────────────────────
    voicesLabel_ = std::make_unique<juce::Label>("VoicesLabel", "Voices");
    voicesLabel_->setFont(juce::Font(juce::FontOptions(17.0f, juce::Font::plain)));
    voicesLabel_->setJustificationType(juce::Justification::centred);
    voicesLabel_->setColour(juce::Label::textColourId, UIColors::SecondaryText);
    addAndMakeVisible(*voicesLabel_);

    // ── CC filter checkboxes ──────────────────────────────────────────────
    // Logic color assignments:
    // - Mod: DeepAmber (#FF7A00) - stays the same
    // - Exp: BrightBlue (#2979FF) - stays the same  
    // - Vol: LogicYellow (#FFD600) - Logic's default automation yellow
    modCheckbox_ = std::make_unique<CCFilterCheckbox>(UIColors::DeepAmber);
    modCheckbox_->setStateChangeCallback([this](bool e){ handleModFilterChange(e); });
    addAndMakeVisible(*modCheckbox_);

    expCheckbox_ = std::make_unique<CCFilterCheckbox>(UIColors::BrightBlue);
    expCheckbox_->setStateChangeCallback([this](bool e){ handleExpFilterChange(e); });
    addAndMakeVisible(*expCheckbox_);

    volCheckbox_ = std::make_unique<CCFilterCheckbox>(UIColors::LogicYellow);
    volCheckbox_->setStateChangeCallback([this](bool e){ handleVolFilterChange(e); });
    addAndMakeVisible(*volCheckbox_);

    // ── Separate labels for each checkbox ────────────────────────────────
    auto makeCheckboxLabel = [this](const juce::String& text) {
        auto lbl = std::make_unique<juce::Label>("", text);
        lbl->setFont(juce::Font(juce::FontOptions(17.0f, juce::Font::plain)));
        lbl->setJustificationType(juce::Justification::centred);
        lbl->setColour(juce::Label::textColourId, UIColors::SecondaryText);
        addAndMakeVisible(*lbl);
        return lbl;
    };
    modLabel_ = makeCheckboxLabel("Mod");
    expLabel_ = makeCheckboxLabel("Exp");
    volLabel_ = makeCheckboxLabel("Vol");

    // ── Slider callback ───────────────────────────────────────────────────
    slider_->onValueChange = [this]() {
        int n = static_cast<int>(slider_->getValue());
        handleVoiceCountChange(n);
        voiceCountLabel_->setText(juce::String(n), juce::dontSendNotification);
    };

    // ── Voice count text callback ─────────────────────────────────────────
    voiceCountLabel_->onTextChange = [this]() {
        int v = voiceCountLabel_->getText().getIntValue();
        if (v >= 2 && v <= 16) {
            handleVoiceCountChange(v);
            slider_->setValue(v, juce::dontSendNotification);
        } else {
            voiceCountLabel_->setText(
                juce::String(static_cast<int>(slider_->getValue())),
                juce::dontSendNotification);
        }
    };

    // ── Initialise from processor state ───────────────────────────────────
    auto& ctrl = processor_.getPluginController();
    int vc = ctrl.getVoiceCount();
    slider_->setValue(vc, juce::dontSendNotification);
    voiceCountLabel_->setText(juce::String(vc), juce::dontSendNotification);

    modCheckbox_->setChecked(ctrl.isModEnabled(), juce::dontSendNotification);
    expCheckbox_->setChecked(ctrl.isExpEnabled(), juce::dontSendNotification);
    volCheckbox_->setChecked(ctrl.isVolEnabled(), juce::dontSendNotification);
}

inline void MIDISplitterEditor::handleVoiceCountChange(int newCount) {
    // Update automation parameter (this will trigger processBlock to sync)
    if (auto* param = processor_.getParameters()[1]) { // voiceCountParam is index 1 (after bypass)
        if (auto* intParam = dynamic_cast<juce::AudioParameterInt*>(param)) {
            *intParam = newCount;
        }
    }
    
    // Also update plugin controller for immediate UI feedback
    processor_.setVoiceCount(newCount);
}

inline void MIDISplitterEditor::handleModFilterChange(bool enabled) {
    // Update automation parameter
    if (auto* param = processor_.getParameters()[2]) { // modFilterParam is index 2
        if (auto* boolParam = dynamic_cast<juce::AudioParameterBool*>(param)) {
            *boolParam = enabled;
        }
    }
    
    // Also update plugin controller and pipeline for immediate effect
    processor_.getPluginController().setModEnabled(enabled);
    if (auto* s = processor_.getSenderPipeline()) s->setModEnabled(enabled);
}

inline void MIDISplitterEditor::handleExpFilterChange(bool enabled) {
    // Update automation parameter
    if (auto* param = processor_.getParameters()[3]) { // expFilterParam is index 3
        if (auto* boolParam = dynamic_cast<juce::AudioParameterBool*>(param)) {
            *boolParam = enabled;
        }
    }
    
    // Also update plugin controller and pipeline for immediate effect
    processor_.getPluginController().setExpEnabled(enabled);
    if (auto* s = processor_.getSenderPipeline()) s->setExpEnabled(enabled);
}

inline void MIDISplitterEditor::handleVolFilterChange(bool enabled) {
    // Update automation parameter
    if (auto* param = processor_.getParameters()[4]) { // volFilterParam is index 4
        if (auto* boolParam = dynamic_cast<juce::AudioParameterBool*>(param)) {
            *boolParam = enabled;
        }
    }
    
    // Also update plugin controller and pipeline for immediate effect
    processor_.getPluginController().setVolEnabled(enabled);
    if (auto* s = processor_.getSenderPipeline()) s->setVolEnabled(enabled);
}

inline void MIDISplitterEditor::timerCallback()
{
    // Read from automation parameters to ensure UI reflects automation changes
    auto& params = processor_.getParameters();
    
    // Get parameter values (Bypass=0, VoiceCount=1, Mod=2, Exp=3, Vol=4)
    int voiceCount = 4;
    bool modEnabled = true;
    bool expEnabled = true;
    bool volEnabled = true;
    
    if (auto* vcParam = dynamic_cast<juce::AudioParameterInt*>(params[1])) {
        voiceCount = vcParam->get();
    }
    if (auto* modParam = dynamic_cast<juce::AudioParameterBool*>(params[2])) {
        modEnabled = modParam->get();
    }
    if (auto* expParam = dynamic_cast<juce::AudioParameterBool*>(params[3])) {
        expEnabled = expParam->get();
    }
    if (auto* volParam = dynamic_cast<juce::AudioParameterBool*>(params[4])) {
        volEnabled = volParam->get();
    }

    // Update voice count UI if changed
    if (static_cast<int>(slider_->getValue()) != voiceCount) {
        slider_->setValue(voiceCount, juce::dontSendNotification);
        voiceCountLabel_->setText(juce::String(voiceCount), juce::dontSendNotification);
    }

    // Update CC filter checkboxes if changed
    if (modCheckbox_ && modCheckbox_->isChecked() != modEnabled)
        modCheckbox_->setChecked(modEnabled, juce::dontSendNotification);

    if (expCheckbox_ && expCheckbox_->isChecked() != expEnabled)
        expCheckbox_->setChecked(expEnabled, juce::dontSendNotification);

    if (volCheckbox_ && volCheckbox_->isChecked() != volEnabled)
        volCheckbox_->setChecked(volEnabled, juce::dontSendNotification);
}

inline void MIDISplitterEditor::togglePluginInfoPopup()
{
    // Don't allow opening gear menu if About popup is open
    if (aboutPopup_ && aboutPopup_->isVisible()) {
        return;
    }
    
    if (pluginInfoPopup_ && pluginInfoPopup_->isVisible()) {
        hidePluginInfoPopup();
    } else {
        // Create and show popup
        if (!pluginInfoPopup_) {
            pluginInfoPopup_ = std::make_unique<PluginInfoPopup>();
            pluginInfoPopup_->onAboutClicked = [this]() {
                hidePluginInfoPopup();
                showAboutPopup();
            };
            pluginInfoPopup_->onSetupGuideClicked = [this]() {
                hidePluginInfoPopup();
                // Open setup guide URL in default browser
                juce::URL setupGuideURL("https://github.com/joe-codes-things/midisplitter/blob/main/README.md");
                setupGuideURL.launchInDefaultBrowser();
            };
        }
        
        // Position in top-left with 10px spacing
        pluginInfoPopup_->setBounds(10, 10, 220, 90);
        addAndMakeVisible(*pluginInfoPopup_);
        pluginInfoPopup_->toFront(false);
    }
}

inline void MIDISplitterEditor::hidePluginInfoPopup()
{
    if (pluginInfoPopup_) {
        removeChildComponent(pluginInfoPopup_.get());
        pluginInfoPopup_->setVisible(false);
    }
}

inline void MIDISplitterEditor::showAboutPopup()
{
    // Determine plugin format dynamically based on the actual wrapper type
    juce::String pluginFormat = "Unknown";
    auto wrapperType = juce::PluginHostType::getPluginLoadedAs();
    
    switch (wrapperType) {
        case juce::AudioProcessor::wrapperType_VST3:
            pluginFormat = "VST3";
            break;
        case juce::AudioProcessor::wrapperType_AudioUnit:
        case juce::AudioProcessor::wrapperType_AudioUnitv3:
            pluginFormat = "AU";
            break;
        case juce::AudioProcessor::wrapperType_AAX:
            pluginFormat = "AAX";
            break;
        case juce::AudioProcessor::wrapperType_Standalone:
            pluginFormat = "Standalone";
            break;
        default:
            pluginFormat = "Unknown";
            break;
    }
    
    // Determine architecture
    juce::String architecture;
    #if JUCE_MAC
        #if JUCE_ARM
            architecture = "Apple Silicon";
        #else
            architecture = "Apple Intel";
        #endif
    #elif JUCE_WINDOWS
        architecture = "Windows";
    #else
        architecture = "Unknown";
    #endif
    
    // Get version from BuildVersion.h
    juce::String version = PLUGIN_FULL_VERSION;
    
    // Format build date from YYYYMMDD to "Jan 1st 2026"
    juce::String rawDate = PLUGIN_BUILD_DATE;  // e.g., "20260526"
    juce::String formattedDate;
    
    if (rawDate.length() == 8) {
        int year = rawDate.substring(0, 4).getIntValue();
        int month = rawDate.substring(4, 6).getIntValue();
        int day = rawDate.substring(6, 8).getIntValue();
        
        // Month names
        const char* monthNames[] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        
        // Day suffix (1st, 2nd, 3rd, 4th, etc.)
        juce::String daySuffix;
        if (day == 1 || day == 21 || day == 31) daySuffix = "st";
        else if (day == 2 || day == 22) daySuffix = "nd";
        else if (day == 3 || day == 23) daySuffix = "rd";
        else daySuffix = "th";
        
        formattedDate = juce::String(monthNames[month]) + " " + 
                       juce::String(day) + daySuffix + " " + 
                       juce::String(year);
    } else {
        formattedDate = rawDate;  // Fallback to raw date if parsing fails
    }
    
    aboutPopup_ = std::make_unique<AboutPopup>(pluginFormat, architecture, version, formattedDate);
    
    // Set up close button callback
    aboutPopup_->onCloseClicked = [this]() {
        hideAboutPopup();
    };
    
    // Center in window
    int x = (WINDOW_WIDTH - 250) / 2;
    int y = (WINDOW_HEIGHT - 140) / 2;
    aboutPopup_->setBounds(x, y, 250, 140);
    
    addAndMakeVisible(*aboutPopup_);
    aboutPopup_->toFront(false);
}

inline void MIDISplitterEditor::hideAboutPopup()
{
    if (aboutPopup_) {
        removeChildComponent(aboutPopup_.get());
        aboutPopup_->setVisible(false);
        aboutPopup_.reset();
    }
}

inline void MIDISplitterEditor::showNLAPopup()
{
    nlaPopup_ = std::make_unique<NLAPopup>();
    
    // Position in top-right with 10px spacing (matching gear icon's 10px spacing)
    int x = WINDOW_WIDTH - 10 - 200;  // 10px from right edge
    int y = 10;  // 10px from top edge
    nlaPopup_->setBounds(x, y, 200, 50);  // Updated height to 50
    
    addAndMakeVisible(*nlaPopup_);
    nlaPopup_->toFront(false);
}

inline void MIDISplitterEditor::hideNLAPopup()
{
    if (nlaPopup_) {
        removeChildComponent(nlaPopup_.get());
        nlaPopup_->setVisible(false);
        nlaPopup_.reset();
    }
}

inline void MIDISplitterEditor::mouseDown(const juce::MouseEvent& e)
{
    // If About popup is open, prevent other interactions
    if (aboutPopup_ && aboutPopup_->isVisible()) {
        auto aboutBounds = aboutPopup_->getBounds();
        if (!aboutBounds.contains(e.getPosition())) {
            hideAboutPopup();
        }
        return;  // Block all other interactions when About is open
    }
    
    // Close popups if clicking outside of them
    if (pluginInfoPopup_ && pluginInfoPopup_->isVisible()) {
        auto popupBounds = pluginInfoPopup_->getBounds();
        auto gearBounds = gearButton_->getBounds();
        
        if (!popupBounds.contains(e.getPosition()) && !gearBounds.contains(e.getPosition())) {
            hidePluginInfoPopup();
        }
    }
    
    if (nlaPopup_ && nlaPopup_->isVisible()) {
        auto nlaBounds = nlaPopup_->getBounds();
        auto nlaLabelBounds = nlaLabel_->getBounds();
        if (!nlaBounds.contains(e.getPosition()) && !nlaLabelBounds.contains(e.getPosition())) {
            hideNLAPopup();
        }
    }
}
