#include "PluginDeviceCard.h"
#include "Commands/CommandRegistry.h"
#include "Commands/PluginCommands.h"
#include "UI/Controls/ContinuousControl.h"

namespace resamper
{

namespace
{
    constexpr int padding = 10, missingBadgeWidth = 48, buttonHeight = 22, rowHeight = 16, rowGap = 3, cpuPollMs = 500;

    const juce::String middleDot (juce::CharPointer_UTF8 ("\xc2\xb7"));
}

//==============================================================================
/** One pinned parameter: its name, a mini bar and its mono value. Drags
    horizontally; clicking the value types one. */
class PluginDeviceCard::PinnedParameter : public ContinuousControl
{
public:
    PinnedParameter (ThemeManager& tm, ContinuousValue::Spec spec, juce::String parameterName)
        : ContinuousControl (tm, std::move (spec), Axis::horizontal), name (std::move (parameterName))
    {
        setComponentID ("pinned");
        setTitle (name);
    }

    juce::String parameterId;

    void paint (juce::Graphics& g) override
    {
        auto& theme = themeManager.getTheme();
        auto r = getLocalBounds();
        drawStyledText (g, themeManager, name, TypeStyle { theme.micro.size, false, 500 }, r.removeFromLeft (nameWidth),
                        juce::Justification::centredLeft, theme.textSecondary);

        if (! isEditingText())
            drawNumber (g, themeManager, getModel().getText(), TypeStyle { theme.micro.size, true, 400 }, getReadoutBounds(),
                        juce::Justification::centredRight, theme.textPrimary);

        const auto bar = r.withTrimmedRight (valueWidth + 6).toFloat().withSizeKeepingCentre ((float) r.getWidth() - valueWidth - 6, 4.0f);
        g.setColour (theme.bgSlot);
        g.fillRoundedRectangle (bar, theme.radiusXs);
        g.setColour (isMouseOverOrDragging() ? theme.accentHover : theme.accent);
        g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * (float) getModel().getProportion()), theme.radiusXs);
    }

protected:
    juce::Rectangle<int> getReadoutBounds() const override   { return getLocalBounds().removeFromRight (valueWidth); }

private:
    static constexpr int nameWidth = 62, valueWidth = 48;
    juce::String name;
};

//==============================================================================
PluginDeviceCard::PluginDeviceCard (CommandRegistry& c, PluginRack& r, ThemeManager& tm, const juce::String& track,
                                    const PluginInfo& info)
    : DeviceCard (c, r, tm, track, info),
      power (tm, DevicePowerButton::Style::plugin),
      openWindow (tm, "Open plug-in window", Button::Variant::outline, Icon::appWindow),
      locate (tm, "Locate", Button::Variant::outline),
      replace (tm, "Replace", Button::Variant::outline)
{
    setComponentID ("DeviceCard/Plugin");
    openWindow.setComponentID ("openWindow");
    locate.setComponentID ("locate");
    replace.setComponentID ("replace");
    locate.setTooltip ("Scan the plug-in folders again for it");
    replace.setTooltip ("Put another plug-in in its place");

    power.onClick = [this] { toggleBypass(); };
    openWindow.onClick = [this] { if (onOpenEditor) onOpenEditor(); };
    locate.onClick = [this] { commands.invoke (cmd::pluginScan); };
    replace.onClick = [this] { showReplaceMenu(); };

    addAndMakeVisible (power);
    addChildComponent (openWindow);
    addChildComponent (locate);
    addChildComponent (replace);

    timerCallback();
    startTimer (cpuPollMs);
}

PluginDeviceCard::~PluginDeviceCard() = default;

juce::String PluginDeviceCard::formatBadge() const
{
    return plugin.format == "AudioUnit" ? juce::String ("AU") : plugin.format;
}

void PluginDeviceCard::setState (const PluginInfo& info, DeviceSize)
{
    plugin = info;
    setTitle (plugin.name);
    // Never colour-only (§18): the vendor and the format are always said.
    const auto vendor = plugin.manufacturer.isNotEmpty() ? plugin.manufacturer : juce::String ("Unknown vendor");
    setDescription (vendor + " " + middleDot + " " + formatBadge() + " plug-in" + (plugin.missing ? ", missing" : ""));
    setAlpha (plugin.enabled ? 1.0f : 0.5f);
    power.setToggleState (plugin.enabled, juce::dontSendNotification);

    openWindow.setVisible (! plugin.missing);
    locate.setVisible (plugin.missing);
    replace.setVisible (plugin.missing);

    rebuildPins();
    resized();
    repaint();
}

void PluginDeviceCard::rebuildPins()
{
    const auto parameters = rack.getParameters (plugin.id);
    std::vector<const PluginParameter*> pinned;

    for (auto& id : plugin.pinnedParameters)
        for (auto& p : parameters)
            if (p.id == id)
                pinned.push_back (&p);

    bool same = pinned.size() == pins.size();

    for (size_t i = 0; same && i < pinned.size(); ++i)
        same = pins[i]->parameterId == pinned[i]->id;

    if (! same)
    {
        pins.clear();

        for (auto* p : pinned)
        {
            auto row = std::make_unique<PinnedParameter> (themeManager, specFor (*p), p->name);
            row->parameterId = p->id;
            row->onChange = setterFor (p->id);
            addAndMakeVisible (*row);
            pins.push_back (std::move (row));
        }
    }

    for (size_t i = 0; i < pins.size(); ++i)
        pins[i]->setValue (pinned[i]->value);
}

void PluginDeviceCard::setWindowOpen (bool open)
{
    windowOpen = open;
    openWindow.setButtonText (open ? "Window open " + middleDot + " focus" : juce::String ("Open plug-in window"));
    repaint();
}

juce::Rectangle<int> PluginDeviceCard::getTitleBar() const
{
    return getLocalBounds().removeFromTop (titleHeight);
}

void PluginDeviceCard::resized()
{
    power.setBounds (getTitleBar().removeFromLeft (padding + 16).withTrimmedLeft (padding - 2));

    auto body = getLocalBounds().withTrimmedTop (titleHeight).withTrimmedBottom (footerHeight).reduced (padding, 8);
    auto buttons = body.removeFromTop (buttonHeight);

    if (plugin.missing)
    {
        // The Missing badge sits left of Locate and Replace.
        buttons.removeFromLeft (missingBadgeWidth + 4);
        replace.setBounds (buttons.removeFromRight (64));
        buttons.removeFromRight (4);
        locate.setBounds (buttons.removeFromRight (60));
    }
    else
    {
        openWindow.setBounds (buttons);
    }

    body.removeFromTop (6);

    for (auto& row : pins)
    {
        row->setBounds (body.removeFromTop (rowHeight));
        body.removeFromTop (rowGap);
    }
}

void PluginDeviceCard::paint (juce::Graphics& g)
{
    auto& theme = themeManager.getTheme();
    const auto bounds = getLocalBounds().toFloat();
    const auto radius = theme.radiusXl;

    g.setColour (theme.bgTrack);
    g.fillRoundedRectangle (bounds, radius);

    {
        juce::Graphics::ScopedSaveState save (g);
        juce::Path clip;
        clip.addRoundedRectangle (bounds, radius);
        g.reduceClipRegion (clip);

        // A neutral title bar with a bottom border, and the status footer.
        auto title = getTitleBar();
        g.setColour (theme.bgElevated);
        g.fillRect (title);
        g.setColour (theme.border);
        g.fillRect (title.removeFromBottom (1));

        g.setColour (theme.bgSlot);
        g.fillRect (getLocalBounds().removeFromBottom (footerHeight));
    }

    // Plug icon, name over vendor, format badge.
    auto title = getTitleBar().withTrimmedLeft (power.getRight() + 4).withTrimmedRight (padding);
    drawIcon (g, Icon::plug, title.removeFromLeft (12).toFloat().withSizeKeepingCentre (12.0f, 12.0f), theme.textSecondary);
    title.removeFromLeft (6);

    const auto badgeText = formatBadge();
    const auto badgeStyle = TypeStyle { theme.micro.size, true, 600 };
    const auto badgeWidth = juce::GlyphArrangement::getStringWidthInt (themeManager.font (badgeStyle), badgeText) + 10;
    auto badge = title.removeFromRight (badgeWidth).withSizeKeepingCentre (badgeWidth, 14);
    g.setColour (theme.border);
    g.drawRoundedRectangle (badge.toFloat().reduced (0.5f), theme.radiusSm, 1.0f);
    drawNumber (g, themeManager, badgeText, badgeStyle, badge, juce::Justification::centred, theme.textSecondary);
    title.removeFromRight (4);

    auto text = title.withSizeKeepingCentre (title.getWidth(), 26);
    drawStyledText (g, themeManager, plugin.name, TypeStyle { theme.label.size, false, 700 }, text.removeFromTop (14),
                    juce::Justification::centredLeft, theme.textPrimary);
    drawStyledText (g, themeManager, plugin.manufacturer.isNotEmpty() ? plugin.manufacturer : juce::String ("Unknown vendor"), TypeStyle { theme.micro.size, false, 400 }, text,
                    juce::Justification::centredLeft, theme.textDim);

    auto body = getLocalBounds().withTrimmedTop (titleHeight).withTrimmedBottom (footerHeight).reduced (padding, 8);

    if (plugin.missing)
    {
        auto missing = body.removeFromTop (buttonHeight).removeFromLeft (missingBadgeWidth).withSizeKeepingCentre (missingBadgeWidth, 14);
        g.setColour (theme.rec.withAlpha (0.2f));
        g.fillRoundedRectangle (missing.toFloat(), theme.radiusSm);
        drawStyledText (g, themeManager, "Missing", theme.micro, missing, juce::Justification::centred, theme.rec);
    }
    else if (pins.empty())
    {
        body.removeFromTop (buttonHeight + 6);
        drawStyledText (g, themeManager, "Pin parameters from the menu", TypeStyle { theme.micro.size, false, 400 }, body.removeFromTop (rowHeight),
                        juce::Justification::centredLeft, theme.textDim);
    }

    // Status: CPU, reported latency, sandbox.
    auto footer = getLocalBounds().removeFromBottom (footerHeight).reduced (padding, 0);
    const auto mono = TypeStyle { theme.micro.size, true, 400 };
    drawNumber (g, themeManager, cpuText, mono, footer.removeFromLeft (64), juce::Justification::centredLeft, theme.textSecondary);
    drawNumber (g, themeManager, juce::String (plugin.latencySamples) + " smp", mono, footer.removeFromLeft (56),
                juce::Justification::centredLeft, theme.textSecondary);

    const auto sandboxColour = plugin.sandboxed ? theme.meterLow : theme.textDim;
    auto sandbox = footer.removeFromRight (70);
    drawStyledText (g, themeManager, plugin.sandboxed ? "Sandboxed" : "In-process", TypeStyle { theme.micro.size, false, 500 }, sandbox.withTrimmedLeft (14),
                    juce::Justification::centredLeft, sandboxColour);
    drawIcon (g, Icon::shieldCheck, sandbox.removeFromLeft (11).toFloat().withSizeKeepingCentre (11.0f, 11.0f), sandboxColour);

    // Outline: red dashes when missing, accent-dim while the window is open.
    if (plugin.missing)
    {
        juce::Path outline, dashed;
        outline.addRoundedRectangle (bounds.reduced (1.0f), radius);
        const float dashes[] = { 4.0f, 3.0f };
        juce::PathStrokeType (1.5f).createDashedStroke (dashed, outline, dashes, 2);
        g.setColour (theme.rec);
        g.fillPath (dashed);
    }
    else if (windowOpen)
    {
        g.setColour (theme.accentDim);
        g.drawRoundedRectangle (bounds.reduced (0.75f), radius, 1.5f);
    }
    else
    {
        g.setColour (theme.border);
        g.drawRoundedRectangle (bounds.reduced (0.5f), radius, 1.0f);
    }
}

void PluginDeviceCard::timerCallback()
{
    auto text = "CPU " + juce::String (rack.getCpuLoad (plugin.id) * 100.0, 1) + " %";

    if (text != cpuText)
    {
        cpuText = text;
        repaint (getLocalBounds().removeFromBottom (footerHeight));
    }
}

void PluginDeviceCard::mouseDoubleClick (const juce::MouseEvent& e)
{
    // Opens or focuses the window; a plug-in card never collapses.
    if (getTitleBar().contains (e.getPosition()) && ! power.getBounds().contains (e.getPosition()) && ! plugin.missing
        && onOpenEditor)
        onOpenEditor();
}

void PluginDeviceCard::addMenuItems (juce::PopupMenu& menu)
{
    menu.addItem ("Open Plug-in Window", ! plugin.missing, false, [this] { if (onOpenEditor) onOpenEditor(); });

    juce::PopupMenu pinMenu;
    const auto full = plugin.pinnedParameters.size() >= PluginRack::maxPinnedParameters;

    for (auto& p : rack.getParameters (plugin.id))
    {
        const auto pinned = plugin.pinnedParameters.contains (p.id);
        pinMenu.addItem (p.name, pinned || ! full, pinned,
                         [this, id = p.id, pinned] { commands.invoke (cmd::pluginSetPinned, { plugin.id, id, ! pinned }); });
    }

    menu.addSubMenu ("Pin Parameter", pinMenu, pinMenu.getNumItems() > 0);
    menu.addSeparator();
}

void PluginDeviceCard::showReplaceMenu()
{
    juce::PopupMenu menu;

    for (auto& candidate : rack.getCatalogue())
        if (candidate.instrument == plugin.instrument && ! candidate.midiEffect)
            menu.addItem (candidate.name + (candidate.external ? "  (" + candidate.manufacturer + ")" : juce::String()),
                          [this, path = candidate.path] { commands.invoke (cmd::pluginReplace, { trackId, plugin.id, path }); });

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&replace));
}

} // namespace resamper
