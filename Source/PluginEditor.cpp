#include "PluginEditor.h"

#include "DSP/ElectryVisuals.h"

#include <cmath>

namespace
{
namespace colours
{
const juce::Colour background { 0xff0b0d10 };
const juce::Colour panel { 0xff13171c };
const juce::Colour panelTop { 0xff232a31 };
const juce::Colour panelOutline { 0xff4c535a };
const juce::Colour binding { 0xffc9c1b2 };
const juce::Colour text { 0xfff0ede6 };
const juce::Colour dimText { 0xffa7adb4 };
const juce::Colour accent { 0xffd44832 };
const juce::Colour accentBright { 0xffff703f };
const juce::Colour accentDark { 0xff50231f };
const juce::Colour oxblood { 0xff42201e };
const juce::Colour knobFace { 0xff171b20 };
const juce::Colour bakeliteEdge { 0xff080a0c };
const juce::Colour nickel { 0xffaeb7bd };
const juce::Colour warmBone { 0xffccd0cf };
const juce::Colour ebony { 0xff101317 };
const juce::Colour keyswitchBlack { 0xff242b31 };
const juce::Colour rosewood { 0xff252b30 };
const juce::Colour rosewoodDark { 0xff0d1115 };
const juce::Colour fretWire { 0xff8d989f };
const juce::Colour sympatheticRing { 0xff91a8b8 };
const juce::Colour pickKeys { 0xffe1b96c };
const juce::Colour styleKeys { 0xffff8057 };
const juce::Colour gestureKeys { 0xffb69ae8 };
const juce::Colour soloKeys { 0xff65c9dd };
} // namespace colours

constexpr int editorWidth = 1180;
constexpr int editorHeight = 910;
constexpr int fretboardPanelHeight = 152;
constexpr int statusDisplayWidth = 256;
constexpr int timerHz = 30;
constexpr int lastDrawnFret = electry::ElectryEngine::fretCount;
constexpr int firstKeyboardNote = electry::ElectryEngine::firstKeyswitchNote; // C0
constexpr int firstPlayableNote = electry::ElectryEngine::lowestMidiPlayableNote; // E2
constexpr int lastKeyboardNote = electry::ElectryEngine::highestMidiPlayableNote; // D7
constexpr int keyswitchCount = electry::ElectryEngine::keyswitchCount;
constexpr int keyboardWhiteKeyCount = 51; // C0..D7 inclusive
constexpr auto visualWeightProperty = "electryVisualWeight";
constexpr float compactKnobWeight = 0.65f;
constexpr int sectionTitleHeight = 30;
constexpr int sectionContentTrim = sectionTitleHeight - 10;
constexpr int effectsHeaderHeight = 42;
constexpr int keyboardLegendHeight = 24;
constexpr auto keyboardInstructions =
    "C0..D0 pick stroke; D#0..A0 style; A#0 vibrato; B0 tremolo; "
    "C1..G1 solo string (G#1 clear). E2..D7 plays.";

enum class KnobTier
{
    detail,
    contextual,
    hero,
    master
};

struct KnobTierMetrics
{
    int width;
    int height;
    float visualWeight;
};

constexpr KnobTierMetrics metricsFor (KnobTier tier) noexcept
{
    switch (tier)
    {
        case KnobTier::detail:     return { 68, 110, 0.52f };
        case KnobTier::contextual: return { 80, 116, 0.72f };
        case KnobTier::hero:       return { 104, 128, 1.00f };
        case KnobTier::master:     return { 80, 122, 0.94f };
    }
    return { 88, 122, 0.82f };
}

static_assert (metricsFor (KnobTier::detail).visualWeight < compactKnobWeight);
static_assert (metricsFor (KnobTier::contextual).visualWeight >= compactKnobWeight);

struct KnobSlot
{
    ElectryKnob* knob;
    KnobTier tier;
};

// JUCE's stock slider label is accessibility-ignored. Electry deliberately
// keeps its editable value as a Tab stop, so retain JUCE's no-wheel behaviour
// while using Label's native editable-text handler.
class ElectrySliderValueLabel final : public juce::Label
{
    void mouseWheelMove (const juce::MouseEvent&,
                         const juce::MouseWheelDetails&) override {}
};

void layoutKnobRow (juce::Rectangle<int> rowArea,
                    std::initializer_list<KnobSlot> slots, int gap)
{
    if (slots.size() == 0 || rowArea.isEmpty())
        return;

    int preferredWidth = gap * (static_cast<int> (slots.size()) - 1);
    int preferredHeight = 0;
    for (const auto& slot : slots)
    {
        preferredWidth += metricsFor (slot.tier).width;
        preferredHeight = juce::jmax (preferredHeight,
                                      metricsFor (slot.tier).height);
    }

    const auto scale = juce::jmin (
        1.0f, static_cast<float> (rowArea.getWidth())
                  / static_cast<float> (juce::jmax (1, preferredWidth)));

    int groupWidth = gap * (static_cast<int> (slots.size()) - 1);
    for (const auto& slot : slots)
        groupWidth += juce::jmax (1, juce::roundToInt (
            static_cast<float> (metricsFor (slot.tier).width) * scale));

    int x = rowArea.getX() + juce::jmax (0, (rowArea.getWidth() - groupWidth) / 2);
    const int groupHeight = juce::jmin (rowArea.getHeight(), preferredHeight);
    const int y = rowArea.getCentreY() - groupHeight / 2;
    for (const auto& slot : slots)
    {
        const auto metrics = metricsFor (slot.tier);
        const int width = juce::jmax (1, juce::roundToInt (
            static_cast<float> (metrics.width) * scale));
        slot.knob->slider.getProperties().set (
            visualWeightProperty, metrics.visualWeight);
        slot.knob->setBounds (x, y, width, groupHeight);
        slot.knob->repaint();
        x += width + gap;
    }
}

constexpr std::array<const char*, keyswitchCount> keyswitchLabels {
    "DN", "UP", "ALT", "SUS", "MUT", "H/P", "HRM", "PNC", "SLD", "X"
};

constexpr std::array<const char*, electry::ElectryEngine::soloStringKeyswitchCount> soloStringLabels {
    "S8", "S7", "S6", "S5", "S4", "S3", "S2", "S1"
};

bool isKeyswitch (int midiNoteNumber) noexcept
{
    return midiNoteNumber >= firstKeyboardNote
        && midiNoteNumber < firstKeyboardNote + keyswitchCount;
}

bool isSoloStringKeyswitch (int midiNoteNumber) noexcept
{
    return electry::ElectryEngine::isMidiSoloStringNote (midiNoteNumber);
}

bool isSoloClearKeyswitch (int midiNoteNumber) noexcept
{
    return electry::ElectryEngine::isMidiSoloClearNote (midiNoteNumber);
}

bool isVibratoGesture (int midiNoteNumber) noexcept
{
    return electry::ElectryEngine::isVibratoGestureNote (midiNoteNumber);
}

bool isTremoloGesture (int midiNoteNumber) noexcept
{
    return electry::ElectryEngine::isTremoloGestureNote (midiNoteNumber);
}

// The dead zone notes before the playable range (A1..D#2: 33..39) are drawn muted.
bool isDeadZoneNote (int midiNoteNumber) noexcept
{
    return midiNoteNumber > electry::ElectryEngine::midiSoloClearNote
        && midiNoteNumber < firstPlayableNote;
}

juce::Colour functionKeyColour (int midiNoteNumber) noexcept
{
    if (isSoloStringKeyswitch (midiNoteNumber) || isSoloClearKeyswitch (midiNoteNumber))
        return colours::soloKeys;
    if (isVibratoGesture (midiNoteNumber) || isTremoloGesture (midiNoteNumber))
        return colours::gestureKeys;
    if (isKeyswitch (midiNoteNumber))
        return midiNoteNumber < electry::ElectryEngine::firstPlayStyleKeyswitchNote
            ? colours::pickKeys : colours::styleKeys;
    return colours::accentBright;
}

void drawKeyboardLegend (juce::Graphics& graphics, juce::Rectangle<int> area)
{
    const auto labelFont = juce::Font (juce::FontOptions (9.5f, juce::Font::bold))
        .withExtraKerningFactor (0.05f);
    const auto rangeFont = juce::Font (juce::FontOptions (10.5f));
    const auto item = [&graphics, &area, &labelFont, &rangeFont] (
        const char* label, const char* range, juce::Colour colour)
    {
        const int labelWidth = juce::GlyphArrangement::getStringWidthInt (labelFont, label);
        const int rangeWidth = juce::GlyphArrangement::getStringWidthInt (rangeFont, range);
        auto bounds = area.removeFromLeft (16 + labelWidth + 8 + rangeWidth);
        graphics.setColour (colour);
        graphics.fillRoundedRectangle (bounds.withWidth (8).withSizeKeepingCentre (8, 3)
                                             .toFloat(), 1.0f);
        bounds.removeFromLeft (16);
        graphics.setColour (colour.interpolatedWith (colours::text, 0.25f));
        graphics.setFont (labelFont);
        graphics.drawText (label, bounds.removeFromLeft (labelWidth),
                            juce::Justification::centredLeft);
        bounds.removeFromLeft (8);
        graphics.setColour (colours::dimText);
        graphics.setFont (rangeFont);
        graphics.drawText (range, bounds, juce::Justification::centredLeft);
        area.removeFromLeft (24);
    };
    item ("PICK STROKE", "C0-D0", colours::pickKeys);
    item ("PLAY STYLE", "D#0-A0", colours::styleKeys);
    item ("GESTURES", "A#0 VIB / B0 TRM", colours::gestureKeys);
    item ("STRING SOLO", "C1-G1 / G#1 CLR", colours::soloKeys);
    graphics.setColour (colours::dimText);
    graphics.setFont (rangeFont);
    graphics.drawText ("PLAYABLE  E2-D7", area, juce::Justification::centredRight);
}

void drawFunctionDecoration (juce::Graphics& graphics, juce::Rectangle<float> area,
                             const juce::String& label, bool selected, bool blackKey,
                             juce::Colour groupColour)
{
    const auto badge = area.removeFromBottom (blackKey ? 18.0f : 22.0f)
                          .reduced (blackKey ? 0.8f : 1.5f, 3.0f);
    graphics.setColour (selected ? groupColour
                                 : colours::ebony.interpolatedWith (groupColour, 0.17f));
    graphics.fillRect (badge);
    graphics.setColour (groupColour.withAlpha (selected ? 1.0f : 0.75f));
    graphics.fillRect (badge.withHeight (selected ? 1.6f : 1.0f));
    graphics.setColour (selected ? colours::background
                                 : groupColour.interpolatedWith (colours::text, 0.55f));
    graphics.setFont (juce::FontOptions (blackKey ? 7.4f : 8.8f, juce::Font::bold));
    graphics.drawFittedText (label, badge.getSmallestIntegerContainer(),
                             juce::Justification::centred, 1, 0.72f);
}

void drawKeyswitchDecoration (juce::Graphics& graphics, juce::Rectangle<float> area,
                              int keyswitchIndex, bool selected, bool blackKey)
{
    drawFunctionDecoration (graphics, area,
        keyswitchLabels[static_cast<std::size_t> (keyswitchIndex)], selected, blackKey,
        functionKeyColour (firstKeyboardNote + keyswitchIndex));
}

void drawSoloDecoration (juce::Graphics& graphics, juce::Rectangle<float> area,
                         int stringIndex, bool selected, bool blackKey)
{
    drawFunctionDecoration (graphics, area,
        soloStringLabels[static_cast<std::size_t> (stringIndex)], selected, blackKey,
        colours::soloKeys);
}

void drawSoloClearDecoration (juce::Graphics& graphics, juce::Rectangle<float> area,
                              bool blackKey, bool isDown)
{
    drawFunctionDecoration (graphics, area, "CLR", isDown, blackKey, colours::soloKeys);
}

void drawVibratoDecoration (juce::Graphics& graphics,
                            juce::Rectangle<float> area, bool isDown)
{
    drawFunctionDecoration (graphics, area, "VIB", isDown, true, colours::gestureKeys);
}

void drawTremoloDecoration (juce::Graphics& graphics,
                            juce::Rectangle<float> area, bool isDown)
{
    drawFunctionDecoration (graphics, area, "TRM", isDown, false, colours::gestureKeys);
}

} // namespace

// ---------------------------------------------------------------------------
// Look and feel
// ---------------------------------------------------------------------------

ElectryLookAndFeel::ElectryLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, colours::background);
    setColour (juce::Slider::textBoxTextColourId, colours::text);
    setColour (juce::Slider::textBoxBackgroundColourId,
               juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, colours::accentDark);
    setColour (juce::Label::textColourId, colours::text);
    setColour (juce::ComboBox::backgroundColourId, colours::knobFace);
    setColour (juce::ComboBox::textColourId, colours::text);
    setColour (juce::ComboBox::outlineColourId,
               colours::panelOutline.withAlpha (0.75f));
    setColour (juce::ComboBox::arrowColourId, colours::accentBright);
    setColour (juce::PopupMenu::backgroundColourId, colours::knobFace);
    setColour (juce::PopupMenu::textColourId, colours::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId,
               colours::accentBright);
    setColour (juce::PopupMenu::highlightedTextColourId, colours::background);
    setColour (juce::PopupMenu::headerTextColourId, colours::dimText);
    setColour (juce::TextButton::buttonColourId, colours::knobFace);
    setColour (juce::TextButton::textColourOffId, colours::text);
    setColour (juce::TooltipWindow::backgroundColourId, colours::panelTop);
    setColour (juce::TooltipWindow::textColourId, colours::text);
    setColour (juce::TooltipWindow::outlineColourId, colours::panelOutline);
    setColour (juce::MidiKeyboardComponent::whiteNoteColourId, colours::warmBone);
    setColour (juce::MidiKeyboardComponent::blackNoteColourId, colours::ebony);
    setColour (juce::MidiKeyboardComponent::textLabelColourId,
               colours::rosewoodDark);
    setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId,
               juce::Colour (0xff353c43));
    setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,
               colours::accent.withAlpha (0.35f));
    setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId,
               colours::accent.withAlpha (0.7f));
}

void ElectryLookAndFeel::drawRotarySlider (juce::Graphics& graphics, int x, int y,
                                           int width, int height, float sliderPos,
                                           float rotaryStartAngle, float rotaryEndAngle,
                                           juce::Slider& slider)
{
    juce::Graphics::ScopedSaveState save (graphics);
    const auto visualWeight = juce::jlimit (
        0.45f, 1.0f,
        static_cast<float> (slider.getProperties().getWithDefault (
            visualWeightProperty, 0.82f)));
    const bool enabled = slider.isEnabled();
    const bool engaged = enabled && (slider.isMouseOverOrDragging()
                                    || slider.hasKeyboardFocus (true));
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (4.5f);
    const auto radius = juce::jmax (1.0f, bounds.getWidth() < bounds.getHeight()
        ? bounds.getWidth() * 0.5f : bounds.getHeight() * 0.5f);
    const auto centre = bounds.getCentre();
    const auto angle = rotaryStartAngle
        + juce::jlimit (0.0f, 1.0f, sliderPos) * (rotaryEndAngle - rotaryStartAngle);
    const auto opacity = enabled ? 1.0f : 0.35f;

    // Engraved scale: compact controls keep three landmarks, heroes eleven.
    const int tickStep = visualWeight < compactKnobWeight ? 5 : 1;
    for (int tick = 0; tick <= 10; tick += tickStep)
    {
        const auto tickAngle = juce::jmap (static_cast<float> (tick), 0.0f, 10.0f,
                                           rotaryStartAngle, rotaryEndAngle);
        const bool major = tick % 5 == 0;
        const auto outer = centre.getPointOnCircumference (radius, tickAngle);
        const auto inner = centre.getPointOnCircumference (
            radius - (major ? 3.8f : 2.0f), tickAngle);
        graphics.setColour ((major ? colours::binding : colours::nickel)
            .withAlpha ((major ? 0.68f : 0.32f) * opacity));
        graphics.drawLine ({ inner, outer }, major ? 1.0f : 0.65f);
    }

    const auto trackRadius = juce::jmax (1.0f, radius - 5.5f);
    juce::Path track, value;
    track.addCentredArc (centre.x, centre.y, trackRadius, trackRadius, 0.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
    value.addCentredArc (centre.x, centre.y, trackRadius, trackRadius, 0.0f,
                         rotaryStartAngle, angle, true);
    graphics.setColour (juce::Colours::black.withAlpha (0.75f * opacity));
    graphics.strokePath (track, juce::PathStrokeType (2.6f));
    graphics.setColour (colours::nickel.withAlpha (0.13f * opacity));
    graphics.strokePath (track, juce::PathStrokeType (0.8f));
    if (engaged)
    {
        graphics.setColour (colours::accent.withAlpha (0.12f));
        graphics.strokePath (value, juce::PathStrokeType (4.0f));
    }
    graphics.setColour (colours::accentBright.withAlpha (
        (engaged ? 0.96f : 0.66f) * opacity));
    graphics.strokePath (value, juce::PathStrokeType (1.0f + 0.25f * visualWeight));

    // Black anodised skirt over a narrow machined-steel chamfer.
    const auto metalRadius = radius * 0.74f;
    const auto metalBounds = juce::Rectangle<float> (
        centre.x - metalRadius, centre.y - metalRadius,
        2.0f * metalRadius, 2.0f * metalRadius);
    graphics.setColour (juce::Colours::black.withAlpha (0.30f * opacity));
    graphics.fillEllipse (metalBounds.expanded (1.0f).translated (0.0f, 2.4f));
    graphics.setColour (juce::Colours::black.withAlpha (0.50f * opacity));
    graphics.fillEllipse (metalBounds.translated (0.0f, 1.3f));
    juce::ColourGradient chamfer (colours::nickel.withAlpha (opacity),
        metalBounds.getX(), metalBounds.getY(),
        colours::bakeliteEdge.withAlpha (opacity),
        metalBounds.getRight(), metalBounds.getBottom(), false);
    chamfer.addColour (0.33, juce::Colour (0xff555e66).withAlpha (opacity));
    chamfer.addColour (0.52, juce::Colour (0xff12171b).withAlpha (opacity));
    chamfer.addColour (0.81, juce::Colour (0xff4c555d).withAlpha (opacity));
    graphics.setGradientFill (chamfer);
    graphics.fillEllipse (metalBounds);

    const auto gripBounds = metalBounds.reduced (1.5f);
    graphics.setGradientFill (juce::ColourGradient (
        juce::Colour (0xff293139).withAlpha (opacity), gripBounds.getX(), gripBounds.getY(),
        colours::bakeliteEdge.withAlpha (opacity), gripBounds.getRight(), gripBounds.getBottom(), false));
    graphics.fillEllipse (gripBounds);
    const int flutes = visualWeight < compactKnobWeight ? 20 : 32;
    for (int flute = 0; flute < flutes; ++flute)
    {
        const auto fluteAngle = juce::MathConstants<float>::twoPi
            * static_cast<float> (flute) / static_cast<float> (flutes);
        const auto inner = centre.getPointOnCircumference (metalRadius * 0.79f, fluteAngle);
        const auto outer = centre.getPointOnCircumference (metalRadius * 0.93f, fluteAngle);
        graphics.setColour ((flute % 2 == 0 ? colours::nickel : juce::Colours::black)
            .withAlpha ((flute % 2 == 0 ? 0.12f : 0.62f) * opacity));
        graphics.drawLine ({ inner, outer }, 0.8f);
    }

    const auto capRadius = metalRadius * 0.79f;
    const auto capBounds = juce::Rectangle<float> (
        centre.x - capRadius, centre.y - capRadius, 2 * capRadius, 2 * capRadius);
    juce::ColourGradient cap (juce::Colour (0xff343d46).withAlpha (opacity),
        centre.x - capRadius * 0.65f, centre.y - capRadius,
        juce::Colour (0xff101419).withAlpha (opacity),
        centre.x + capRadius * 0.70f, centre.y + capRadius, false);
    cap.addColour (0.52, colours::knobFace.withAlpha (opacity));
    graphics.setGradientFill (cap);
    graphics.fillEllipse (capBounds);
    graphics.setColour (colours::nickel.withAlpha (0.22f * opacity));
    graphics.drawEllipse (capBounds, 0.7f);
    graphics.setColour (juce::Colours::black.withAlpha (0.48f * opacity));
    graphics.drawEllipse (capBounds.reduced (1.2f), 0.6f);

    // A few faint horizontal machining lines stay inside the metal face.
    for (int line = -3; line <= 3; ++line)
    {
        const float lineY = static_cast<float> (line) * capRadius * 0.19f;
        const float halfWidth = std::sqrt (juce::jmax (0.0f,
            capRadius * capRadius * 0.80f - lineY * lineY));
        graphics.setColour (colours::nickel.withAlpha (0.028f * opacity));
        graphics.drawLine (centre.x - halfWidth, centre.y + lineY,
                           centre.x + halfWidth, centre.y + lineY, 0.55f);
    }

    const auto pointerOuter = centre.getPointOnCircumference (capRadius * 0.82f, angle);
    const auto pointerInner = centre.getPointOnCircumference (capRadius * 0.31f, angle);
    graphics.setColour (juce::Colours::black.withAlpha (0.68f * opacity));
    graphics.drawLine ({ pointerInner, pointerOuter }, 3.4f);
    if (enabled)
    {
        graphics.setColour (colours::accent.withAlpha (engaged ? 0.24f : 0.11f));
        graphics.drawLine ({ pointerInner, pointerOuter }, 4.1f);
    }
    graphics.setColour (colours::accentBright.withAlpha (opacity));
    graphics.drawLine ({ pointerInner, pointerOuter }, 1.8f);
    graphics.setColour (colours::text.withAlpha (0.80f * opacity));
    graphics.fillEllipse (pointerOuter.x - 0.8f, pointerOuter.y - 0.8f, 1.6f, 1.6f);
    if (slider.hasKeyboardFocus (true) && enabled)
    {
        graphics.setColour (colours::binding.withAlpha (0.88f));
        graphics.strokePath (track, juce::PathStrokeType (0.7f));
    }
}

void ElectryLookAndFeel::drawButtonBackground (juce::Graphics& graphics,
                                               juce::Button& button,
                                               const juce::Colour& backgroundColour,
                                               bool isHighlighted, bool isDown)
{
    juce::Graphics::ScopedSaveState save (graphics);
    const auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = button.getToggleState();
    const bool enabled = button.isEnabled();
    const auto cut = juce::jmin (4.0f, bounds.getHeight() * 0.17f);
    const auto chamfered = [] (juce::Rectangle<float> area, float corner)
    {
        juce::Path path;
        path.startNewSubPath (area.getX() + corner, area.getY());
        path.lineTo (area.getRight() - corner, area.getY());
        path.lineTo (area.getRight(), area.getY() + corner);
        path.lineTo (area.getRight(), area.getBottom() - corner);
        path.lineTo (area.getRight() - corner, area.getBottom());
        path.lineTo (area.getX() + corner, area.getBottom());
        path.lineTo (area.getX(), area.getBottom() - corner);
        path.lineTo (area.getX(), area.getY() + corner);
        path.closeSubPath();
        return path;
    };
    const auto shape = chamfered (bounds, cut);
    auto fill = on ? juce::Colour (0xff49261f) : backgroundColour;
    if (enabled && isDown) fill = fill.brighter (0.16f);
    else if (enabled && isHighlighted) fill = fill.brighter (0.09f);

    graphics.setColour (juce::Colours::black.withAlpha (0.50f));
    graphics.fillPath (shape, juce::AffineTransform::translation (0.0f, 1.0f));
    graphics.setGradientFill (juce::ColourGradient (
        fill.brighter (on ? 0.12f : 0.05f), bounds.getCentreX(), bounds.getY(),
        fill.darker (0.28f), bounds.getCentreX(), bounds.getBottom(), false));
    graphics.fillPath (shape);
    if (on && enabled)
    {
        graphics.setColour (colours::accent.withAlpha (0.13f));
        graphics.strokePath (shape, juce::PathStrokeType (3.0f));
    }
    graphics.setColour (on ? colours::accent.withAlpha (0.92f)
        : colours::panelOutline.withAlpha (isHighlighted && enabled ? 0.90f : 0.55f));
    graphics.strokePath (shape, juce::PathStrokeType (on ? 1.0f : 0.75f));
    graphics.setColour (colours::nickel.withAlpha (on ? 0.17f : 0.12f));
    graphics.drawLine (bounds.getX() + cut + 1.0f, bounds.getY() + 1.0f,
                       bounds.getRight() - cut - 1.0f, bounds.getY() + 1.0f, 0.65f);
    if (on)
    {
        const auto indicatorWidth = juce::jmin (17.0f, bounds.getWidth() * 0.26f);
        graphics.setColour (colours::accentBright);
        graphics.fillRoundedRectangle (bounds.getCentreX() - indicatorWidth * 0.5f,
            bounds.getBottom() - 2.6f, indicatorWidth, 1.6f, 0.8f);
    }
    if (enabled && button.hasKeyboardFocus (true))
    {
        graphics.setColour (colours::binding.withAlpha (0.90f));
        graphics.strokePath (chamfered (bounds.reduced (2.5f), juce::jmax (1.0f, cut - 1.5f)),
                             juce::PathStrokeType (0.8f));
    }
}

void ElectryLookAndFeel::drawButtonText (juce::Graphics& graphics, juce::TextButton& button,
                                         bool isHighlighted, bool)
{
    const auto font = getTextButtonFont (button, button.getHeight());
    graphics.setFont (font);
    graphics.setColour (button.getToggleState() || isHighlighted ? colours::text
        : colours::text.withAlpha (0.83f));
    auto area = button.getLocalBounds().reduced (5, 2);
    const auto label = button.getButtonText();
    const auto* parent = button.getParentComponent();
    const bool stroke = parent != nullptr && parent->getComponentID() == "pickStyleStrip";
    const bool amp = parent != nullptr && parent->getComponentID() == "ampModel";
    const float estimatedTextWidth = static_cast<float> (label.length()) * font.getHeight() * 0.56f;
    if ((stroke || amp) && static_cast<float> (area.getWidth()) > estimatedTextWidth + 24.0f)
    {
        const auto groupWidth = juce::jmin (area.getWidth(), juce::roundToInt (estimatedTextWidth) + 22);
        area = area.withSizeKeepingCentre (groupWidth, area.getHeight());
        const auto iconArea = area.removeFromLeft (15).toFloat();
        area.removeFromLeft (5);
        const auto mid = iconArea.getCentre();
        juce::Path icon;
        if (stroke)
        {
            const bool up = label.containsIgnoreCase ("UP");
            const bool alternate = label.containsIgnoreCase ("ALT");
            const auto arrow = [&] (float horizontal, bool pointsUp)
            {
                const auto top = mid.y - 5.0f, bottom = mid.y + 5.0f;
                const auto tip = pointsUp ? top : bottom;
                const auto wing = pointsUp ? top + 3.0f : bottom - 3.0f;
                icon.startNewSubPath (horizontal, pointsUp ? bottom : top);
                icon.lineTo (horizontal, tip);
                icon.startNewSubPath (horizontal - 2.6f, wing);
                icon.lineTo (horizontal, tip);
                icon.lineTo (horizontal + 2.6f, wing);
            };
            arrow (mid.x - (alternate ? 3.0f : 0.0f), up);
            if (alternate) arrow (mid.x + 3.0f, true);
        }
        else if (label.containsIgnoreCase ("CLEAN"))
        {
            icon.startNewSubPath (mid.x - 6.0f, mid.y);
            icon.cubicTo (mid.x - 3.0f, mid.y - 9.0f, mid.x - 1.0f, mid.y - 4.0f, mid.x, mid.y);
            icon.cubicTo (mid.x + 2.0f, mid.y + 8.0f, mid.x + 4.0f, mid.y + 4.0f, mid.x + 6.0f, mid.y);
        }
        else
        {
            const bool modern = label.containsIgnoreCase ("MODERN");
            icon.startNewSubPath (mid.x - 6.0f, mid.y + 4.0f);
            icon.lineTo (mid.x - (modern ? 5.0f : 3.0f), mid.y - 4.0f);
            icon.lineTo (mid.x - 1.0f, mid.y - 4.0f);
            icon.lineTo (mid.x + (modern ? 0.0f : 2.0f), mid.y + 4.0f);
            icon.lineTo (mid.x + 5.0f, mid.y + 4.0f);
            icon.lineTo (mid.x + 6.0f, mid.y - 4.0f);
        }
        graphics.setColour (button.getToggleState() ? colours::accentBright : colours::binding);
        graphics.strokePath (icon, juce::PathStrokeType (1.15f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        graphics.setColour (button.getToggleState() || isHighlighted ? colours::text
            : colours::text.withAlpha (0.83f));
    }
    graphics.drawFittedText (label, area, juce::Justification::centred, 1);
}

void ElectryLookAndFeel::drawLabel (juce::Graphics& graphics, juce::Label& label)
{
    const bool caption = static_cast<bool> (label.getProperties().getWithDefault (
        "electryControlCaption", false));
    const bool sliderValue = static_cast<bool> (label.getProperties().getWithDefault (
        "electrySliderValue", false));
    graphics.setColour (label.findColour (juce::Label::textColourId)
        .withMultipliedAlpha (label.isEnabled() ? 1.0f : 0.5f));
    graphics.setFont (label.getFont());
    if (! label.isBeingEdited())
        graphics.drawFittedText (label.getText(), label.getLocalBounds(),
            label.getJustificationType(), sliderValue ? 1 : 2, label.getMinimumHorizontalScale());
    if (! caption && label.hasKeyboardFocus (true) && label.isEnabled())
    {
        graphics.setColour (colours::accentBright.withAlpha (0.8f));
        graphics.drawRoundedRectangle (label.getLocalBounds().toFloat().reduced (0.6f),
                                        2.0f, 0.75f);
    }
}

void ElectryLookAndFeel::drawComboBox (juce::Graphics& graphics, int width,
                                        int height, bool isButtonDown,
                                        int, int, int, int, juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float> (0.5f, 0.5f,
                                           static_cast<float> (width) - 1.0f,
                                           static_cast<float> (height) - 1.0f);
    const bool engaged = box.isEnabled()
        && (isButtonDown || box.isMouseOverOrDragging() || box.hasKeyboardFocus (true));
    const auto fill = colours::knobFace.brighter (engaged ? 0.12f : 0.02f);
    juce::ColourGradient gradient (fill.brighter (0.08f), bounds.getX(),
                                   bounds.getY(), fill.darker (0.25f),
                                   bounds.getX(), bounds.getBottom(), false);
    juce::Path shape;
    constexpr float chamfer = 3.5f;
    shape.startNewSubPath (bounds.getX() + chamfer, bounds.getY());
    shape.lineTo (bounds.getRight() - chamfer, bounds.getY());
    shape.lineTo (bounds.getRight(), bounds.getY() + chamfer);
    shape.lineTo (bounds.getRight(), bounds.getBottom() - chamfer);
    shape.lineTo (bounds.getRight() - chamfer, bounds.getBottom());
    shape.lineTo (bounds.getX() + chamfer, bounds.getBottom());
    shape.lineTo (bounds.getX(), bounds.getBottom() - chamfer);
    shape.lineTo (bounds.getX(), bounds.getY() + chamfer);
    shape.closeSubPath();
    graphics.setGradientFill (gradient);
    graphics.fillPath (shape);
    graphics.setColour (engaged ? colours::binding.withAlpha (0.78f)
                               : colours::panelOutline.withAlpha (0.66f));
    graphics.strokePath (shape, juce::PathStrokeType (0.8f));
    graphics.setColour (colours::nickel.withAlpha (0.15f));
    graphics.drawVerticalLine (width - 31, bounds.getY() + 6.0f, bounds.getBottom() - 6.0f);

    const auto arrowX = bounds.getRight() - 18.0f;
    const auto arrowY = bounds.getCentreY();
    juce::Path arrow;
    arrow.startNewSubPath (arrowX - 4.0f, arrowY - 2.0f);
    arrow.lineTo (arrowX, arrowY + 2.0f);
    arrow.lineTo (arrowX + 4.0f, arrowY - 2.0f);
    graphics.setColour (colours::accentBright.withAlpha (box.isEnabled() ? 0.9f : 0.35f));
    graphics.strokePath (arrow, juce::PathStrokeType (1.5f,
                                                       juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
}

void ElectryLookAndFeel::positionComboBoxText (juce::ComboBox& box,
                                                juce::Label& label)
{
    label.setBounds (10, 1, juce::jmax (0, box.getWidth() - 40),
                     juce::jmax (0, box.getHeight() - 2));
    label.setFont (getComboBoxFont (box));
    label.setJustificationType (juce::Justification::centredLeft);
}

juce::Font ElectryLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return juce::Font (juce::FontOptions (12.5f, juce::Font::bold))
        .withExtraKerningFactor (0.025f);
}

juce::Label* ElectryLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* label = new ElectrySliderValueLabel;
    label->getProperties().set ("electrySliderValue", true);
    const bool linearBar = slider.getSliderStyle() == juce::Slider::LinearBar
                        || slider.getSliderStyle() == juce::Slider::LinearBarVertical;
    const auto background = slider.findColour (
        juce::Slider::textBoxBackgroundColourId);
    label->setKeyboardType (juce::TextInputTarget::decimalKeyboard);
    label->setFont (juce::FontOptions (11.5f));
    label->setColour (juce::Label::textColourId, colours::binding.withAlpha (0.88f));
    label->setColour (juce::Label::backgroundColourId,
                      linearBar ? juce::Colours::transparentBlack : background);
    label->setColour (juce::Label::outlineColourId,
                      slider.findColour (juce::Slider::textBoxOutlineColourId));
    label->setColour (juce::TextEditor::textColourId,
                      slider.findColour (juce::Slider::textBoxTextColourId));
    label->setColour (juce::TextEditor::backgroundColourId,
                      linearBar ? background.withAlpha (0.7f) : colours::panel);
    label->setColour (juce::TextEditor::outlineColourId,
                      slider.findColour (juce::Slider::textBoxOutlineColourId));
    label->setColour (juce::TextEditor::highlightColourId,
                      slider.findColour (juce::Slider::textBoxHighlightColourId));
    label->setColour (juce::TextEditor::highlightedTextColourId, colours::text);
    label->setColour (juce::TextEditor::focusedOutlineColourId, colours::accentBright);
    label->setJustificationType (juce::Justification::centred);
    return label;
}

juce::Font ElectryLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return juce::Font (juce::FontOptions (
        juce::jmin (12.0f, static_cast<float> (buttonHeight) * 0.6f),
        juce::Font::bold)).withExtraKerningFactor (0.025f);
}

std::unique_ptr<juce::FocusOutline> ElectryLookAndFeel::createFocusOutlineForComponent (
    juce::Component&)
{
    // Keep JUCE's native focus tracking and accessibility surface; replace
    // only the stock yellow outline with the instrument's metal/ember finish.
    struct MetalFocusOutline final : public juce::FocusOutline::OutlineWindowProperties
    {
        juce::Rectangle<int> getOutlineBounds (juce::Component& component) override
        {
            return component.getScreenBounds().expanded (1);
        }

        void drawOutline (juce::Graphics& graphics, int width, int height) override
        {
            const auto bounds = juce::Rectangle<int> (0, 0, width, height)
                .toFloat().reduced (0.85f);
            const auto cut = juce::jmin (3.0f, bounds.getHeight() * 0.18f);
            juce::Path shape;
            shape.startNewSubPath (bounds.getX() + cut, bounds.getY());
            shape.lineTo (bounds.getRight() - cut, bounds.getY());
            shape.lineTo (bounds.getRight(), bounds.getY() + cut);
            shape.lineTo (bounds.getRight(), bounds.getBottom() - cut);
            shape.lineTo (bounds.getRight() - cut, bounds.getBottom());
            shape.lineTo (bounds.getX() + cut, bounds.getBottom());
            shape.lineTo (bounds.getX(), bounds.getBottom() - cut);
            shape.lineTo (bounds.getX(), bounds.getY() + cut);
            shape.closeSubPath();
            graphics.setColour (colours::binding.withAlpha (0.95f));
            graphics.strokePath (shape, juce::PathStrokeType (1.5f));
            const float markWidth = juce::jmin (12.0f, bounds.getWidth() * 0.25f);
            graphics.setColour (colours::accentBright.withAlpha (0.88f));
            graphics.drawLine (bounds.getCentreX() - markWidth * 0.5f, bounds.getBottom(),
                               bounds.getCentreX() + markWidth * 0.5f, bounds.getBottom(), 1.5f);
        }
    };
    return std::make_unique<juce::FocusOutline> (std::make_unique<MetalFocusOutline>());
}

// ---------------------------------------------------------------------------
// Keyboard
// ---------------------------------------------------------------------------

ElectryKeyboardComponent::ElectryKeyboardComponent (juce::MidiKeyboardState& state)
    : MidiKeyboardComponent (state, horizontalKeyboard)
{
}

void ElectryKeyboardComponent::setSelectedKeyswitches (int pickIndex,
                                                       int styleIndex)
{
    const auto pickCount = electry::ElectryEngine::pickStyleKeyswitchCount;
    const auto clampedPick = juce::jlimit (0, pickCount - 1, pickIndex);
    const auto clampedStyle = juce::jlimit (
        0, electry::ElectryEngine::playStyleKeyswitchCount - 1, styleIndex);
    if (selectedPickIndex == clampedPick && selectedStyleIndex == clampedStyle)
        return;

    selectedPickIndex = clampedPick;
    selectedStyleIndex = clampedStyle;
    repaint();
}

void ElectryKeyboardComponent::setSoloStringMask (std::uint8_t mask)
{
    if (activeSoloMask == mask)
        return;
    activeSoloMask = mask;
    repaint();
}

bool ElectryKeyboardComponent::isKeyswitchSelected (int keyswitchIndex) const noexcept
{
    const auto pickCount = electry::ElectryEngine::pickStyleKeyswitchCount;
    return keyswitchIndex < pickCount
        ? keyswitchIndex == selectedPickIndex
        : keyswitchIndex - pickCount == selectedStyleIndex;
}

bool ElectryKeyboardComponent::isSoloStringSelected (int stringIndex) const noexcept
{
    return stringIndex >= 0 && stringIndex < electry::ElectryEngine::stringCount
        && (activeSoloMask & (1u << stringIndex)) != 0;
}

juce::String ElectryKeyboardComponent::getWhiteNoteText (int midiNoteNumber)
{
    if (isKeyswitch (midiNoteNumber) || isSoloStringKeyswitch (midiNoteNumber)
        || isSoloClearKeyswitch (midiNoteNumber) || isDeadZoneNote (midiNoteNumber))
        return {};

    if (midiNoteNumber == firstPlayableNote || midiNoteNumber % 12 == 0)
        return juce::MidiMessage::getMidiNoteName (
            midiNoteNumber, true, true, getOctaveForMiddleC());

    return {};
}

void ElectryKeyboardComponent::drawWhiteNote (
    int midiNoteNumber, juce::Graphics& graphics, juce::Rectangle<float> area,
    bool isDown, bool isOver, juce::Colour lineColour, juce::Colour textColour)
{
    juce::ignoreUnused (lineColour, textColour);
    const bool keyswitch = isKeyswitch (midiNoteNumber);
    const bool tremoloGesture = isTremoloGesture (midiNoteNumber);
    const bool soloKey = isSoloStringKeyswitch (midiNoteNumber);
    const bool soloClr = isSoloClearKeyswitch (midiNoteNumber);
    const bool deadZone = isDeadZoneNote (midiNoteNumber);
    const bool functionKey = keyswitch || tremoloGesture || soloKey || soloClr;
    const int soloIndex = soloKey
        ? midiNoteNumber - electry::ElectryEngine::firstMidiSoloStringNote : -1;
    const bool soloSelected = soloKey && isSoloStringSelected (soloIndex);
    const bool selected = soloSelected || (keyswitch
        && isKeyswitchSelected (midiNoteNumber - firstKeyboardNote));
    const auto groupColour = functionKeyColour (midiNoteNumber);

    // Group tint remains visible even when no key in that group is active.
    // The pitched range retains its familiar ivory/black piano silhouette.
    const auto face = deadZone ? colours::ebony.brighter (0.035f)
                    : functionKey ? colours::rosewood.interpolatedWith (
                        groupColour, selected ? 0.34f : 0.19f)
                    : colours::warmBone;
    graphics.setGradientFill ({ face.brighter (functionKey ? 0.04f : 0.10f),
                                area.getCentreX(), area.getY(),
                                face.darker (functionKey ? 0.16f : 0.055f),
                                area.getCentreX(), area.getBottom(), false });
    graphics.fillRect (area);
    if (isOver && ! deadZone)
    {
        graphics.setColour (groupColour.withAlpha (functionKey ? 0.12f : 0.08f));
        graphics.fillRect (area);
    }
    if (isDown && ! deadZone)
    {
        graphics.setColour (functionKey ? groupColour.withAlpha (0.24f)
                                       : colours::accent.withAlpha (0.38f));
        graphics.fillRect (area);
    }
    graphics.setColour (colours::ebony.withAlpha (functionKey ? 0.95f : 0.70f));
    graphics.fillRect (area.withRight (area.getX() + 0.8f));
    graphics.setColour (juce::Colours::white.withAlpha (functionKey ? 0.06f : 0.34f));
    graphics.fillRect (area.withTrimmedLeft (1.0f).withHeight (0.8f));
    graphics.setColour (colours::ebony.withAlpha (functionKey ? 0.50f : 0.16f));
    graphics.fillRect (area.withTop (area.getBottom() - 3.0f));

    if (functionKey || (isDown && ! deadZone))
    {
        graphics.setColour (groupColour.withAlpha (selected || isDown ? 1.0f : 0.45f));
        graphics.fillRect (area.reduced (1.0f, 0.0f).withTop (area.getBottom() - 3.0f));
    }

    if (keyswitch)
        drawKeyswitchDecoration (graphics, area, midiNoteNumber - firstKeyboardNote,
                                 selected, false);
    else if (tremoloGesture)
        drawTremoloDecoration (graphics, area, isDown);
    else if (soloKey)
        drawSoloDecoration (graphics, area, soloIndex, soloSelected, false);
    else if (soloClr)
        drawSoloClearDecoration (graphics, area, false, isDown);
    else if (! deadZone)
    {
        graphics.setColour (colours::ebony.withAlpha (0.85f));
        graphics.setFont (juce::FontOptions (9.0f, juce::Font::bold));
        graphics.drawFittedText (getWhiteNoteText (midiNoteNumber),
            area.withTop (area.getBottom() - 20.0f).reduced (1.0f, 2.0f)
                .getSmallestIntegerContainer(), juce::Justification::centred, 1, 0.75f);
    }

    if (midiNoteNumber == firstPlayableNote)
    {
        graphics.setColour (colours::accent);
        graphics.fillRect (area.withWidth (2.0f));
    }
}

void ElectryKeyboardComponent::drawBlackNote (
    int midiNoteNumber, juce::Graphics& graphics, juce::Rectangle<float> area,
    bool isDown, bool isOver, juce::Colour noteFillColour)
{
    juce::ignoreUnused (noteFillColour);
    const bool keyswitch = isKeyswitch (midiNoteNumber);
    const bool vibratoGesture = isVibratoGesture (midiNoteNumber);
    const bool soloKey = isSoloStringKeyswitch (midiNoteNumber);
    const bool soloClr = isSoloClearKeyswitch (midiNoteNumber);
    const bool deadZone = isDeadZoneNote (midiNoteNumber);
    const bool functionKey = keyswitch || vibratoGesture || soloKey || soloClr;
    const int soloIndex = soloKey
        ? midiNoteNumber - electry::ElectryEngine::firstMidiSoloStringNote : -1;
    const bool soloSelected = soloKey && isSoloStringSelected (soloIndex);
    const bool selected = soloSelected || (keyswitch
        && isKeyswitchSelected (midiNoteNumber - firstKeyboardNote));
    const auto groupColour = functionKeyColour (midiNoteNumber);
    const auto face = functionKey
        ? colours::rosewoodDark.interpolatedWith (groupColour, selected ? 0.27f : 0.16f)
        : colours::rosewoodDark;

    graphics.setColour (juce::Colours::black.withAlpha (0.42f));
    graphics.fillRect (area.translated (1.2f, 2.0f));
    graphics.setGradientFill ({ face.brighter (0.07f),
                                area.getCentreX(), area.getY(),
                                functionKey ? face.darker (0.24f) : colours::ebony,
                                area.getCentreX(), area.getBottom(), false });
    graphics.fillRect (area);
    // The bevel is confined to the front lip so adjacent raised keys remain
    // clearly distinct without glossy highlights over the function labels.
    graphics.setColour (colours::nickel.withAlpha (deadZone ? 0.04f : 0.11f));
    graphics.fillRect (area.reduced (1.0f, 0.0f)
                          .withTop (area.getBottom() - 5.0f));
    if (isOver && ! deadZone)
    {
        graphics.setColour (groupColour.withAlpha (0.12f));
        graphics.fillRect (area);
    }
    if (isDown && ! deadZone)
    {
        graphics.setColour (functionKey ? groupColour.withAlpha (0.28f)
                                       : colours::accentDark.withAlpha (0.84f));
        graphics.fillRect (area);
    }
    graphics.setColour (colours::nickel.withAlpha (deadZone ? 0.08f : 0.26f));
    graphics.drawLine (area.getX() + 0.6f, area.getY(),
                       area.getX() + 0.6f, area.getBottom() - 1.0f, 0.6f);
    if (functionKey || (isDown && ! deadZone))
    {
        graphics.setColour (groupColour.withAlpha (selected || isDown ? 1.0f : 0.50f));
        graphics.fillRect (area.reduced (0.6f, 0.0f)
                              .withTop (area.getBottom() - 2.0f));
    }

    if (keyswitch)
        drawKeyswitchDecoration (graphics, area, midiNoteNumber - firstKeyboardNote,
                                 selected, true);
    else if (vibratoGesture)
        drawVibratoDecoration (graphics, area, isDown);
    else if (soloKey)
        drawSoloDecoration (graphics, area, soloIndex, soloSelected, true);
    else if (soloClr)
        drawSoloClearDecoration (graphics, area, true, isDown);
}

// ---------------------------------------------------------------------------
// Choice strip
// ---------------------------------------------------------------------------

void ElectryTextButton::paintButton (juce::Graphics& graphics,
                                     bool isHighlighted, bool isDown)
{
    if (isEnabled())
    {
        juce::TextButton::paintButton (graphics, isHighlighted, isDown);
        return;
    }

    graphics.beginTransparencyLayer (0.5f);
    juce::TextButton::paintButton (graphics, isHighlighted, isDown);
    graphics.endTransparencyLayer();
}

bool ElectryTextButton::keyPressed (const juce::KeyPress& key)
{
    if (isEnabled() && onNavigation != nullptr
        && (key.isKeyCode (juce::KeyPress::leftKey)
            || key.isKeyCode (juce::KeyPress::rightKey)
            || key.isKeyCode (juce::KeyPress::upKey)
            || key.isKeyCode (juce::KeyPress::downKey)))
        return onNavigation (key);

    return juce::TextButton::keyPressed (
        key.isKeyCode (juce::KeyPress::spaceKey)
            ? juce::KeyPress { juce::KeyPress::returnKey }
            : key);
}

ElectryChoiceStrip::ElectryChoiceStrip (juce::String title,
                                        juce::StringArray choices,
                                        int maximumColumns,
                                        juce::String accessibilityTitle)
    : titleText (std::move (title)),
      maxColumns (juce::jmax (1, maximumColumns))
{
    const auto choiceContext = accessibilityTitle.isNotEmpty()
        ? std::move (accessibilityTitle)
        : titleText;
    setTitle (choiceContext);

    for (int index = 0; index < choices.size(); ++index)
    {
        auto button = std::make_unique<ElectryTextButton> (choices[index]);
        button->setTitle (choiceContext + ": " + choices[index]);
        button->setClickingTogglesState (true);
        button->setRadioGroupId (1, juce::dontSendNotification);
        button->setHasFocusOutline (true);
        button->onClick = [this, index]
        {
            activateChoice (index);
        };
        button->onNavigation = [this, index] (const juce::KeyPress& key)
        {
            const auto count = static_cast<int> (buttons.size());
            if (count < 2)
                return false;

            const int direction = key.isKeyCode (juce::KeyPress::leftKey)
                               || key.isKeyCode (juce::KeyPress::upKey)
                ? -1 : 1;
            int next = index;
            for (int attempt = 1; attempt < count; ++attempt)
            {
                next = (next + direction + count) % count;
                if (buttons[static_cast<std::size_t> (next)]->isEnabled())
                {
                    activateChoice (next);
                    return true;
                }
            }
            return false;
        };
        addAndMakeVisible (*button);
        buttons.push_back (std::move (button));
    }
    if (! buttons.empty())
        setSelectedIndex (0);
}

std::unique_ptr<juce::AccessibilityHandler>
ElectryChoiceStrip::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler> (
        *this, juce::AccessibilityRole::group);
}

void ElectryChoiceStrip::activateChoice (int index)
{
    if (index < 0 || index >= static_cast<int> (buttons.size()))
        return;

    auto& target = *buttons[static_cast<std::size_t> (index)];
    if (! target.isEnabled())
        return;

    setSelectedIndex (index);
    if (target.isShowing())
        target.grabKeyboardFocus();
    if (onChoice != nullptr)
        onChoice (index);
}

void ElectryChoiceStrip::setSelectedIndex (int newIndex)
{
    selectedIndex = juce::jlimit (0, static_cast<int> (buttons.size()) - 1, newIndex);
    for (int index = 0; index < static_cast<int> (buttons.size()); ++index)
    {
        auto& button = *buttons[static_cast<std::size_t> (index)];
        const bool selected = index == selectedIndex;
        button.setToggleState (selected, juce::dontSendNotification);
        button.setWantsKeyboardFocus (selected);
    }
}

void ElectryChoiceStrip::setTooltipText (const juce::String& text)
{
    for (auto& button : buttons)
        button->setTooltip (text);
}

void ElectryChoiceStrip::paint (juce::Graphics& graphics)
{
    if (titleText.isEmpty())
        return;

    graphics.setColour (colours::dimText.withMultipliedAlpha (isEnabled() ? 1.0f : 0.5f));
    graphics.setFont (juce::Font (juce::FontOptions (10.8f, juce::Font::bold))
                          .withExtraKerningFactor (0.07f));
    graphics.drawFittedText (titleText,
                             getLocalBounds().removeFromTop (17).reduced (3, 0),
                             juce::Justification::centredLeft, 1, 0.94f);
}

void ElectryChoiceStrip::resized()
{
    auto area = getLocalBounds();
    if (titleText.isNotEmpty())
        area.removeFromTop (19);
    if (buttons.empty())
        return;

    constexpr int gap = 4;
    const int buttonCount = static_cast<int> (buttons.size());
    const int rowCount = (buttonCount + maxColumns - 1) / maxColumns;
    const int usableHeight = juce::jmax (0, area.getHeight() - gap * (rowCount - 1));

    for (int index = 0; index < buttonCount; ++index)
    {
        const int row = index / maxColumns;
        const int firstInRow = row * maxColumns;
        const int columnsInRow = juce::jmin (maxColumns, buttonCount - firstInRow);
        const int usableWidth = juce::jmax (0,
            area.getWidth() - gap * (columnsInRow - 1));
        const int column = index - firstInRow;
        // Divide edge positions, rather than truncating each button's size,
        // so every row closes cleanly against its panel at any host scale.
        const int left = column * usableWidth / columnsInRow + column * gap;
        const int right = (column + 1) * usableWidth / columnsInRow + column * gap;
        const int top = row * usableHeight / rowCount + row * gap;
        const int bottom = (row + 1) * usableHeight / rowCount + row * gap;
        buttons[static_cast<std::size_t> (index)]->setBounds (
            area.getX() + left, area.getY() + top, right - left, bottom - top);
    }
}

// ---------------------------------------------------------------------------
// Knob
// ---------------------------------------------------------------------------

ElectryKnob::ElectryKnob (juce::String name)
{
    setName (name + " control");
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 78, 20);
    slider.setName (name);
    slider.setTitle (name);
    slider.setWantsKeyboardFocus (true);
    slider.setHasFocusOutline (true);
    addAndMakeVisible (slider);

    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (11.5f, juce::Font::bold));
    label.setColour (juce::Label::textColourId, colours::dimText);
    label.getProperties().set ("electryControlCaption", true);
    label.setMinimumHorizontalScale (0.94f);
    label.setAccessible (false);
    addAndMakeVisible (label);
}

void ElectryKnob::parentHierarchyChanged()
{
    // The slider is constructed before this knob joins the editor. JUCE does
    // not recreate its text box merely because the inherited LookAndFeel
    // changed with that parent, so install the instrument's editable label
    // now rather than depending on a later text-box width change to do it.
    slider.lookAndFeelChanged();
}

void ElectryKnob::resized()
{
    const auto visualWeight = static_cast<float> (
        slider.getProperties().getWithDefault (visualWeightProperty, 0.82f));
    const bool compact = visualWeight < compactKnobWeight;
    const bool hero = visualWeight >= 0.9f;

    auto area = getLocalBounds();
    constexpr int labelHeight = 26;
    label.setFont (juce::Font (juce::FontOptions (
        hero ? 11.5f : (compact ? 10.5f : 11.0f), juce::Font::bold))
                       .withExtraKerningFactor (0.025f));
    label.setColour (juce::Label::textColourId,
                     hero ? colours::binding : colours::dimText);
    label.setBounds (area.removeFromTop (labelHeight).reduced (2, 0));
    area.removeFromTop (2);
    slider.setTextBoxStyle (
        juce::Slider::TextBoxBelow, false,
        juce::jlimit (48, hero ? 88 : 78, juce::jmax (48, getWidth() - 4)),
        20);
    slider.setBounds (area);
}

// ---------------------------------------------------------------------------
// Status display
// ---------------------------------------------------------------------------

void ElectryStatusDisplay::setStatus (int activeVoices, int sympatheticStrings,
                                      bool ready, double sampleRate,
                                      int midiMutePressure, int vibratoGesture,
                                      int tremoloGesture,
                                      bool scheduleRepaint)
{
    midiMutePressure = juce::jlimit (0, 127, midiMutePressure);
    vibratoGesture = juce::jlimit (0, 127, vibratoGesture);
    tremoloGesture = juce::jlimit (0, 127, tremoloGesture);
    if (voices == activeVoices && sympathetic == sympatheticStrings
        && isReady == ready && juce::approximatelyEqual (rate, sampleRate)
        && mutePressure == midiMutePressure && vibrato == vibratoGesture
        && tremolo == tremoloGesture)
        return;
    voices = activeVoices;
    sympathetic = sympatheticStrings;
    isReady = ready;
    rate = sampleRate;
    mutePressure = midiMutePressure;
    vibrato = vibratoGesture;
    tremolo = tremoloGesture;
    const auto status = getStatusText();
    if (status != getTitle())
    {
        setTitle (status);
        if (auto* handler = getAccessibilityHandler())
            handler->notifyAccessibilityEvent (juce::AccessibilityEvent::titleChanged);
    }
    if (scheduleRepaint)
        repaint();
}

juce::String ElectryStatusDisplay::getStatusText() const
{
    if (! isReady)
        return "ENGINE STANDBY";

    juce::String status = juce::String (voices)
                        + (voices == 1 ? " STRING" : " STRINGS");
    if (sympathetic > 0)
        status += " +" + juce::String (sympathetic) + " RING";
    if (vibrato > 0)
        status += "  |  VIB " + juce::String (juce::roundToInt (
            100.0f * static_cast<float> (vibrato) / 127.0f)) + "%";
    if (tremolo > 0)
        status += "  |  TRM " + juce::String (juce::roundToInt (
            100.0f * static_cast<float> (tremolo) / 127.0f)) + "%";
    if (vibrato == 0 && tremolo == 0)
    {
        if (mutePressure > 0)
            status += "  |  CC2 MUTE +" + juce::String (juce::roundToInt (
                100.0f * static_cast<float> (mutePressure) / 127.0f)) + "%";
        else if (rate > 0.0)
            status += "  |  " + juce::String (rate / 1000.0, 1) + " kHz";
    }
    return status;
}

std::unique_ptr<juce::AccessibilityHandler>
ElectryStatusDisplay::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler> (
        *this, juce::AccessibilityRole::staticText);
}

void ElectryStatusDisplay::paint (juce::Graphics& graphics)
{
    auto bounds = getLocalBounds().toFloat();
    graphics.setColour (juce::Colours::black.withAlpha (0.62f));
    graphics.fillRoundedRectangle (bounds, 5.0f);
    graphics.setColour (colours::panelOutline.withAlpha (0.52f));
    graphics.drawRoundedRectangle (bounds.reduced (0.5f), 5.0f, 1.0f);

    graphics.setColour (isReady ? colours::accentBright : colours::dimText);
    graphics.fillEllipse (8.0f, bounds.getCentreY() - 1.8f, 3.6f, 3.6f);
    graphics.setFont (juce::FontOptions (11.5f, juce::Font::bold));
    graphics.drawFittedText (getStatusText(),
                             getLocalBounds().withTrimmedLeft (18).reduced (0, 1),
                             juce::Justification::centredLeft, 1, 0.72f);
}

// ---------------------------------------------------------------------------
// Fretboard display
// ---------------------------------------------------------------------------

ElectryFretboardDisplay::ElectryFretboardDisplay()
{
    setInterceptsMouseClicks (true, false);
    setName ("Fretboard display");
    setWantsKeyboardFocus (true);
    setMouseClickGrabsKeyboardFocus (true);
    setHasFocusOutline (true);
    setHelpText ("Use Up and Down or number keys 1 through 8 to select a "
                 "physical string, then press Space or Return to repick it.");
    setTooltip ("Click any held string row, or use Up/Down or 1-8 then "
                "Space/Return, for one hard repick. Host MIDI "
                "E7 through B7 provides the same eight-string trigger lane "
                "with velocity control.");
    selectString (0);
}

void ElectryFretboardDisplay::selectString (int stringIndex)
{
    const int next = juce::jlimit (
        0, electry::ElectryEngine::stringCount - 1, stringIndex);
    if (next == selectedString)
        return;

    selectedString = next;
    updateAccessibilityTitle();
    repaint();
}

void ElectryFretboardDisplay::updateAccessibilityTitle()
{
    if (selectedString < 0
        || selectedString >= electry::ElectryEngine::stringCount)
        return;

    const auto& state = rows[static_cast<std::size_t> (selectedString)].state;
    juce::String title = juce::String ("Live fretboard: physical string ")
                       + juce::String (
                           electry::ElectryEngine::stringCount - selectedString)
                       + " selected, ";
    if (state.sounding)
    {
        if (state.fret == 0)
            title += "open ";
        else if (state.fret > 0)
            title += "fret " + juce::String (state.fret) + " ";
        if (state.midiNote >= 0)
            title += juce::MidiMessage::getMidiNoteName (
                state.midiNote, true, true, 4) + ", ";
        title += (state.strokeUp ? "upstroke, " : "downstroke, ");
        // `sounding` also covers a MIDI-held string whose delayed attack has
        // not begun or whose voice has already retired. "Held" is the exact
        // invariant and does not promise audible output in either case.
        title += (state.releasing ? "releasing" : "held");
    }
    else if (state.sympathetic)
    {
        if (state.midiNote >= 0)
            title += "open " + juce::MidiMessage::getMidiNoteName (
                state.midiNote, true, true, 4) + ", ";
        title += "sympathetic ring";
    }
    else
    {
        title += "silent";
    }
    title += ".";
    setTitle (title);
    if (auto* handler = getAccessibilityHandler())
        handler->notifyAccessibilityEvent (juce::AccessibilityEvent::titleChanged);
}

int ElectryFretboardDisplay::stringAtY (float y) const noexcept
{
    const auto height = static_cast<float> (getHeight());
    if (height <= 0.0f || y < 0.0f || y >= height)
        return -1;

    constexpr float inset = 0.085f;
    const auto first = electry::visuals::stringRowFraction (
        0, electry::ElectryEngine::stringCount, inset);
    const auto second = electry::visuals::stringRowFraction (
        1, electry::ElectryEngine::stringCount, inset);
    const auto spacing = second - first;
    const auto fraction = y / height;
    const int stringIndex = juce::roundToInt ((fraction - first) / spacing);
    if (stringIndex < 0
        || stringIndex >= electry::ElectryEngine::stringCount)
        return -1;

    const auto row = electry::visuals::stringRowFraction (
        stringIndex, electry::ElectryEngine::stringCount, inset);
    return std::abs (fraction - row) <= spacing * 0.48f ? stringIndex : -1;
}

void ElectryFretboardDisplay::mouseMove (const juce::MouseEvent& event)
{
    const int next = stringAtY (event.position.y);
    if (next == hoveredString)
        return;

    hoveredString = next;
    setMouseCursor (next >= 0 ? juce::MouseCursor::PointingHandCursor
                              : juce::MouseCursor::NormalCursor);
    repaint();
}

void ElectryFretboardDisplay::mouseExit (const juce::MouseEvent&)
{
    if (hoveredString < 0)
        return;

    hoveredString = -1;
    setMouseCursor (juce::MouseCursor::NormalCursor);
    repaint();
}

void ElectryFretboardDisplay::mouseDown (const juce::MouseEvent& event)
{
    if (! event.mods.isLeftButtonDown())
        return;

    const int stringIndex = stringAtY (event.position.y);
    if (stringIndex >= 0)
    {
        selectString (stringIndex);
        if (onRepick)
            onRepick (stringIndex);
    }
}

bool ElectryFretboardDisplay::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::upKey)
        selectString (selectedString - 1);
    else if (key == juce::KeyPress::downKey)
        selectString (selectedString + 1);
    else if (key.getKeyCode() >= '1' && key.getKeyCode() <= '8')
        selectString (electry::ElectryEngine::stringCount
                      - (key.getKeyCode() - '0'));
    else if (key == juce::KeyPress::spaceKey
             || key == juce::KeyPress::returnKey)
    {
        if (onRepick)
            onRepick (selectedString);
        return true;
    }
    else
    {
        return false;
    }

    return true;
}

std::unique_ptr<juce::AccessibilityHandler>
ElectryFretboardDisplay::createAccessibilityHandler()
{
    auto actions = juce::AccessibilityActions().addAction (
        juce::AccessibilityActionType::press,
        [this]
        {
            if (onRepick)
                onRepick (selectedString);
        });
    return std::make_unique<juce::AccessibilityHandler> (
        *this, juce::AccessibilityRole::button, std::move (actions));
}

bool ElectryFretboardDisplay::refresh (const ElectryAudioProcessor& processor,
                                       float frameSeconds)
{
    bool moving = false;
    bool selectedStateChanged = false;
    for (int stringIndex = 0;
         stringIndex < electry::ElectryEngine::stringCount; ++stringIndex)
    {
        auto& row = rows[static_cast<std::size_t> (stringIndex)];
        const auto next = processor.getStringVisualState (stringIndex);
        const bool changed = next.midiNote != row.state.midiNote
                          || next.fret != row.state.fret
                          || next.sounding != row.state.sounding
                          || next.sympathetic != row.state.sympathetic
                          || next.releasing != row.state.releasing
                          || next.strokeUp != row.state.strokeUp;
        selectedStateChanged = selectedStateChanged
                            || (stringIndex == selectedString && changed);
        row.state = next;

        const float previousLevel = row.level;
        row.level = electry::visuals::meterBallistics (row.level, next.level,
                                                       0.55f, 0.18f);
        if (row.level > 0.004f)
        {
            // A visible wobble rate per string. It is deliberately not the
            // audio pitch, which would alias into nonsense at any frame rate a
            // GUI can sustain; it only has to read as "this string is moving".
            row.phase += juce::MathConstants<float>::twoPi
                       * (4.5f + 0.8f * static_cast<float> (stringIndex))
                       * frameSeconds;
            while (row.phase > juce::MathConstants<float>::twoPi)
                row.phase -= juce::MathConstants<float>::twoPi;
            moving = true;
        }
        else if (previousLevel > 0.0f)
        {
            row.level = 0.0f;
            row.phase = 0.0f;
            moving = true;
        }

        moving = moving || changed;
    }
    const auto currentSoloMask = processor.getSoloStringMask();
    const bool soloMaskChanged = currentSoloMask != soloMask;
    soloMask = currentSoloMask;
    moving = moving || soloMaskChanged;
    if (selectedStateChanged)
        updateAccessibilityTitle();
    return moving;
}

void ElectryFretboardDisplay::paint (juce::Graphics& graphics)
{
    auto bounds = getLocalBounds().toFloat();
    if (bounds.getWidth() < 120.0f || bounds.getHeight() < 40.0f)
        return;

    // These partitions and row coordinates also define the established
    // physical-string interaction surface. Styling does not move a hit target.
    auto tuningArea = bounds.removeFromLeft (42.0f);
    auto meterArea = bounds.removeFromRight (54.0f);
    bounds.removeFromLeft (4.0f);
    meterArea.removeFromLeft (8.0f);
    const auto neck = bounds;
    const auto neckX = neck.getX();
    const auto neckWidth = neck.getWidth();
    const auto neckSpan = electry::visuals::fretSpan (lastDrawnFret);
    const auto fretX = [neckX, neckWidth, neckSpan] (int fret)
    {
        return neckX + neckWidth
             * electry::visuals::fretWireFraction (fret, lastDrawnFret, neckSpan);
    };

    juce::Path fingerboard;
    constexpr float bevel = 3.0f;
    fingerboard.startNewSubPath (neck.getX(), neck.getY() + bevel);
    fingerboard.lineTo (neck.getX() + bevel, neck.getY());
    fingerboard.lineTo (neck.getRight() - bevel, neck.getY());
    fingerboard.lineTo (neck.getRight(), neck.getY() + bevel);
    fingerboard.lineTo (neck.getRight(), neck.getBottom() - bevel);
    fingerboard.lineTo (neck.getRight() - bevel, neck.getBottom());
    fingerboard.lineTo (neck.getX() + bevel, neck.getBottom());
    fingerboard.lineTo (neck.getX(), neck.getBottom() - bevel);
    fingerboard.closeSubPath();
    graphics.setGradientFill ({ colours::rosewoodDark, neck.getCentreX(), neck.getY(),
                                colours::ebony, neck.getCentreX(),
                                neck.getBottom(), false });
    graphics.fillPath (fingerboard);
    graphics.setColour (colours::panelOutline.withAlpha (0.60f));
    graphics.strokePath (fingerboard, juce::PathStrokeType (0.7f));
    graphics.setColour (colours::nickel.withAlpha (0.11f));
    graphics.drawLine (neck.getX() + bevel, neck.getY() + 1.0f,
                       neck.getRight() - bevel, neck.getY() + 1.0f, 0.7f);

    const auto firstRow = electry::visuals::stringRowFraction (
        0, electry::ElectryEngine::stringCount, 0.085f);
    const auto secondRow = electry::visuals::stringRowFraction (
        1, electry::ElectryEngine::stringCount, 0.085f);
    const auto rowSpacing = neck.getHeight() * (secondRow - firstRow);
    const auto rowBounds = [&] (int stringIndex)
    {
        const auto rowY = neck.getY() + neck.getHeight()
            * electry::visuals::stringRowFraction (
                  stringIndex, electry::ElectryEngine::stringCount, 0.085f);
        return juce::Rectangle<float> (0.0f, rowY - rowSpacing * 0.46f,
                                       static_cast<float> (getWidth()),
                                       rowSpacing * 0.92f);
    };

    for (int stringIndex = 0; stringIndex < electry::ElectryEngine::stringCount;
         ++stringIndex)
    {
        const bool selected = stringIndex == selectedString;
        const bool soloed = soloMask != 0 && (soloMask & (1u << stringIndex)) != 0;
        const bool hovered = stringIndex == hoveredString;
        if (! (selected || soloed || hovered))
            continue;
        const auto row = rowBounds (stringIndex);
        const bool focused = selected && hasKeyboardFocus (true);
        graphics.setColour (colours::accentBright.withAlpha (
            focused ? 0.12f : selected || soloed ? 0.075f : 0.045f));
        graphics.fillRect (row);
        graphics.setColour (colours::accentBright.withAlpha (
            focused ? 0.95f : selected || soloed ? 0.68f : 0.28f));
        graphics.fillRect (row.withWidth (2.0f));
        if (focused)
            graphics.drawRect (row.reduced (0.5f), 0.8f);
    }

    // Slender split-diamond inlays: the established fret positions remain
    // legible, while the neck reads as a precise ebony-and-titanium surface.
    const auto drawInlay = [&graphics, &fretX, &neck] (int fret, bool doubled)
    {
        const auto x = 0.5f * (fretX (fret - 1) + fretX (fret));
        const auto halfWidth = juce::jmin (2.5f, (fretX (fret) - fretX (fret - 1)) * 0.22f);
        const auto halfHeight = juce::jmin (4.3f, neck.getHeight() * 0.045f);
        const auto drawDiamond = [&] (float y)
        {
            juce::Path inlay;
            inlay.startNewSubPath (x, y - halfHeight);
            inlay.lineTo (x + halfWidth, y);
            inlay.lineTo (x, y + halfHeight);
            inlay.lineTo (x - halfWidth, y);
            inlay.closeSubPath();
            graphics.setColour (colours::nickel.withAlpha (0.25f));
            graphics.fillPath (inlay);
            graphics.setColour (colours::ebony.withAlpha (0.75f));
            graphics.drawLine (x, y - halfHeight, x, y + halfHeight, 0.7f);
        };
        if (doubled)
        {
            drawDiamond (neck.getCentreY() - neck.getHeight() * 0.24f);
            drawDiamond (neck.getCentreY() + neck.getHeight() * 0.24f);
        }
        else
            drawDiamond (neck.getCentreY());
    };
    for (const int fret : electry::visuals::inlayFrets)
        drawInlay (fret, false);
    drawInlay (electry::visuals::octaveInlayFret, true);
    drawInlay (electry::visuals::upperOctaveInlayFret, true);

    graphics.setColour (colours::nickel.withAlpha (0.86f));
    graphics.fillRect (neck.getX(), neck.getY() + 2.0f, 2.4f, neck.getHeight() - 4.0f);
    graphics.setColour (colours::text.withAlpha (0.46f));
    graphics.fillRect (neck.getX() + 0.5f, neck.getY() + 2.0f, 0.6f,
                       neck.getHeight() - 4.0f);
    for (int fret = 1; fret <= lastDrawnFret; ++fret)
    {
        const auto x = fretX (fret);
        graphics.setColour (juce::Colours::black.withAlpha (0.40f));
        graphics.drawLine (x + 0.8f, neck.getY() + 2.0f,
                           x + 0.8f, neck.getBottom() - 2.0f, 1.0f);
        graphics.setColour (colours::fretWire.withAlpha (0.45f));
        graphics.drawLine (x, neck.getY() + 2.0f, x,
                           neck.getBottom() - 2.0f, 0.7f);
    }

    static constexpr std::array<const char*, electry::ElectryEngine::stringCount>
        tuningNames { "E1", "B1", "E2", "A2", "D3", "G3", "B3", "E4" };

    for (int stringIndex = 0;
         stringIndex < electry::ElectryEngine::stringCount; ++stringIndex)
    {
        const auto& row = rows[static_cast<std::size_t> (stringIndex)];
        const auto y = neck.getY() + neck.getHeight()
            * electry::visuals::stringRowFraction (
                  stringIndex, electry::ElectryEngine::stringCount, 0.085f);
        const auto thickness = electry::visuals::stringThickness (
            stringIndex, 0.65f, 2.15f);
        const auto heat = electry::visuals::levelHeat (row.level);
        const bool ringing = row.state.sounding || row.state.sympathetic;
        const bool soloed = soloMask != 0 && (soloMask & (1u << stringIndex)) != 0;
        const auto activeColour = row.state.sympathetic ? colours::sympatheticRing
            : row.state.releasing ? colours::accent : colours::accentBright;
        const auto stoppedFraction = row.state.sounding && row.state.fret > 0
            ? electry::visuals::fretWireFraction (row.state.fret, lastDrawnFret,
                                                 neckSpan)
            : 0.0f;
        const auto swing = std::sin (row.phase) * heat
                         * juce::jmin (4.0f, neck.getHeight() * 0.045f);

        juce::Path string;
        constexpr int steps = 40;
        for (int step = 0; step <= steps; ++step)
        {
            const auto u = static_cast<float> (step) / static_cast<float> (steps);
            const auto x = neck.getX() + neckWidth * u;
            const auto offset = ringing ? swing
                * electry::visuals::vibrationShape (u, stoppedFraction) : 0.0f;
            if (step == 0)
                string.startNewSubPath (x, y + offset);
            else
                string.lineTo (x, y + offset);
        }
        graphics.setColour (juce::Colours::black.withAlpha (0.55f));
        graphics.strokePath (string, juce::PathStrokeType (thickness + 1.4f));
        graphics.setColour (colours::nickel.withAlpha (ringing ? 0.20f : 0.55f));
        graphics.strokePath (string, juce::PathStrokeType (thickness));
        if (ringing)
        {
            // Only the bridge-side speaking section carries the ember path;
            // the finger-to-nut segment remains the same quiet metal string.
            juce::Graphics::ScopedSaveState save (graphics);
            graphics.reduceClipRegion (juce::Rectangle<float> (
                neckX + neckWidth * stoppedFraction, neck.getY(),
                neckWidth * (1.0f - stoppedFraction), neck.getHeight())
                    .getSmallestIntegerContainer());
            graphics.setColour (activeColour.withAlpha (0.10f + 0.12f * heat));
            graphics.strokePath (string, juce::PathStrokeType (thickness + 3.0f));
            graphics.setColour (activeColour.withAlpha (0.60f + 0.40f * heat));
            graphics.strokePath (string, juce::PathStrokeType (thickness));
        }
        else if (thickness > 1.1f)
        {
            graphics.setColour (colours::text.withAlpha (0.14f));
            graphics.drawLine (neckX, y - thickness * 0.23f, neck.getRight(),
                               y - thickness * 0.23f, 0.45f);
        }

        if (row.state.sounding && row.state.midiNote >= 0 && row.state.fret > 0)
        {
            const auto x = neckX + neckWidth
                * electry::visuals::fretCentreFraction (row.state.fret,
                                                        lastDrawnFret, neckSpan);
            const auto halfHeight = juce::jmin (6.0f, rowSpacing * 0.46f);
            constexpr float halfWidth = 10.0f;
            constexpr float cut = 2.0f;
            juce::Path marker;
            marker.startNewSubPath (x - halfWidth + cut, y - halfHeight);
            marker.lineTo (x + halfWidth, y - halfHeight);
            marker.lineTo (x + halfWidth, y + halfHeight - cut);
            marker.lineTo (x + halfWidth - cut, y + halfHeight);
            marker.lineTo (x - halfWidth, y + halfHeight);
            marker.lineTo (x - halfWidth, y - halfHeight + cut);
            marker.closeSubPath();
            graphics.setColour (colours::ebony);
            graphics.fillPath (marker);
            graphics.setColour (activeColour.withAlpha (0.9f));
            graphics.strokePath (marker, juce::PathStrokeType (0.8f));
            graphics.setColour (colours::text);
            graphics.setFont (juce::FontOptions (8.5f, juce::Font::bold));
            graphics.drawText (
                juce::MidiMessage::getMidiNoteName (row.state.midiNote, true, false, 4),
                juce::Rectangle<float> (x - halfWidth, y - halfHeight,
                                        halfWidth * 2.0f, halfHeight * 2.0f),
                juce::Justification::centred);
        }

        auto labelBounds = juce::Rectangle<float> (
            tuningArea.getX(), y - 6.0f, tuningArea.getWidth(), 12.0f);
        auto stringNumberBounds = labelBounds.removeFromLeft (14.0f);
        graphics.setColour (stringIndex == selectedString || soloed
                                ? colours::accentBright : colours::dimText);
        graphics.setFont (juce::FontOptions (8.8f, juce::Font::bold));
        graphics.drawText (juce::String (
                               electry::ElectryEngine::stringCount - stringIndex),
                           stringNumberBounds, juce::Justification::centred);
        graphics.setColour (ringing ? colours::text : colours::dimText);
        graphics.setFont (juce::FontOptions (10.2f, juce::Font::bold));
        graphics.drawText (tuningNames[static_cast<std::size_t> (stringIndex)],
                           labelBounds, juce::Justification::centredRight);

        const juce::Rectangle<float> meterTrack (
            meterArea.getX(), y - 1.5f, meterArea.getWidth(), 3.0f);
        graphics.setColour (colours::panelOutline.withAlpha (0.28f));
        graphics.fillRect (meterTrack);
        if (heat > 0.01f)
        {
            graphics.setColour (activeColour.withAlpha (0.90f));
            graphics.fillRect (meterTrack.withWidth (
                juce::jmax (2.0f, meterTrack.getWidth() * heat)));
        }
        // Three quiet cuts make the activity rail readable at a glance without
        // introducing another animation or a decorative noise texture.
        graphics.setColour (colours::ebony);
        for (int divider = 1; divider < 4; ++divider)
            graphics.fillRect (meterTrack.getX() + meterTrack.getWidth()
                * static_cast<float> (divider) * 0.25f, meterTrack.getY(),
                1.0f, meterTrack.getHeight());
    }
}

// ---------------------------------------------------------------------------
// Editor
// ---------------------------------------------------------------------------

ElectryAudioProcessorEditor::ElectryAudioProcessorEditor (ElectryAudioProcessor& p)
    : AudioProcessorEditor (&p),
      electryProcessor (p),
      keyboard (p.keyboardState)
{
    setLookAndFeel (&lookAndFeel);

    logoLabel.setText ("ELECTRY", juce::dontSendNotification);
    logoLabel.setFont (juce::Font (juce::FontOptions (37.0f, juce::Font::bold | juce::Font::italic))
                           .withExtraKerningFactor (0.08f));
    logoLabel.setColour (juce::Label::textColourId, colours::text);
    logoLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (logoLabel);

    editionLabel.setText ("EIGHT STRINGS  /  DROP E",
                          juce::dontSendNotification);
    editionLabel.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold))
                              .withExtraKerningFactor (0.16f));
    editionLabel.setColour (juce::Label::textColourId, colours::dimText);
    editionLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (editionLabel);

    for (int index = 0; index < electryProcessor.getNumPrograms(); ++index)
        factoryProgramSelector.addItem (
            electryProcessor.getProgramName (index), index + 1);
    factoryProgramSelector.setSelectedId (
        electryProcessor.getCurrentProgram() + 1, juce::dontSendNotification);
    factoryProgramSelector.setTooltip (
        "Rigs initialize guitar, FX, Mute Tightness and Mute Pressure. They "
        "never change the PICK STROKE or PLAY STYLE latches; choose Mute "
        "or Dead below.");
    factoryProgramSelector.setName ("Factory rig");
    factoryProgramSelector.setTitle ("Factory rig");
    factoryProgramSelector.setComponentID ("factoryProgram");
    factoryProgramSelector.setHasFocusOutline (true);
    factoryProgramSelector.onChange = [this]
    {
        const int index = factoryProgramSelector.getSelectedId() - 1;
        if (index >= 0)
            electryProcessor.setCurrentProgram (index);
    };
    addAndMakeVisible (factoryProgramSelector);

    addAndMakeVisible (statusDisplay);

    panicButton.setComponentID ("panic");
    panicButton.setHasFocusOutline (true);
    panicButton.setColour (juce::TextButton::buttonColourId,
                           colours::oxblood.darker (0.12f));
    panicButton.setTooltip ("Immediately silence all strings");
    panicButton.onClick = [this] { electryProcessor.requestPanic(); };
    addAndMakeVisible (panicButton);

    pickStyleStrip.onChoice = [this] (int index)
    {
        keyboard.setSelectedKeyswitches (
            index, electryProcessor.getEffectivePlayStyleIndex());
        electryProcessor.triggerArticulation (index);
    };
    pickStyleStrip.setTooltipText (
        "Picking hand: Down, Up or Alternate (C0 to D0). The pick stroke "
        "combines independently with every play style.");
    pickStyleStrip.setComponentID ("pickStyleStrip");
    addAndMakeVisible (pickStyleStrip);

    playStyleStrip.onChoice = [this] (int index)
    {
        keyboard.setSelectedKeyswitches (
            electryProcessor.getCurrentPickStyleIndex(),
            electryProcessor.getEffectivePlayStyleIndex());
        electryProcessor.triggerArticulation (
            electry::ElectryEngine::pickStyleKeyswitchCount + index);
    };
    playStyleStrip.setTooltipText (
        "Select the base style (D#0 to A0). Mute is the bridge hand; Dead is "
        "the fretting hand. The selected pick stroke still applies.");
    playStyleStrip.setComponentID ("playStyleStrip");
    addAndMakeVisible (playStyleStrip);

    playStyleKeyModeStrip.onChoice = [this] (int index)
    {
        electryProcessor.setPlayStyleKeysHold (index == 1);
    };
    playStyleKeyModeStrip.setTooltipText (
        "LATCH leaves MIDI play-style keys selected. HOLD uses them only while pressed, then returns to the PLAY STYLE choice.");
    playStyleKeyModeStrip.setComponentID ("playStyleKeyMode");
    addAndMakeVisible (playStyleKeyModeStrip);

    // The pickup selector strip binds to the choice parameter.
    pickupStrip.onChoice = [this] (int index)
    {
        if (auto* parameter = electryProcessor.parameters.getParameter (
                electry::parameters::pickupSelector))
        {
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost (
                parameter->convertTo0to1 (static_cast<float> (index)));
            parameter->endChangeGesture();
        }
    };
    if (auto* parameter = electryProcessor.parameters.getParameter (
            electry::parameters::pickupSelector))
    {
        pickupAttachment = std::make_unique<juce::ParameterAttachment> (
            *parameter,
            [this] (float newValue)
            {
                pickupStrip.setSelectedIndex (juce::roundToInt (newValue));
            },
            nullptr);
        pickupAttachment->sendInitialUpdate();
    }
    pickupStrip.setComponentID ("pickupSelector");
    addAndMakeVisible (pickupStrip);

    outputModeStrip.onChoice = [this] (int index)
    {
        auto* outputParameter = electryProcessor.parameters.getParameter (
            electry::parameters::outputMode);
        if (outputParameter == nullptr)
            return;

        outputParameter->beginChangeGesture();
        outputParameter->setValueNotifyingHost (
            outputParameter->convertTo0to1 (static_cast<float> (index)));
        outputParameter->endChangeGesture();
    };
    if (auto* parameter = electryProcessor.parameters.getParameter (
            electry::parameters::outputMode))
    {
        outputModeAttachment = std::make_unique<juce::ParameterAttachment> (
            *parameter,
            [this] (float newValue)
            {
                outputModeStrip.setSelectedIndex (juce::roundToInt (newValue));
            },
            nullptr);
        outputModeAttachment->sendInitialUpdate();
    }
    outputModeStrip.setTooltipText (
        "Mono is an authentic summed DI. Stereo spreads one guitar's eight "
        "strings across the stereo field. DOUBLE runs "
        "two independent Electry performances, one per channel. Choose DOUBLE "
        "before the phrase; it does not clone notes already ringing.");
    outputModeStrip.setComponentID (electry::parameters::outputMode);
    addAndMakeVisible (outputModeStrip);

    ampModelStrip.onChoice = [this] (int index)
    {
        auto* parameter = electryProcessor.parameters.getParameter (
            electry::parameters::ampModel);
        if (parameter == nullptr)
            return;

        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (
            parameter->convertTo0to1 (static_cast<float> (index)));
        parameter->endChangeGesture();
    };
    if (auto* parameter = electryProcessor.parameters.getParameter (
            electry::parameters::ampModel))
    {
        ampModelAttachment = std::make_unique<juce::ParameterAttachment> (
            *parameter,
            [this] (float newValue)
            {
                ampModelStrip.setSelectedIndex (juce::roundToInt (newValue));
            },
            nullptr);
        ampModelAttachment->sendInitialUpdate();
    }
    ampModelStrip.setTooltipText (
        "Selects the complete amplifier and cabinet voice: American clean, "
        "British crunch, or modern high-gain.");
    ampModelStrip.setComponentID (electry::parameters::ampModel);
    addAndMakeVisible (ampModelStrip);

    fxOversamplingStrip.onChoice = [this] (int index)
    {
        auto* parameter = electryProcessor.parameters.getParameter (
            electry::parameters::fxOversampling);
        if (parameter == nullptr)
            return;

        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (
            parameter->convertTo0to1 (static_cast<float> (index)));
        parameter->endChangeGesture();
    };
    if (auto* parameter = electryProcessor.parameters.getParameter (
            electry::parameters::fxOversampling))
    {
        fxOversamplingAttachment = std::make_unique<juce::ParameterAttachment> (
            *parameter,
            [this] (float newValue)
            {
                fxOversamplingStrip.setSelectedIndex (juce::roundToInt (newValue));
            },
            nullptr);
        fxOversamplingAttachment->sendInitialUpdate();
    }
    fxOversamplingStrip.setTooltipText (
        "STANDARD balances sound quality and CPU use. HIGH gives cleaner "
        "high-gain processing at a higher CPU cost. At 44.1/48 kHz they use "
        "4x and 8x oversampling; both adapt at higher sample rates.");
    fxOversamplingStrip.setComponentID (electry::parameters::fxOversampling);
    addAndMakeVisible (fxOversamplingStrip);

    fxOversamplingLabel.setText ("QUALITY", juce::dontSendNotification);
    fxOversamplingLabel.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    fxOversamplingLabel.setColour (juce::Label::textColourId,
                                    colours::binding.withAlpha (0.92f));
    fxOversamplingLabel.setJustificationType (juce::Justification::centredRight);
    fxOversamplingLabel.setAccessible (false);
    addAndMakeVisible (fxOversamplingLabel);

    using namespace electry::parameters;
    const auto setup = [this] (ElectryKnob& knob, const char* parameterId,
                               const char* tooltip)
    {
        const juce::String help = juce::String (tooltip)
                                + " Double-click to reset to its default.";
        knob.slider.setTooltip (help);
        knob.slider.setHelpText (help);
        knob.setComponentID (parameterId);
        addAndMakeVisible (knob);
        attachSlider (knob.slider, parameterId);
    };

#if ELECTRY_MEASURED_BODY_RESPONSE
    setup (guitarBuildKnob, guitarBuild,
           "Changes the body voice, scale length and string gauge together. "
           "Pickups and playing controls remain independent.");
    setup (bodyResonanceKnob, bodyResonance,
           "Amount of quiet material-dependent structural pickup colour; zero "
           "is an exact bypass");
#else
    setup (guitarBuildKnob, guitarBuild,
           "Changes the body voice, scale length and string gauge together. "
           "Pickups and playing controls remain independent.");
    setup (bodyResonanceKnob, bodyResonance,
           "How much solid-body structural colour reaches the pickups");
#endif
    setup (pickupTypeKnob, pickupType,
           "Pickup construction: wide humbucker toward narrow single coil");
    setup (toneKnob, tone, "Passive tone control loading the pickup resonance");
    setup (stringAgeKnob, stringAge, "String condition: fresh toward old and dead");
    setup (pickPositionKnob, pickPosition,
           "Picking spot: close to the bridge toward over the neck");
    setup (pickHardnessKnob, pickHardness,
           "Plectrum stiffness and edge: soft and round toward hard and sharp");
    setup (bendTimeKnob, bendTime,
           "Travel time of a pitch-wheel bend: how long the strings take to "
           "reach the wheel rather than snapping to it");
    setup (muteDampingKnob, muteDamping,
           "Loose half-mute toward tight metal chug for the E0 Mute "
           "play style");
    setup (velocityKnob, velocity, "How strongly MIDI velocity drives the pluck");
    setup (pickNoiseKnob, pickNoise, "Plectrum contact and scrape level");
    setup (fingerNoiseKnob, fingerNoise, "Fretting-hand contact level");
    setup (releaseNoiseKnob, releaseNoise, "String damping noise at note end");
    setup (artifactsKnob, artifacts,
           "Amount of subtle deterministic hardware ring, fret buzz, and incidental collision");
    setup (sympatheticKnob, sympathetic,
           "Bridge-coupled sympathetic ring of the strings you are not fingering. "
           "At 0% the coupled waveguides are bypassed exactly.");
    setup (palmMuteKnob, palmMute,
           "Continuous bridge-hand pressure across every play style, including "
           "Dead; MIDI CC2 adds to it while you play.");
    setup (strumSpreadKnob, strumSpread,
           "Mean pick travel time per string crossed. At 0 ms a chord starts as "
           "one block; any higher value groups cross-string arrivals for up to "
           "35 ms from the first and adds a 20 ms assembly pre-roll.");
    setup (tremoloRateKnob, tremoloRate,
           "Free-running picking speed while B0 TRM is held (not transport "
           "synced). 8, 12 and 16 strokes/s match the capture protocol; "
           "12 strokes/s equals 180 BPM sixteenth notes.");
    setup (resonanceKnob, resonanceDepth,
           "Full-scale reach of the modulation-wheel (CC1) resonance: how far "
           "the wheel can raise the sympathetic coupling and how much of the "
           "amplified output may feed back into the strings. At 100% a "
           "distorted tone self-resonates with the wheel up.");
    setup (outputKnob, output, "Master output level");
    setup (distortionKnob, distortion,
           "Drive through the oversampled diode pedal; 0% is true bypass");
    setup (ampKnob, amp,
           "Drive through the tube, transformer and cabinet path; 0% is true bypass");
    setup (compressorKnob, compressor, "Fast levelling for tight rhythm playing");
    setup (delayKnob, delay, "Tempo-neutral 360 ms lead delay");
    setup (roomKnob, room, "Compact stereo room ambience");

    fxEnableButton.setName ("FX enabled");
    fxEnableButton.setTitle ("FX enabled");
    fxEnableButton.setComponentID (electry::parameters::fxEnabled);
    fxEnableButton.setClickingTogglesState (true);
    fxEnableButton.setWantsKeyboardFocus (true);
    fxEnableButton.setHasFocusOutline (true);
    fxEnableButton.setTooltip (
        "Enable or bypass the complete effects chain. FX OFF plays the dry "
        "guitar and keeps your amplifier and effects settings for next time.");
    fxEnableButton.setHelpText (fxEnableButton.getTooltip());
    fxEnableButton.onClick = [this]
    {
        if (auto* parameter = electryProcessor.parameters.getParameter (
                electry::parameters::fxEnabled))
        {
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost (
                fxEnableButton.getToggleState() ? 1.0f : 0.0f);
            parameter->endChangeGesture();
        }
    };
    addAndMakeVisible (fxEnableButton);
    if (auto* parameter = electryProcessor.parameters.getParameter (
            electry::parameters::fxEnabled))
    {
        fxEnabledAttachment = std::make_unique<juce::ParameterAttachment> (
            *parameter,
            [this] (float enabled) { updateFxEnabledState (enabled >= 0.5f); },
            nullptr);
        fxEnabledAttachment->sendInitialUpdate();
    }

    fretboardDisplay.setComponentID ("fretboard");
    fretboardDisplay.onRepick = [this] (int stringIndex)
    {
        electryProcessor.triggerStringRepick (stringIndex);
    };
    addAndMakeVisible (fretboardDisplay);

    keyboard.setAvailableRange (firstKeyboardNote, lastKeyboardNote);
    keyboard.setLowestVisibleKey (firstKeyboardNote);
    keyboard.setScrollButtonsVisible (false);
    keyboard.setKeyWidth (24.0f);
    keyboard.setBlackNoteLengthProportion (0.64f);
    keyboard.setOctaveForMiddleC (4);
    keyboard.setTitle ("MIDI keyboard: " + juce::String (keyboardInstructions));
    keyboard.setHelpText (keyboardInstructions);
    keyboard.setComponentID ("keyboard");
    keyboard.setHasFocusOutline (true);
    addAndMakeVisible (keyboard);

    setSize (editorWidth, editorHeight);
    // The fretboard animates string motion, so the editor runs at a display
    // rate rather than the old status-only 12 Hz. Only the fretboard repaints,
    // and only while something is actually moving.
    startTimerHz (timerHz);

    // Populate the status readout and articulation strip immediately so the
    // panel opens in its real state instead of waiting up to a timer tick.
    timerCallback();
}

ElectryAudioProcessorEditor::~ElectryAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void ElectryAudioProcessorEditor::updateFxEnabledState (bool enabled)
{
    // Keep stored FX settings intact while exposing bypass as a single,
    // always-reachable control. Disabled descendants retain their canonical
    // accessible names and use the same dimmed treatment as other controls.
    const std::array<juce::Component*, 8> controls {
        &ampModelStrip, &fxOversamplingStrip, &fxOversamplingLabel,
        &distortionKnob, &ampKnob, &compressorKnob, &delayKnob, &roomKnob
    };
    bool returnFocus = false;
    for (auto* control : controls)
        returnFocus = returnFocus || (! enabled && control->hasKeyboardFocus (true));

    fxEnableButton.setToggleState (enabled, juce::dontSendNotification);
    fxEnableButton.setButtonText (enabled ? "FX ON" : "FX OFF");
    for (auto* control : controls)
        control->setEnabled (enabled);
    if (returnFocus && fxEnableButton.isShowing())
        fxEnableButton.grabKeyboardFocus();
}

void ElectryAudioProcessorEditor::attachSlider (juce::Slider& slider,
                                                const char* parameterId)
{
    // Double-click resets a knob to its parameter's own default rather than
    // to whatever JUCE's built-in fallback would pick, so it stays correct
    // for every knob without maintaining a second table of defaults here.
    if (auto* parameter = electryProcessor.parameters.getParameter (parameterId))
    {
        // Keep the compact panel label while exposing the complete canonical
        // parameter name to accessibility clients (for example, "Mute
        // pressure" rather than the visible "MUTE").
        const auto parameterName = parameter->getName (100);
        slider.setTitle (parameterName);
        slider.setTooltip (parameterName + ". " + slider.getTooltip());
        slider.setHelpText (slider.getTooltip());
        for (auto* child : slider.getChildren())
            if (auto* label = dynamic_cast<juce::Label*> (child);
                label != nullptr && label->isEditable())
                label->setTooltip (slider.getTooltip());
        slider.setDoubleClickReturnValue (
            true, parameter->convertFrom0to1 (parameter->getDefaultValue()));
    }

    sliderAttachments.push_back (std::make_unique<SliderAttachment> (
        electryProcessor.parameters, parameterId, slider));
}

void ElectryAudioProcessorEditor::timerCallback()
{
    statusDisplay.setStatus (electryProcessor.getActiveVoiceCount(),
                             electryProcessor.getSympatheticStringCount(),
                             electryProcessor.isEngineReady(),
                             electryProcessor.getCurrentSampleRateForDisplay(),
                             electryProcessor.getMidiMutePressureForDisplay(),
                             electryProcessor.getVibratoGestureForDisplay(),
                             electryProcessor.getTremoloGestureForDisplay());
    const int programId = electryProcessor.getCurrentProgram() + 1;
    if (factoryProgramSelector.getSelectedId() != programId)
        factoryProgramSelector.setSelectedId (programId, juce::dontSendNotification);
    const auto pickIndex = electryProcessor.getCurrentPickStyleIndex();
    const auto baseStyleIndex = electryProcessor.getCurrentPlayStyleIndex();
    const auto effectiveStyleIndex =
        electryProcessor.getEffectivePlayStyleIndex();
    pickStyleStrip.setSelectedIndex (pickIndex);
    playStyleStrip.setSelectedIndex (baseStyleIndex);
    playStyleKeyModeStrip.setSelectedIndex (
        electryProcessor.getPlayStyleKeysHold() ? 1 : 0);
    keyboard.setSelectedKeyswitches (pickIndex, effectiveStyleIndex);
    keyboard.setSoloStringMask (electryProcessor.getSoloStringMask());

    if (fretboardDisplay.refresh (electryProcessor, 1.0f / static_cast<float> (timerHz)))
        fretboardDisplay.repaint();
}

void ElectryAudioProcessorEditor::paint (juce::Graphics& graphics)
{
    const auto chassis = getLocalBounds().toFloat();
    juce::ColourGradient metal (juce::Colour (0xff20252b), 0.0f, 0.0f,
                                colours::background, chassis.getWidth() * 0.7f,
                                chassis.getHeight(), false);
    metal.addColour (0.25, juce::Colour (0xff101318));
    graphics.setGradientFill (metal);
    graphics.fillAll();

    // Fine satin grain lives in the chassis, not behind the control legends.
    // Vector strokes remain sharp at host display scales without a bitmap skin.
    for (int y = 1; y < getHeight(); y += 3)
    {
        graphics.setColour (juce::Colours::white.withAlpha (
            y % 9 == 1 ? 0.018f : 0.008f));
        graphics.drawHorizontalLine (y, 1.0f, chassis.getRight() - 1.0f);
    }
    graphics.setColour (colours::nickel.withAlpha (0.20f));
    graphics.drawRoundedRectangle (chassis.reduced (0.5f), 7.0f, 1.0f);
    graphics.setColour (juce::Colours::black.withAlpha (0.7f));
    graphics.drawRoundedRectangle (chassis.reduced (3.0f), 5.0f, 1.0f);

    // The cut-steel E mark and forward rake echo a plectrum's sharp edge.
    for (int bar = 0; bar < 3; ++bar)
    {
        const float y = 30.0f + 8.0f * static_cast<float> (bar);
        const float x = 24.0f - 2.0f * static_cast<float> (bar);
        juce::Path cut;
        cut.startNewSubPath (x + 3.0f, y);
        cut.lineTo (x + 26.0f - 3.0f * static_cast<float> (bar), y);
        cut.lineTo (x + 20.0f - 3.0f * static_cast<float> (bar), y + 5.0f);
        cut.lineTo (x, y + 5.0f); cut.closeSubPath();
        graphics.setColour (bar == 1 ? colours::text : colours::accentBright);
        graphics.fillPath (cut);
    }
    graphics.setColour (colours::nickel.withAlpha (0.17f));
    graphics.drawLine (307.0f, 28.0f, 307.0f, 72.0f, 1.0f);
    graphics.drawLine (18.0f, 84.0f, chassis.getRight() - 18.0f, 84.0f, 1.0f);
    graphics.setColour (colours::accentBright);
    graphics.drawLine (18.0f, 84.0f, 81.0f, 84.0f, 1.5f);

    const auto cutPanel = [] (juce::Rectangle<float> r)
    {
        constexpr float corner = 5.0f;
        juce::Path path;
        path.startNewSubPath (r.getX() + corner, r.getY());
        path.lineTo (r.getRight() - corner, r.getY());
        path.lineTo (r.getRight(), r.getY() + corner);
        path.lineTo (r.getRight(), r.getBottom() - corner);
        path.lineTo (r.getRight() - corner, r.getBottom());
        path.lineTo (r.getX() + corner, r.getBottom());
        path.lineTo (r.getX(), r.getBottom() - corner);
        path.lineTo (r.getX(), r.getY() + corner);
        path.closeSubPath();
        return path;
    };
    const std::array<const char*, sectionCount> titles {
        "", "FRETBOARD", "PERFORMANCE", "TONE & RESPONSE", "OUTPUT",
        "INSTRUMENT", "CONTACT & TEXTURE", "AMPLIFIER & FX"
    };
    for (int section = 0; section < sectionCount; ++section)
    {
        const auto bounds = sectionBounds[static_cast<std::size_t> (section)];
        if (bounds.isEmpty()) continue;
        const auto r = bounds.toFloat();
        graphics.setColour (juce::Colours::black.withAlpha (0.5f));
        graphics.fillPath (cutPanel (r.translated (0, 2)));
        juce::ColourGradient panelLight (colours::panelTop.interpolatedWith (
                                            colours::panel, 0.5f),
                                         r.getX(), r.getY(), colours::panel,
                                         r.getRight(), r.getBottom(), false);
        graphics.setGradientFill (panelLight);
        graphics.fillPath (cutPanel (r));
        graphics.setColour (colours::panelOutline.withAlpha (0.52f));
        graphics.strokePath (cutPanel (r.reduced (0.5f)), juce::PathStrokeType (1.0f));
        graphics.setColour (juce::Colours::white.withAlpha (0.055f));
        graphics.drawLine (r.getX() + 6, r.getY() + 1,
                           r.getRight() - 6, r.getY() + 1, 1.0f);

        if (titles[static_cast<std::size_t> (section)][0] == '\0') continue;
        const int captionHeight = section == effectsSection
            ? effectsHeaderHeight : sectionTitleHeight;
        const float dividerY = r.getY() + static_cast<float> (captionHeight) - 1.0f;
        graphics.setColour (colours::nickel.withAlpha (0.12f));
        graphics.drawLine (r.getX() + 14, dividerY,
                           r.getRight() - 14, dividerY, 0.8f);

        // Small engraved section symbols carry the same meaning as their text.
        juce::Path symbol;
        const float x = r.getX() + 14;
        const float y = r.getY() + static_cast<float> (captionHeight - 12) * 0.5f;
        if (section == fretboardSection || section == coreSection)
        {
            for (int i = 0; i < 3; ++i)
            {
                const float dx = static_cast<float> (i) * 4.0f;
                symbol.startNewSubPath (x + dx, y);
                symbol.lineTo (x + dx, y + 11);
                symbol.startNewSubPath (x + dx - 1.5f, y + 3 + dx * 0.5f);
                symbol.lineTo (x + dx + 1.5f, y + 3 + dx * 0.5f);
            }
        }
        else if (section == effectsSection)
        {
            symbol.startNewSubPath (x + 8, y); symbol.lineTo (x + 2, y + 6);
            symbol.lineTo (x + 7, y + 6); symbol.lineTo (x + 1, y + 12);
        }
        else if (section == buildSection || section == detailSection)
        {
            symbol.startNewSubPath (x, y + 2); symbol.lineTo (x + 10, y + 2);
            symbol.lineTo (x + 5, y + 12); symbol.closeSubPath();
        }
        else
        {
            symbol.startNewSubPath (x, y + 6); symbol.lineTo (x + 2, y + 6);
            symbol.lineTo (x + 4, y + 1); symbol.lineTo (x + 7, y + 11);
            symbol.lineTo (x + 9, y + 6); symbol.lineTo (x + 12, y + 6);
        }
        graphics.setColour (colours::accentBright.withAlpha (0.85f));
        graphics.strokePath (symbol, juce::PathStrokeType (1.2f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        graphics.setColour (colours::binding);
        graphics.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold))
                              .withExtraKerningFactor (0.075f));
        auto titleBounds = bounds.withHeight (captionHeight).withTrimmedLeft (34);
        if (section == effectsSection)
            titleBounds.setRight (fxEnableButton.getX() - 10);
        graphics.drawText (titles[static_cast<std::size_t> (section)],
                           titleBounds, juce::Justification::centredLeft);
        if (section == fretboardSection)
        {
            graphics.setColour (colours::dimText);
            graphics.setFont (juce::FontOptions (9.5f));
            graphics.drawText ("SELECT A STRING / CLICK TO REPICK",
                bounds.withHeight (sectionTitleHeight).reduced (14, 0),
                juce::Justification::centredRight);
        }
    }
    drawKeyboardLegend (graphics, keyboard.getBounds()
        .withY (keyboard.getY() - keyboardLegendHeight).withHeight (keyboardLegendHeight));
}

void ElectryAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (18);

    auto header = area.removeFromTop (64);
    auto brand = header.removeFromLeft (292);
    logoLabel.setBounds (brand.withTrimmedLeft (38).withHeight (43));
    editionLabel.setBounds (brand.withTrimmedLeft (40).withTrimmedTop (42));
    header.removeFromLeft (20);
    panicButton.setBounds (header.removeFromRight (92).reduced (0, 14));
    header.removeFromRight (16);
    statusDisplay.setBounds (header.removeFromRight (statusDisplayWidth)
                                 .reduced (0, 14));
    header.removeFromRight (20);
    factoryProgramSelector.setBounds (header.withTrimmedTop (22)
                                          .withTrimmedBottom (8));
    area.removeFromTop (10);

    // The legend above the keyboard carries the ranges; no footer is needed.
    keyboard.setBounds (area.removeFromBottom (100));
    keyboard.setKeyWidth (static_cast<float> (keyboard.getWidth())
                          / static_cast<float> (keyboardWhiteKeyCount));
    area.removeFromBottom (keyboardLegendHeight);
    area.removeFromBottom (12);

    // The two keyswitch strips and their compact play-style operating mode.
    auto articulationArea = area.removeFromTop (72);
    sectionBounds[articulationSection] = articulationArea;
    auto stripRow = articulationArea.reduced (12, 8);
    const int pickWidth = 202;
    const int modeWidth = 138;
    pickStyleStrip.setBounds (stripRow.removeFromLeft (pickWidth));
    stripRow.removeFromLeft (12);
    playStyleKeyModeStrip.setBounds (stripRow.removeFromLeft (modeWidth));
    stripRow.removeFromLeft (12);
    playStyleStrip.setBounds (stripRow);
    area.removeFromTop (10);

    // The live fretboard sits directly under the play styles, beside the five
    // performance controls that change what it shows.
    {
        auto fretboardRow = area.removeFromTop (
            juce::jmin (fretboardPanelHeight, juce::jmax (0, area.getHeight() - 260)));
        area.removeFromTop (12);
        auto performanceArea = fretboardRow.removeFromRight (
            juce::jmin (520, fretboardRow.getWidth() / 2));
        fretboardRow.removeFromRight (12);
        sectionBounds[fretboardSection] = fretboardRow;
        sectionBounds[performanceSection] = performanceArea;

        fretboardDisplay.setBounds (
            fretboardRow.reduced (12, 10).withTrimmedTop (sectionContentTrim));
        // These frequently used controls share the wider performance panel
        // evenly. Keep their compact dial height while giving full captions,
        // especially PALM PRESSURE, a comfortable single-line width.
        const auto controlsArea = performanceArea.reduced (10)
            .withTrimmedTop (sectionContentTrim);
        const std::array<ElectryKnob*, 5> performanceControls {
            &sympatheticKnob, &palmMuteKnob, &strumSpreadKnob,
            &tremoloRateKnob, &resonanceKnob
        };
        constexpr int controlGap = 10;
        constexpr int controlCount = static_cast<int> (performanceControls.size());
        const int usableWidth = controlsArea.getWidth() - controlGap * (controlCount - 1);
        const auto metrics = metricsFor (KnobTier::detail);
        const int height = juce::jmin (controlsArea.getHeight(), metrics.height);
        for (int index = 0; index < controlCount; ++index)
        {
            auto& knob = *performanceControls[static_cast<std::size_t> (index)];
            const int left = usableWidth * index / controlCount + controlGap * index;
            const int right = usableWidth * (index + 1) / controlCount + controlGap * index;
            knob.slider.getProperties().set (visualWeightProperty, metrics.visualWeight);
            knob.setBounds (controlsArea.getX() + left,
                            controlsArea.getCentreY() - height / 2,
                            right - left, height);
            knob.repaint();
        }
    }

    // Give the primary tone controls a clear, aligned row, then reserve
    // enough vertical room for the amp selector above the smaller FX dials.
    const int mainHeight = juce::jmax (180, area.getHeight() - 228);
    auto mainRow = area.removeFromTop (mainHeight);
    area.removeFromTop (12);
    auto secondaryRow = area;

    // Output mode stays comfortably operable beside the primary tone panel.
    auto masterArea = mainRow.removeFromRight (208);
    mainRow.removeFromRight (12);
    auto coreArea = mainRow;
    sectionBounds[coreSection] = coreArea;
    sectionBounds[masterSection] = masterArea;

    {
        auto inner = coreArea.reduced (16, 10)
                             .withTrimmedTop (sectionContentTrim);
        auto selectorArea = inner.removeFromLeft (juce::jmin (80, inner.getWidth()));
        pickupStrip.setBounds (selectorArea);
        inner.removeFromLeft (juce::jmin (6, inner.getWidth()));
        layoutKnobRow (
            inner,
            { { &pickupTypeKnob, KnobTier::hero },
              { &toneKnob, KnobTier::hero },
              { &pickPositionKnob, KnobTier::hero },
              { &pickHardnessKnob, KnobTier::hero },
              { &stringAgeKnob, KnobTier::hero },
              { &bodyResonanceKnob, KnobTier::hero },
              { &velocityKnob, KnobTier::hero } },
            10);
    }

    {
        auto masterInner = masterArea.reduced (12, 10)
                                     .withTrimmedTop (sectionContentTrim);
        outputModeStrip.setBounds (masterInner.removeFromTop (34));
        masterInner.removeFromTop (6);
        layoutKnobRow (
            masterInner,
            { { &outputKnob, KnobTier::master } }, 0);
    }

    const int secondaryContentWidth = juce::jmax (0, secondaryRow.getWidth() - 24);
    const int buildWidth = juce::roundToInt (
        static_cast<float> (secondaryContentWidth) * 0.14f);
    auto buildArea = secondaryRow.removeFromLeft (buildWidth);
    secondaryRow.removeFromLeft (juce::jmin (12, secondaryRow.getWidth()));
    const int detailWidth = juce::roundToInt (
        static_cast<float> (secondaryContentWidth) * 0.39f);
    auto detailArea = secondaryRow.removeFromLeft (detailWidth);
    secondaryRow.removeFromLeft (juce::jmin (12, secondaryRow.getWidth()));
    auto effectsArea = secondaryRow;
    sectionBounds[buildSection] = buildArea;
    sectionBounds[detailSection] = detailArea;
    sectionBounds[effectsSection] = effectsArea;

    layoutKnobRow (
        buildArea.reduced (12, 10).withTrimmedTop (sectionContentTrim),
        { { &guitarBuildKnob, KnobTier::hero } }, 0);

    layoutKnobRow (
        detailArea.reduced (12, 10).withTrimmedTop (sectionContentTrim),
        { { &muteDampingKnob, KnobTier::contextual },
          { &bendTimeKnob, KnobTier::detail },
          { &pickNoiseKnob, KnobTier::detail },
          { &fingerNoiseKnob, KnobTier::detail },
          { &releaseNoiseKnob, KnobTier::detail },
          { &artifactsKnob, KnobTier::detail } },
        4);

    auto effectsInner = effectsArea.reduced (12, 8);
    effectsInner.setTop (effectsArea.getY() + effectsHeaderHeight + 2);
    // The global switch remains available in bypass. All header controls have
    // a 28px hit area, a clear gap from their label, and room inside the panel.
    auto effectsHeader = effectsArea.withHeight (effectsHeaderHeight).reduced (12, 7);
    fxOversamplingStrip.setBounds (effectsHeader.removeFromRight (164));
    effectsHeader.removeFromRight (8);
    fxOversamplingLabel.setBounds (effectsHeader.removeFromRight (54));
    effectsHeader.removeFromRight (14);
    fxEnableButton.setBounds (effectsHeader.removeFromRight (76));
    ampModelStrip.setBounds (effectsInner.removeFromTop (46));
    effectsInner.removeFromTop (6);
    layoutKnobRow (
        effectsInner,
        { { &distortionKnob, KnobTier::detail },
          { &ampKnob, KnobTier::detail },
          { &compressorKnob, KnobTier::detail },
          { &delayKnob, KnobTier::detail },
          { &roomKnob, KnobTier::detail } }, 2);
}
