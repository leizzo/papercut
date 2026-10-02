#include "Icons.h"

namespace resamper
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
            case Icon::spline:        return { "M21 5a2 2 0 1 1-4 0a2 2 0 1 1 4 0Z M7 19a2 2 0 1 1-4 0a2 2 0 1 1 4 0Z "
                                               "M5 17A12 12 0 0 1 17 5", false };
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
            case Icon::gitMerge:      return { "M18 15 A3 3 0 1 1 17.99 15 Z M6 3 A3 3 0 1 1 5.99 3 Z M6 21 V9 A9 9 0 0 0 15 18", false };
            case Icon::plug:          return { "M12 22v-5 M9 8V2 M15 8V2 M18 8v5a4 4 0 0 1-4 4h-4a4 4 0 0 1-4-4V8Z", false };
            case Icon::appWindow:     return { "M4 4h16a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V6a2 2 0 0 1 2-2Z "
                                               "M10 4v4 M2 8h20 M6 4v4", false };
            case Icon::externalLink:  return { "M15 3h6v6 M10 14L21 3 M18 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6", false };
            case Icon::pin:           return { "M12 17v5 M9 10.76a2 2 0 0 1-1.11 1.79l-1.78 0.9A2 2 0 0 0 5 15.24V16a1 1 0 0 0 1 1h12 "
                                               "a1 1 0 0 0 1-1v-0.76a2 2 0 0 0-1.11-1.79l-1.78-0.9A2 2 0 0 1 15 10.76V7a1 1 0 0 1 1-1 "
                                               "a2 2 0 0 0 0-4H8a2 2 0 0 0 0 4a1 1 0 0 1 1 1Z", false };
            case Icon::cpu:           return { "M6 4h12a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2H6a2 2 0 0 1-2-2V6a2 2 0 0 1 2-2Z "
                                               "M10 9h4a1 1 0 0 1 1 1v4a1 1 0 0 1-1 1h-4a1 1 0 0 1-1-1v-4a1 1 0 0 1 1-1Z "
                                               "M15 2v2 M15 20v2 M2 15h2 M2 9h2 M20 15h2 M20 9h2 M9 2v2 M9 20v2", false };
            case Icon::timer:         return { "M10 2h4 M12 14l3-3 M20 14a8 8 0 1 1-16 0a8 8 0 1 1 16 0Z", false };
            case Icon::shieldCheck:   return { "M20 13c0 5-3.5 7.5-7.66 8.95a1 1 0 0 1-0.67-0.01C7.5 20.5 4 18 4 13V6a1 1 0 0 1 1-1 "
                                               "c2 0 4.5-1.2 6.24-2.72a1.17 1.17 0 0 1 1.52 0C14.51 3.81 17 5 19 5a1 1 0 0 1 1 1Z "
                                               "M9 12l2 2 4-4", false };
            case Icon::ellipsis:      return { "M13 12a1 1 0 1 1-2 0a1 1 0 1 1 2 0Z M20 12a1 1 0 1 1-2 0a1 1 0 1 1 2 0Z "
                                               "M6 12a1 1 0 1 1-2 0a1 1 0 1 1 2 0Z", false };
            case Icon::maximize2:     return { "M15 3h6v6 M9 21H3v-6 M21 3l-7 7 M3 21l7-7", false };
            case Icon::minimize2:     return { "M4 14h6v6 M20 10h-6V4 M14 10l7-7 M3 21l7-7", false };
            case Icon::foldVertical:  return { "M12 22v-6 M12 8V2 M4 12H2 M10 12H8 M16 12h-2 M22 12h-2 M15 19l-3-3-3 3 M15 5l-3 3-3-3", false };
            case Icon::unfoldVertical: return { "M12 22v-6 M12 8V2 M4 12H2 M10 12H8 M16 12h-2 M22 12h-2 M15 19l-3 3-3-3 M15 5l-3-3-3 3", false };
            case Icon::squareDashed:  return { "M5 3a2 2 0 0 0-2 2 M19 3a2 2 0 0 1 2 2 M21 19a2 2 0 0 1-2 2 M5 21a2 2 0 0 1-2-2 "
                                               "M9 3h1 M9 21h1 M14 3h1 M14 21h1 M3 9v1 M21 9v1 M3 14v1 M21 14v1", false };
            case Icon::chevronLeft:   return { "M15 18 L9 12 L15 6", false };
            case Icon::undo2:         return { "M9 14 L4 9 L9 4 M4 9h10.5a5.5 5.5 0 0 1 5.5 5.5a5.5 5.5 0 0 1-5.5 5.5H11", false };
            case Icon::redo2:         return { "M15 14 L20 9 L15 4 M20 9H9.5A5.5 5.5 0 0 0 4 14.5A5.5 5.5 0 0 0 9.5 20H13", false };
            case Icon::save:          return { "M15.2 3a2 2 0 0 1 1.4 0.6l3.8 3.8a2 2 0 0 1 0.6 1.4V19a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2Z "
                                               "M17 21v-7a1 1 0 0 0-1-1H8a1 1 0 0 0-1 1v7 M7 3v4a1 1 0 0 0 1 1h7", false };
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

} // namespace resamper
