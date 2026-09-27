#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace papercut
{

/** The design's line icons, drawn in a 24 x 24 box (lucide-style, 2 px strokes). */
enum class Icon
{
    play, stop, record, skipBack, spline, rotateCcw, metronome, follow,
    chevronDown, chevronRight, layers, arrowUpRight, arrowDown, power, search,
    folder, plus, x, audioLines, music, file, gripVertical
};

/** Draws the icon centred in area, stroked (or filled, for solid glyphs) in colour. */
void drawIcon (juce::Graphics&, Icon, juce::Rectangle<float> area, juce::Colour);

} // namespace papercut
