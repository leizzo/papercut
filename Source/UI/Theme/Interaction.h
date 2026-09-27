#pragma once

#include "ThemeManager.h"

namespace papercut
{

/** Where an interactive component is in the PRD §15.5 state model. */
struct ControlState
{
    bool enabled = true;
    bool hovered = false;
    bool pressed = false;
    bool on = false;          ///< toggled / active / selected
    bool focused = false;     ///< has keyboard focus

    /** Enabled, hover and keyboard focus read from the component; on and pressed from the caller. */
    static ControlState of (const juce::Component&, bool on = false, bool pressed = false);
};

/** The fill and the text colour for one state. */
struct StateColours
{
    juce::Colour fill, text;
};

/** The §15.5 rules. Default: base. Hover: bg-hover (a primary control: accent-hover).
    Pressed / on: the state colour, with text-on-accent. Disabled draws as default;
    the component's alpha dims it (see applyEnablement). */
StateColours stateColours (const Theme&, const ControlState&, juce::Colour base, juce::Colour onColour,
                           juce::Colour text, bool primary = false);

/** The 2 px focus ring outside bounds. Draw it whenever the state is focused: it is never removed. */
void paintFocusRing (juce::Graphics&, const Theme&, juce::Rectangle<float> bounds, float cornerRadius);

/** A circle with a slash: disabled controls and invalid drop targets. */
juce::MouseCursor notAllowedCursor();

/** Disabled components sit at the Theme's disabled opacity and show a not-allowed cursor. */
void applyEnablement (juce::Component&, const Theme&);

/** A drop shadow for each Shadow of an elevation level, under a rounded rectangle. */
void paintElevation (juce::Graphics&, const std::vector<Shadow>&, juce::Rectangle<float> bounds, float cornerRadius);

/** Draws a number (time, dB, %, pan, BPM, count) in the mono font at the style's size (§15.2). */
void drawNumber (juce::Graphics&, const ThemeManager&, const juce::String& text, const TypeStyle&,
                 juce::Rectangle<int> area, juce::Justification, juce::Colour);

/** Draws text in a type token's font and case. */
void drawStyledText (juce::Graphics&, const ThemeManager&, const juce::String& text, const TypeStyle&,
                     juce::Rectangle<int> area, juce::Justification, juce::Colour);

} // namespace papercut
