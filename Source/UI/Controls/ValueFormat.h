#pragma once

#include <juce_core/juce_core.h>

#include <functional>

namespace resamper
{

/** How a control's value reads as text and how typed text reads back
    (PRD §16.2: "-6db", "L30", "40%"). Parsing accepts the unit, spaces and
    either case. */
struct ValueFormat
{
    std::function<juce::String (double)> format;

    /** Sets value and returns true when text is a value of this kind. */
    std::function<bool (const juce::String& text, double& value)> parse;

    /** Decibels: "-6.0 dB", "+2.4 dB"; at or below floorDb, "-inf dB". */
    static ValueFormat decibels (double floorDb = -100.0);

    /** Pan -1..1: "L30", "C", "R20". Also parses a signed percent ("-30"). */
    static ValueFormat pan();

    /** 0..1 as a percent: "40%". */
    static ValueFormat percent();

    /** Tempo: "124.00". */
    static ValueFormat bpm();

    /** A plain number with the given decimals and an optional unit suffix. */
    static ValueFormat number (int decimals, juce::String unit = {});
};

} // namespace resamper
