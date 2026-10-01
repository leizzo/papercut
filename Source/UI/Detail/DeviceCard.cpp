#include "DeviceCard.h"
#include "NativeDeviceCard.h"
#include "PluginDeviceCard.h"
#include "Commands/CommandRegistry.h"
#include "Commands/PluginCommands.h"

namespace resamper
{

std::unique_ptr<DeviceCard> DeviceCard::create (CommandRegistry& c, PluginRack& r, ThemeManager& tm, const juce::String& track,
                                                const PluginInfo& info)
{
    if (info.external)
        return std::make_unique<PluginDeviceCard> (c, r, tm, track, info);

    return std::make_unique<NativeDeviceCard> (c, r, tm, track, info);
}

DeviceCard::DeviceCard (CommandRegistry& c, PluginRack& r, ThemeManager& tm, const juce::String& track, const PluginInfo& info)
    : commands (c), rack (r), themeManager (tm), trackId (track), plugin (info)
{
    setTitle (info.name);
}

void DeviceCard::toggleBypass()
{
    commands.invoke (cmd::pluginSetBypassed, { trackId, plugin.id, plugin.enabled });
}

void DeviceCard::showMenu()
{
    juce::PopupMenu menu;
    addMenuItems (menu);
    menu.addItem (plugin.enabled ? "Bypass" : "Enable", [this] { toggleBypass(); });
    menu.addSeparator();
    menu.addItem ("Delete", [this] { commands.invoke (cmd::pluginRemove, { trackId, plugin.id }); });
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this));
}

void DeviceCard::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu() && getTitleBar().contains (e.getPosition()))
        showMenu();
}

void DeviceCard::mouseDrag (const juce::MouseEvent& e)
{
    // Dragging the title bar reorders the chain (the DeviceChain row is the drop target).
    if (getTitleBar().contains (e.getMouseDownPosition()) && e.getDistanceFromDragStart() > 4)
        if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor (this); container != nullptr
                                                                                            && ! container->isDragAndDropActive())
        {
            auto d = new juce::DynamicObject();
            d->setProperty ("deviceCard", plugin.id);
            container->startDragging (juce::var (d), this, juce::ScaledImage (createComponentSnapshot (getLocalBounds())));
        }
}

//==============================================================================
DevicePowerButton::DevicePowerButton (ThemeManager& tm, Style s) : ThemedButton (tm, "Power"), style (s)
{
    setComponentID ("power");
    setTooltip ("Power (bypass)");
}

void DevicePowerButton::setDotColour (juce::Colour c)
{
    dot = c;
    repaint();
}

void DevicePowerButton::paintButton (juce::Graphics& g, bool highlighted, bool)
{
    auto& theme = themeManager.getTheme();
    const auto disc = getLocalBounds().toFloat().withSizeKeepingCentre (14.0f, 14.0f);
    const auto on = getToggleState();

    if (style == Style::native)
    {
        g.setColour (theme.textOnAccent.withAlpha (highlighted ? 0.8f : 1.0f));
        g.fillEllipse (disc);

        if (on)
        {
            g.setColour (dot);
            g.fillEllipse (disc.withSizeKeepingCentre (5.0f, 5.0f));
        }
    }
    else
    {
        g.setColour (on ? theme.accent : theme.textDim);
        g.drawEllipse (disc.reduced (0.75f), highlighted ? 2.0f : 1.5f);

        if (on)
            g.fillEllipse (disc.withSizeKeepingCentre (5.0f, 5.0f));
    }

    paintFocus (g, disc.getWidth() / 2.0f);
}

//==============================================================================
DeviceHeaderButton::DeviceHeaderButton (ThemeManager& tm, const juce::String& name, std::optional<Icon> i, const juce::String& text)
    : ThemedButton (tm, name), icon (i)
{
    setButtonText (text);
    setTooltip (name);
}

void DeviceHeaderButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto& theme = themeManager.getTheme();
    const auto bounds = getLocalBounds().toFloat();

    if (getToggleState() || ((highlighted || down) && isEnabled()))
    {
        g.setColour (theme.textOnAccent.withAlpha (down || getToggleState() ? 0.25f : 0.12f));
        g.fillRoundedRectangle (bounds, theme.radiusSm);
    }

    auto area = getLocalBounds();

    if (icon)
        drawIcon (g, *icon, (getButtonText().isEmpty() ? area : area.removeFromLeft (getHeight())).toFloat().reduced (3.0f),
                  theme.textOnAccent);

    if (getButtonText().isNotEmpty())
        drawStyledText (g, themeManager, getButtonText(), TypeStyle { theme.micro.size, false, 600 }, area,
                        juce::Justification::centred, theme.textOnAccent);

    paintFocus (g, theme.radiusSm);
}

} // namespace resamper
