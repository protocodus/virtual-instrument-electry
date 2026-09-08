#pragma once

#include <JuceHeader.h>

#include "PluginProcessor.h"

#include <array>
#include <functional>
#include <memory>
#include <vector>

class ElectryLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    ElectryLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool isHighlighted, bool isDown) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&,
                         bool isHighlighted, bool isDown) override;
    void drawLabel (juce::Graphics&, juce::Label&) override;
    void drawComboBox (juce::Graphics&, int width, int height,
                       bool isButtonDown, int buttonX, int buttonY,
                       int buttonW, int buttonH, juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    std::unique_ptr<juce::FocusOutline> createFocusOutlineForComponent (
        juce::Component&) override;
};

// JUCE's text button activates on Return but not Space, and its radio buttons
// do not navigate with arrows. Electry advertises these controls as keyboard-
// focusable, so add those conventional keyboard paths without duplicating the
// buttons' existing click actions.
class ElectryTextButton final : public juce::TextButton
{
public:
    using juce::TextButton::TextButton;

    std::function<bool (const juce::KeyPress&)> onNavigation;
    void paintButton (juce::Graphics&, bool isHighlighted, bool isDown) override;
    bool keyPressed (const juce::KeyPress&) override;
};

// A guitar-oriented keyboard for keyswitches and the pitched playable range.
// Pick style stays latched; the play-style highlight follows either its latch
// or the active HOLD override.
class ElectryKeyboardComponent final : public juce::MidiKeyboardComponent
{
public:
    explicit ElectryKeyboardComponent (juce::MidiKeyboardState&);

    void setSelectedKeyswitches (int pickIndex, int styleIndex);
    void setSoloStringMask (std::uint8_t mask);

    void drawWhiteNote (int midiNoteNumber, juce::Graphics&,
                        juce::Rectangle<float> area, bool isDown, bool isOver,
                        juce::Colour lineColour, juce::Colour textColour) override;
    void drawBlackNote (int midiNoteNumber, juce::Graphics&,
                        juce::Rectangle<float> area, bool isDown, bool isOver,
                        juce::Colour noteFillColour) override;
    juce::String getWhiteNoteText (int midiNoteNumber) override;

private:
    bool isKeyswitchSelected (int keyswitchIndex) const noexcept;
    bool isSoloStringSelected (int stringIndex) const noexcept;

    int selectedPickIndex = 0;
    int selectedStyleIndex = 0;
    std::uint8_t activeSoloMask = 0;
};

// A titled row of exclusive buttons, used for the pickup selector and the
// play-style (keyswitch) strip.
class ElectryChoiceStrip final : public juce::Component
{
public:
    ElectryChoiceStrip (juce::String title, juce::StringArray choices,
                        int maximumColumns = 8,
                        juce::String accessibilityTitle = {});

    std::function<void (int)> onChoice;
    void setSelectedIndex (int newIndex);
    void setTooltipText (const juce::String& text);
    int getSelectedIndex() const noexcept { return selectedIndex; }

    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void activateChoice (int index);

    juce::String titleText;
    std::vector<std::unique_ptr<ElectryTextButton>> buttons;
    int maxColumns;
    int selectedIndex = 0;
};

class ElectryKnob final : public juce::Component
{
public:
    explicit ElectryKnob (juce::String name);
    void parentHierarchyChanged() override;
    void resized() override;

    juce::Slider slider;

private:
    juce::Label label;
};

class ElectryStatusDisplay final : public juce::Component
{
public:
    void setStatus (int activeVoices, int sympatheticStrings, bool ready,
                    double sampleRate, int midiMutePressure, int vibratoGesture,
                    int tremoloGesture, bool scheduleRepaint = true);
    juce::String getStatusText() const;
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;
    void paint (juce::Graphics&) override;

private:
    int voices = -1;
    int sympathetic = -1;
    bool isReady = false;
    double rate = 0.0;
    int mutePressure = -1;
    int vibrato = -1;
    int tremolo = -1;
};

// Live eight-string fretboard. It shows which physical string carries every
// sounding note, where it is stopped, how hard it is ringing, and which
// strings are only ringing through the sympathetic bridge coupling. A click
// on one row reuses the engine's existing held-string repick gesture. All of
// its geometry and ballistics come from the JUCE-free electry::visuals helpers,
// so the drawing code stays a thin renderer.
class ElectryFretboardDisplay final : public juce::Component,
                                       public juce::SettableTooltipClient
{
public:
    ElectryFretboardDisplay();

    std::function<void (int)> onRepick;

    // Pulls one frame of per-string state. Returns true while the picture is
    // still changing, so the editor only repaints a moving display.
    bool refresh (const ElectryAudioProcessor&, float frameSeconds);

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

private:
    struct StringRow
    {
        electry::StringVisualState state {};
        float level = 0.0f;
        float phase = 0.0f;
    };

    std::array<StringRow, electry::ElectryEngine::stringCount> rows {};
    int selectedString = -1;
    int hoveredString = -1;
    std::uint8_t soloMask = 0;

    void updateAccessibilityTitle();
    void selectString (int stringIndex);
    int stringAtY (float y) const noexcept;
};

class ElectryAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    explicit ElectryAudioProcessorEditor (ElectryAudioProcessor&);
    ~ElectryAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    enum Section
    {
        articulationSection,
        fretboardSection,
        performanceSection,
        coreSection,
        masterSection,
        buildSection,
        detailSection,
        effectsSection,
        sectionCount
    };

    void timerCallback() override;
    void attachSlider (juce::Slider&, const char* parameterId);
    void updateFxEnabledState (bool enabled);

    ElectryAudioProcessor& electryProcessor;
    ElectryLookAndFeel lookAndFeel;
    juce::TooltipWindow tooltipWindow { this, 600 };

    juce::Label logoLabel;
    juce::Label editionLabel;
    juce::ComboBox factoryProgramSelector;
    ElectryStatusDisplay statusDisplay;
    ElectryTextButton panicButton { "PANIC" };

    // The two independent keyswitch banks: how the pick moves and what the
    // hands do. Any combination of the two is reachable.
    ElectryChoiceStrip pickStyleStrip {
        "PICK STROKE",
        { "DOWN", "UP", "ALT" }
    };
    ElectryChoiceStrip playStyleStrip {
        "PLAY STYLE",
        { "SUSTAIN", "MUTE", "HAMMER", "HARMONIC", "PINCH", "SLIDE",
          "DEAD" }
    };
    ElectryChoiceStrip playStyleKeyModeStrip {
        "KEY MODE", { "LATCH", "HOLD" }
    };
    ElectryChoiceStrip pickupStrip {
        "PICKUP", { "NECK", "BOTH", "BRIDGE" }, 1
    };
    ElectryChoiceStrip outputModeStrip {
        {}, { "MONO", "STEREO", "DOUBLE" }, 8, "OUTPUT MODE"
    };
    ElectryChoiceStrip ampModelStrip {
        "AMP VOICE",
        { "AMERICAN CLEAN", "BRITISH CRUNCH", "MODERN HIGH-GAIN" }
    };
    ElectryChoiceStrip fxOversamplingStrip {
        {}, { "STANDARD", "HIGH" }, 2, "FX OVERSAMPLING"
    };
    juce::Label fxOversamplingLabel;
    ElectryTextButton fxEnableButton { "FX OFF" };

    ElectryKnob guitarBuildKnob { "BUILD" };
    ElectryKnob bodyResonanceKnob { "BODY RESONANCE" };

    ElectryKnob pickupTypeKnob { "PICKUP TYPE" };
    ElectryKnob toneKnob { "TONE" };

    ElectryKnob stringAgeKnob { "STRING AGE" };
    ElectryKnob pickPositionKnob { "PICK POSITION" };
    ElectryKnob pickHardnessKnob { "PICK HARDNESS" };
    ElectryKnob bendTimeKnob { "BEND TIME" };
    ElectryKnob muteDampingKnob { "MUTE TIGHTNESS" };
    ElectryKnob velocityKnob { "VELOCITY" };

    ElectryKnob pickNoiseKnob { "PICK NOISE" };
    ElectryKnob fingerNoiseKnob { "FINGER NOISE" };
    ElectryKnob releaseNoiseKnob { "RELEASE NOISE" };
    ElectryKnob artifactsKnob { "MECHANICS" };

    ElectryKnob sympatheticKnob { "STRING RING" };
    ElectryKnob palmMuteKnob { "PALM PRESSURE" };
    ElectryKnob strumSpreadKnob { "STRUM TIME" };
    ElectryKnob tremoloRateKnob { "PICK RATE" };
    ElectryKnob resonanceKnob { "RESONANCE" };

    ElectryKnob outputKnob { "OUTPUT" };
    ElectryKnob distortionKnob { "PEDAL DRIVE" };
    ElectryKnob ampKnob { "AMP DRIVE" };
    ElectryKnob compressorKnob { "COMPRESSOR" };
    ElectryKnob delayKnob { "DELAY" };
    ElectryKnob roomKnob { "ROOM" };

    ElectryFretboardDisplay fretboardDisplay;
    ElectryKeyboardComponent keyboard;

    std::unique_ptr<juce::ParameterAttachment> pickupAttachment;
    std::unique_ptr<juce::ParameterAttachment> outputModeAttachment;
    std::unique_ptr<juce::ParameterAttachment> ampModelAttachment;
    std::unique_ptr<juce::ParameterAttachment> fxOversamplingAttachment;
    std::unique_ptr<juce::ParameterAttachment> fxEnabledAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::array<juce::Rectangle<int>, sectionCount> sectionBounds {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ElectryAudioProcessorEditor)
};
