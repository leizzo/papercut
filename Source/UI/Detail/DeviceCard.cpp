#include "DeviceCard.h"
#include "Commands/CommandRegistry.h"
#include "Commands/PluginCommands.h"

namespace papercut
{

namespace
{
    constexpr int titleHeight = 30, knobWidth = 48, knobGap = 14, bodyPadding = 16, dialSize = 36;
}

DeviceCard::DeviceCard (CommandRegistry& c, PluginRack& r, ThemeManager& tm, const juce::String& track, const PluginInfo& info)
    : commands (c), rack (r), themeManager (tm), trackId (track), plugin (info)
{
    setTitle (info.name);
    rebuildKnobs (rack.getParameters (plugin.id));
}

void DeviceCard::rebuildKnobs (const std::vector<PluginParameter>& parameters)
{
    std::vector<juce::String> ids;

    for (size_t i = 0; i < parameters.size() && (int) i < maxKnobs; ++i)
        ids.push_back (parameters[i].id);

    if (ids != parameterIds)
    {
        knobs.clear();
        parameterIds = ids;

        for (size_t i = 0; i < parameterIds.size(); ++i)
        {
            auto& p = parameters[i];
            ContinuousValue::Spec spec;
            spec.minimum = p.minimum;
            spec.maximum = p.maximum;
            spec.defaultValue = p.defaultValue;
            spec.format.format = [this, id = p.id] (double v) { return rack.getParameterText (plugin.id, id, (float) v); };
            spec.format.parse = ValueFormat::number (3).parse;

            auto knob = std::make_unique<Knob> (themeManager, spec, p.name);
            knob->setDialSize (dialSize);
            knob->setTooltip (p.name);
            knob->onChange = [this, id = p.id] (double v, bool continues)
            {
                commands.invoke ("plugin.setParameter", pluginParameterArgs (plugin.id, id, (float) v, continues));
            };
            addAndMakeVisible (*knob);
            knobs.push_back (std::move (knob));
        }

        resized();
    }

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        knobs[i]->setValue (parameters[i].value);
        knobs[i]->setDimmed (! plugin.enabled);
        knobs[i]->setArcColour (colour);
        knobs[i]->setVisible (! collapsed);
    }
}

void DeviceCard::setState (const PluginInfo& info, bool isCollapsed)
{
    auto& theme = themeManager.getTheme();
    plugin = info;
    // A stable pick from the track palette by device type (the design: EQ Eight is always arp, Saturator bass).
    colour = theme.trackColour ((int) ((juce::uint32) (plugin.manufacturer + "/" + plugin.name).hashCode()
                                       % (juce::uint32) theme.trackPalette.size()));
    collapsed = isCollapsed;
    setAlpha (plugin.enabled ? 1.0f : 0.5f);
    rebuildKnobs (rack.getParameters (plugin.id));
    repaint();
}

int DeviceCard::getPreferredWidth() const
{
    if (collapsed)
        return collapsedWidth;

    const auto n = juce::jmax (3, (int) knobs.size());
    return 2 * bodyPadding + n * knobWidth + (n - 1) * knobGap;
}

juce::Rectangle<int> DeviceCard::titleBar() const
{
    return collapsed ? getLocalBounds() : getLocalBounds().removeFromTop (titleHeight);
}

juce::Rectangle<int> DeviceCard::powerButton() const
{
    auto bar = titleBar();
    return (collapsed ? bar.removeFromTop (titleHeight) : bar.removeFromLeft (11 + 14)).withTrimmedLeft (collapsed ? 0 : 11)
               .withSizeKeepingCentre (14, 14);
}

void DeviceCard::paint (juce::Graphics& g)
{
    auto& theme = themeManager.getTheme();
    const auto bounds = getLocalBounds().toFloat();
    const auto radius = theme.radiusXl;

    g.setColour (theme.bgTrack);
    g.fillRoundedRectangle (bounds, radius);

    // Title bar in the device colour.
    {
        juce::Graphics::ScopedSaveState save (g);
        juce::Path clip;
        clip.addRoundedRectangle (bounds, radius);
        g.reduceClipRegion (clip);
        g.setColour (colour);
        g.fillRect (titleBar());
    }

    // Power: a dark circle with a dot in the device colour while on.
    const auto power = powerButton().toFloat();
    g.setColour (theme.textOnAccent);
    g.fillEllipse (power);

    if (plugin.enabled)
    {
        g.setColour (colour);
        g.fillEllipse (power.withSizeKeepingCentre (5.0f, 5.0f));
    }

    g.setColour (theme.textOnAccent);
    g.setFont (themeManager.font (TypeStyle { theme.label.size, false, 700 }));

    if (collapsed)
    {
        // The name runs down the collapsed card.
        juce::Graphics::ScopedSaveState save (g);
        const auto area = getLocalBounds().withTrimmedTop (titleHeight);
        g.addTransform (juce::AffineTransform::rotation (juce::MathConstants<float>::halfPi, (float) area.getCentreX(), (float) area.getCentreY()));
        g.drawText (plugin.name, area.withSizeKeepingCentre (area.getHeight(), area.getWidth()), juce::Justification::centredLeft, true);
    }
    else
    {
        g.drawText (plugin.name, titleBar().withTrimmedLeft (11 + 14 + 8).withTrimmedRight (8), juce::Justification::centredLeft, true);
    }

    if (plugin.missing)
    {
        juce::Path outline, dashed;
        outline.addRoundedRectangle (bounds.reduced (1.0f), radius);
        const float dashes[] = { 4.0f, 3.0f };
        juce::PathStrokeType (1.5f).createDashedStroke (dashed, outline, dashes, 2);
        g.setColour (theme.rec);
        g.fillPath (dashed);

        if (! collapsed)
        {
            auto badge = getLocalBounds().withTrimmedTop (titleHeight).reduced (bodyPadding).removeFromTop (14).removeFromLeft (52);
            g.setColour (theme.rec.withAlpha (0.2f));
            g.fillRoundedRectangle (badge.toFloat(), theme.radiusSm);
            drawStyledText (g, themeManager, "Missing", theme.micro, badge, juce::Justification::centred, theme.rec);
        }
    }
    else
    {
        g.setColour (theme.border);
        g.drawRoundedRectangle (bounds.reduced (0.5f), radius, 1.0f);
    }
}

void DeviceCard::resized()
{
    auto body = getLocalBounds().withTrimmedTop (titleHeight).reduced (bodyPadding, 14);

    for (auto& knob : knobs)
    {
        knob->setBounds (body.removeFromLeft (knobWidth).withHeight (dialSize + 4 + 2 * 13));
        body.removeFromLeft (knobGap);
    }
}

void DeviceCard::mouseDown (const juce::MouseEvent& e)
{
    if (! titleBar().contains (e.getPosition()))
        return;

    if (e.mods.isPopupMenu())
    {
        showMenu();
        return;
    }

    if (powerButton().expanded (3).contains (e.getPosition()))
        commands.invoke ("plugin.setBypassed", pluginBypassArgs (trackId, plugin.id, plugin.enabled));
}

void DeviceCard::mouseDrag (const juce::MouseEvent& e)
{
    // Dragging the title bar reorders the chain (the DeviceChain row is the drop target).
    if (titleBar().contains (e.getMouseDownPosition()) && e.getDistanceFromDragStart() > 4)
        if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor (this); container != nullptr
                                                                                            && ! container->isDragAndDropActive())
        {
            auto d = new juce::DynamicObject();
            d->setProperty ("deviceCard", plugin.id);
            container->startDragging (juce::var (d), this, juce::ScaledImage (createComponentSnapshot (getLocalBounds())));
        }
}

void DeviceCard::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (titleBar().contains (e.getPosition()) && ! powerButton().expanded (3).contains (e.getPosition()) && onToggleCollapsed)
        onToggleCollapsed();
}

void DeviceCard::showMenu()
{
    juce::PopupMenu menu;
    menu.addItem ("Open Plug-in Window", [this] { if (onOpenEditor) onOpenEditor(); });
    menu.addItem (plugin.enabled ? "Bypass" : "Enable",
                  [this] { commands.invoke ("plugin.setBypassed", pluginBypassArgs (trackId, plugin.id, plugin.enabled)); });
    menu.addItem (collapsed ? "Expand" : "Collapse", [this] { if (onToggleCollapsed) onToggleCollapsed(); });
    menu.addSeparator();
    menu.addItem ("Delete", [this] { commands.invoke ("plugin.remove", pluginArgs (trackId, plugin.id)); });
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this));
}

} // namespace papercut
