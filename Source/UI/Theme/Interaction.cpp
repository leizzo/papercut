#include "Interaction.h"

namespace papercut
{

ControlState ControlState::of (const juce::Component& c, bool on, bool pressed)
{
    ControlState s;
    s.enabled = c.isEnabled();
    s.hovered = s.enabled && c.isMouseOverOrDragging (true);
    s.pressed = s.enabled && pressed;
    s.on = on;
    s.focused = c.hasKeyboardFocus (false);
    return s;
}

StateColours stateColours (const Theme& theme, const ControlState& state, juce::Colour base, juce::Colour onColour,
                           juce::Colour text, bool primary)
{
    if (state.on || (state.pressed && ! primary))
        return { state.hovered ? onColour.brighter (0.08f) : onColour, theme.textOnAccent };

    if (primary)
        return { state.hovered || state.pressed ? theme.accentHover : theme.accent, theme.textOnAccent };

    if (state.hovered)
        return { theme.bgHover, text };

    return { base, text };
}

void paintFocusRing (juce::Graphics& g, const Theme& theme, juce::Rectangle<float> bounds, float cornerRadius)
{
    g.setColour (theme.focusRing);
    g.drawRoundedRectangle (bounds.expanded (1.0f), cornerRadius + 1.0f, 2.0f);
}

juce::MouseCursor notAllowedCursor()
{
    static const juce::MouseCursor cursor = []
    {
        constexpr int size = 20;
        juce::Image image (juce::Image::ARGB, size, size, true);
        juce::Graphics g (image);
        const auto ring = juce::Rectangle<float> (0.0f, 0.0f, (float) size, (float) size).reduced (3.0f);

        for (auto [colour, width] : { std::pair (juce::Colours::white, 4.5f), std::pair (juce::Colour (0xffe0443a), 2.0f) })
        {
            g.setColour (colour);
            g.drawEllipse (ring, width);
            g.drawLine ({ ring.getTopLeft().translated (2.5f, 2.5f), ring.getBottomRight().translated (-2.5f, -2.5f) }, width);
        }

        return juce::MouseCursor (image, size / 2, size / 2);
    }();

    return cursor;
}

void applyEnablement (juce::Component& c, const Theme& theme)
{
    const auto enabled = c.isEnabled();
    c.setAlpha (enabled ? 1.0f : theme.disabledOpacity);
    c.setMouseCursor (enabled ? juce::MouseCursor::NormalCursor : notAllowedCursor());
}

void paintElevation (juce::Graphics& g, const std::vector<Shadow>& level, juce::Rectangle<float> bounds, float cornerRadius)
{
    juce::Path shape;
    shape.addRoundedRectangle (bounds, cornerRadius);

    for (auto& shadow : level)
        juce::DropShadow (shadow.colour, shadow.radius, shadow.offset).drawForPath (g, shape);
}

void drawNumber (juce::Graphics& g, const ThemeManager& tm, const juce::String& text, const TypeStyle& style,
                 juce::Rectangle<int> area, juce::Justification justification, juce::Colour colour)
{
    g.setColour (colour);
    g.setFont (tm.numberFont (style));
    g.drawText (text, area, justification, false);
}

void drawStyledText (juce::Graphics& g, const ThemeManager& tm, const juce::String& text, const TypeStyle& style,
                     juce::Rectangle<int> area, juce::Justification justification, juce::Colour colour)
{
    g.setColour (colour);
    g.setFont (tm.font (style));
    g.drawText (style.apply (text), area, justification, true);
}

} // namespace papercut
