#include "InsertSlot.h"

namespace papercut
{

InsertSlot::InsertSlot (ThemeManager& tm, int i) : themeManager (tm), index (i)
{
    setRepaintsOnMouseActivity (true);
    setTitle ("Insert " + juce::String (i + 1));
}

void InsertSlot::setPlugin (std::optional<PluginInfo> p)
{
    plugin = std::move (p);
    setTooltip (plugin ? plugin->name + (plugin->missing ? " (missing)" : juce::String())
                       : juce::String ("Empty insert: click to add an effect, or drop one here"));
    repaint();
}

void InsertSlot::setDropHighlight (std::optional<bool> valid)
{
    dropHighlight = valid;
    setMouseCursor (valid.has_value() && ! *valid ? notAllowedCursor() : juce::MouseCursor::NormalCursor);
    repaint();
}

juce::Rectangle<int> InsertSlot::powerBounds() const
{
    return getLocalBounds().removeFromLeft (7 + 6 + 3).withTrimmedLeft (4);
}

void InsertSlot::paint (juce::Graphics& g)
{
    auto& theme = themeManager.getTheme();
    const auto bounds = getLocalBounds().toFloat();
    const auto radius = theme.radiusMd;
    const auto hovered = isMouseOver (true);

    g.setColour (plugin ? (hovered ? theme.bgHover : theme.bgElevated) : (hovered ? theme.bgTrack : theme.bgSlot));
    g.fillRoundedRectangle (bounds, radius);

    if (plugin)
    {
        const auto on = plugin->enabled;
        auto r = getLocalBounds().reduced (7, 0);
        g.setColour (plugin->missing ? theme.rec : on ? theme.accent : theme.textDim);
        g.fillEllipse (r.removeFromLeft (6).withSizeKeepingCentre (6, 6).toFloat());
        r.removeFromLeft (6);

        if (plugin->missing)
        {
            auto badge = r.removeFromRight (40).withSizeKeepingCentre (40, 12);
            g.setColour (theme.rec.withAlpha (0.2f));
            g.fillRoundedRectangle (badge.toFloat(), theme.radiusSm);
            drawStyledText (g, themeManager, "Missing", theme.micro, badge, juce::Justification::centred, theme.rec);
        }

        drawStyledText (g, themeManager, plugin->name, TypeStyle { 10.0f, false, 400 }, r, juce::Justification::centredLeft,
                        on ? theme.textPrimary : theme.textDim);

        if (plugin->missing)
        {
            juce::Path outline, dashed;
            outline.addRoundedRectangle (bounds.reduced (0.75f), radius);
            const float dashes[] = { 3.0f, 2.0f };
            juce::PathStrokeType (1.0f).createDashedStroke (dashed, outline, dashes, 2);
            g.setColour (theme.rec);
            g.fillPath (dashed);
        }
    }

    if (dropHighlight.has_value() && *dropHighlight)
    {
        g.setColour (theme.accentDim);
        g.drawRoundedRectangle (bounds.reduced (1.0f), radius, 2.0f);
    }
}

void InsertSlot::mouseDown (const juce::MouseEvent& e)
{
    dragStarted = false;

    if (e.mods.isPopupMenu())
    {
        if (onMenu)
            onMenu (e);

        return;
    }

    if (plugin && powerBounds().contains (e.getPosition()))
    {
        if (onPowerClick)
            onPowerClick (e);

        return;
    }
}

void InsertSlot::mouseUp (const juce::MouseEvent& e)
{
    if (dragStarted || e.mods.isPopupMenu() || e.mouseWasDraggedSinceMouseDown()
        || (plugin && powerBounds().contains (e.getMouseDownPosition())))
        return;

    if (onClick)
        onClick (e);
}

void InsertSlot::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragStarted && plugin && e.getDistanceFromDragStart() > 4 && ! powerBounds().contains (e.getMouseDownPosition()))
    {
        dragStarted = true;

        if (onDrag)
            onDrag (e);
    }
}

} // namespace papercut
