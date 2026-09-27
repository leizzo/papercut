#include "Icons.h"

namespace papercut
{

namespace
{
    struct Glyph
    {
        const char* svgPath;
        bool filled;
    };

    Glyph glyphFor (Icon icon)
    {
        switch (icon)
        {
            case Icon::play:          return { "M7 4 L19 12 L7 20 Z", true };
            case Icon::stop:          return { "M6 6 H18 V18 H6 Z", true };
            case Icon::record:        return { "M12 5 A7 7 0 1 1 11.99 5 Z", true };
            case Icon::skipBack:      return { "M19 20 L9 12 L19 4 Z M5 19 V5", false };
            case Icon::spline:        return { "M3 19 C8 19 8 5 12 5 C16 5 16 19 21 19", false };
            case Icon::rotateCcw:     return { "M3 12 A9 9 0 1 0 6 5.3 M3 4 V9 H8", false };
            case Icon::metronome:     return { "M8 21 L11 3 H13 L16 21 Z M12 14 L18 6 M6 17 H18", false };
            case Icon::follow:        return { "M3 12 H15 M11 8 L15 12 L11 16 M20 5 V19", false };
            case Icon::chevronDown:   return { "M6 9 L12 15 L18 9", false };
            case Icon::chevronRight:  return { "M9 6 L15 12 L9 18", false };
            case Icon::layers:        return { "M12 3 L21 8 L12 13 L3 8 Z M3 12 L12 17 L21 12 M3 16 L12 21 L21 16", false };
            case Icon::arrowUpRight:  return { "M7 17 L17 7 M8 7 H17 V16", false };
            case Icon::arrowDown:     return { "M12 5 V19 M6 13 L12 19 L18 13", false };
            case Icon::power:         return { "M12 3 V12 M6.3 6.3 A8 8 0 1 0 17.7 6.3", false };
            case Icon::search:        return { "M11 4 A7 7 0 1 1 10.99 4 Z M16 16 L21 21", false };
            case Icon::folder:        return { "M3 6 H9 L11 8 H21 V19 H3 Z", false };
            case Icon::plus:          return { "M12 5 V19 M5 12 H19", false };
            case Icon::x:             return { "M6 6 L18 18 M18 6 L6 18", false };
            case Icon::audioLines:    return { "M3 10 V14 M7 6 V18 M11 3 V21 M15 8 V16 M19 5 V19", false };
            case Icon::music:         return { "M9 18 V5 L20 3 V16 M9 18 A3 3 0 1 1 8.99 18 M20 16 A3 3 0 1 1 19.99 16", false };
            case Icon::file:          return { "M6 3 H14 L19 8 V21 H6 Z M14 3 V8 H19", false };
            case Icon::gripVertical:  return { "M9 6 V6.1 M15 6 V6.1 M9 12 V12.1 M15 12 V12.1 M9 18 V18.1 M15 18 V18.1", false };
        }

        return { "", false };
    }
}

void drawIcon (juce::Graphics& g, Icon icon, juce::Rectangle<float> area, juce::Colour colour)
{
    const auto glyph = glyphFor (icon);
    auto path = juce::Drawable::parseSVGPath (glyph.svgPath);
    const auto box = area.withSizeKeepingCentre (juce::jmin (area.getWidth(), area.getHeight()),
                                                 juce::jmin (area.getWidth(), area.getHeight()));
    const auto scale = box.getWidth() / 24.0f;
    path.applyTransform (juce::AffineTransform::scale (scale).translated (box.getX(), box.getY()));

    g.setColour (colour);

    if (glyph.filled)
        g.fillPath (path);
    else
        g.strokePath (path, juce::PathStrokeType (juce::jmax (1.0f, 2.0f * scale), juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
}

} // namespace papercut
