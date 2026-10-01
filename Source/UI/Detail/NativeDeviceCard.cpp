#include "NativeDeviceCard.h"
#include "Commands/CommandRegistry.h"
#include "Commands/PluginCommands.h"

namespace resamper
{

namespace
{
    constexpr int knobWidth = 48, knobGap = 10, bodyPadding = 14, dialSize = 34, knobHeight = dialSize + 4 + 2 * 13;
    constexpr int headerPadding = 6, headerGap = 2, iconButton = 20, presetWidth = 52, abWidth = 28, modsWidth = 44,
                  minNameWidth = 60;

    /** Mix, Wet / Dry and Out belong in the Output zone, after the divider. */
    bool isOutputParameter (const juce::String& name)
    {
        for (auto* word : { "mix", "wet", "dry" })
            if (name.containsIgnoreCase (word))
                return true;

        return name.startsWithIgnoreCase ("out");
    }

    int zoneWidth (int columns)
    {
        return columns > 0 ? columns * knobWidth + (columns - 1) * knobGap : 0;
    }

    int minHeaderWidth()
    {
        return 2 * headerPadding + iconButton + minNameWidth + presetWidth + abWidth + modsWidth + 3 * iconButton
             + 7 * headerGap;
    }

    const char* modsTooltip = "Modulators: the Mods Drawer is coming";
}

NativeDeviceCard::NativeDeviceCard (CommandRegistry& c, PluginRack& r, ThemeManager& tm, const juce::String& track,
                                    const PluginInfo& info)
    : DeviceCard (c, r, tm, track, info),
      power (tm, DevicePowerButton::Style::native),
      preset (tm, "Preset: presets are coming with the preset browser", {}, "Default"),
      ab (tm, "A/B compare is coming", {}, "A/B"),
      mods (tm, modsTooltip, {}, "Mods 0"),
      fold (tm, "Fold", Icon::chevronLeft),
      expand (tm, "Expand", Icon::maximize2),
      options (tm, "Options", Icon::ellipsis)
{
    setComponentID ("DeviceCard/Native");
    setDescription ("Native device");

    preset.setComponentID ("preset");
    ab.setComponentID ("ab");
    mods.setComponentID ("mods");
    fold.setComponentID ("fold");
    expand.setComponentID ("expand");
    options.setComponentID ("options");

    // Until they land, these show their state but can't be used.
    preset.setEnabled (false);
    ab.setEnabled (false);
    mods.setEnabled (false);

    power.onClick = [this] { toggleBypass(); };
    fold.onClick = [this] { if (onSizeChange) onSizeChange (toggledSize (size, DeviceSize::folded)); };
    expand.onClick = [this] { if (onSizeChange) onSizeChange (toggledSize (size, DeviceSize::expanded)); };
    options.onClick = [this] { showMenu(); };

    for (auto* b : std::initializer_list<juce::Component*> { &power, &preset, &ab, &mods, &fold, &expand, &options })
        addAndMakeVisible (b);

    rebuild (rack.getParameters (plugin.id));
}

void NativeDeviceCard::rebuild (const std::vector<PluginParameter>& list)
{
    std::vector<juce::String> ids;

    for (auto& p : list)
        ids.push_back (p.id);

    if (ids != parameterIds)
    {
        parameters.clear();
        parameterIds = ids;

        for (auto& p : list)
        {
            auto knob = std::make_unique<Knob> (themeManager, specFor (p), p.name);
            knob->setComponentID (p.id);
            knob->setDialSize (dialSize);
            knob->setTooltip (p.name);
            knob->onChange = setterFor (p.id);
            addChildComponent (*knob);
            parameters.push_back ({ p.id, isOutputParameter (p.name), std::move (knob) });
        }

        // Controls first, then the Output zone, each in the device's own order.
        std::stable_partition (parameters.begin(), parameters.end(), [] (const Parameter& p) { return ! p.output; });
    }

    for (auto& p : list)
        for (auto& shown : parameters)
            if (shown.id == p.id)
            {
                shown.knob->setValue (p.value);
                shown.knob->setAutomated (p.automated);
                shown.knob->setDimmed (! plugin.enabled);
                shown.knob->setArcColour (colour);
            }

    resized();
}

void NativeDeviceCard::setState (const PluginInfo& info, DeviceSize newSize)
{
    auto& theme = themeManager.getTheme();
    plugin = info;
    size = newSize;
    // A stable pick from the track palette by device type (the design: EQ Eight is always arp, Saturator bass).
    colour = theme.trackColour ((int) ((juce::uint32) (plugin.manufacturer + "/" + plugin.name).hashCode()
                                       % (juce::uint32) theme.trackPalette.size()));
    setTitle (plugin.name);
    setAlpha (plugin.enabled ? 1.0f : 0.5f);

    power.setToggleState (plugin.enabled, juce::dontSendNotification);
    power.setDotColour (colour);
    fold.setIcon (size == DeviceSize::folded ? Icon::chevronRight : Icon::chevronLeft);
    fold.setTooltip (size == DeviceSize::folded ? "Unfold" : "Fold");
    expand.setIcon (size == DeviceSize::expanded ? Icon::minimize2 : Icon::maximize2);
    expand.setTooltip (size == DeviceSize::expanded ? "Compact" : "Expand");
    mods.setButtonText (size == DeviceSize::folded ? "0" : "Mods 0");

    rebuild (rack.getParameters (plugin.id));
    repaint();
}

int NativeDeviceCard::columns (int knobs) const
{
    // Expanded stacks two rows; compact is one.
    return size == DeviceSize::expanded ? (knobs + 1) / 2 : knobs;
}

std::vector<NativeDeviceCard::Parameter*> NativeDeviceCard::shownParameters (bool output)
{
    std::vector<Parameter*> shown;

    for (auto& p : parameters)
        if (p.output == output && (output || size == DeviceSize::expanded || (int) shown.size() < maxCompactControls))
            shown.push_back (&p);

    return shown;
}

int NativeDeviceCard::getPreferredWidth (int dockedWidth) const
{
    if (size == DeviceSize::folded)
        return foldedWidth;

    int controls = 0, outputs = 0;

    for (auto& p : parameters)
        ++(p.output ? outputs : controls);

    if (size == DeviceSize::compact)
        controls = juce::jmin (controls, maxCompactControls);

    const auto divider = controls > 0 && outputs > 0 ? 2 * knobGap + 1 : 0;
    const auto body = 2 * bodyPadding + zoneWidth (columns (controls)) + divider + zoneWidth (columns (outputs));
    return juce::jmax (minHeaderWidth(), body, size == DeviceSize::expanded ? dockedWidth : 0);
}

void NativeDeviceCard::focusFirstControl()
{
    // Only a card on screen can take focus.
    for (auto& p : parameters)
        if (p.knob->isShowing())
        {
            p.knob->grabKeyboardFocus();
            return;
        }
}

juce::Rectangle<int> NativeDeviceCard::getTitleBar() const
{
    return size == DeviceSize::folded ? getLocalBounds() : getLocalBounds().removeFromTop (headerHeight);
}

void NativeDeviceCard::addMenuItems (juce::PopupMenu& menu)
{
    auto resize = [this] (DeviceSize s) { return [this, s] { if (onSizeChange) onSizeChange (s); }; };
    menu.addItem ("Fold", true, size == DeviceSize::folded, resize (toggledSize (size, DeviceSize::folded)));
    menu.addItem ("Expand", true, size == DeviceSize::expanded, resize (toggledSize (size, DeviceSize::expanded)));
    menu.addSeparator();
}

void NativeDeviceCard::resized()
{
    const auto folded = size == DeviceSize::folded;

    for (auto* b : std::initializer_list<juce::Component*> { &preset, &ab, &expand, &options })
        b->setVisible (! folded);

    if (folded)
    {
        // A strip: power, unfold, the name down the middle, the Mods count at the foot.
        auto strip = getLocalBounds();
        power.setBounds (strip.removeFromTop (headerHeight));
        fold.setBounds (strip.removeFromTop (iconButton).reduced (4, 0));
        mods.setBounds (strip.removeFromBottom (iconButton + 4).reduced (4, 2));

        for (auto& p : parameters)
            p.knob->setVisible (false);

        dividerX = -1;
        return;
    }

    auto header = getLocalBounds().removeFromTop (headerHeight).reduced (headerPadding, 4);
    power.setBounds (header.removeFromLeft (iconButton));
    header.removeFromLeft (headerGap);

    for (auto* b : { &options, &expand, &fold })
    {
        b->setBounds (header.removeFromRight (iconButton));
        header.removeFromRight (headerGap);
    }

    mods.setBounds (header.removeFromRight (modsWidth));
    header.removeFromRight (headerGap);
    ab.setBounds (header.removeFromRight (abWidth));
    header.removeFromRight (headerGap);
    preset.setBounds (header.removeFromRight (presetWidth));

    for (auto& p : parameters)
        p.knob->setVisible (false);

    // Zones: Controls, then the divider and Output.
    auto body = getLocalBounds().withTrimmedTop (headerHeight).reduced (bodyPadding, 0);
    const auto rows = size == DeviceSize::expanded ? 2 : 1;
    const auto top = headerHeight + (getHeight() - headerHeight - rows * knobHeight) / 2;

    auto place = [&] (const std::vector<Parameter*>& zone)
    {
        const auto cols = columns ((int) zone.size());

        for (size_t i = 0; i < zone.size(); ++i)
        {
            const auto col = (int) i % juce::jmax (1, cols), row = (int) i / juce::jmax (1, cols);
            zone[i]->knob->setBounds (body.getX() + col * (knobWidth + knobGap), top + row * knobHeight, knobWidth, knobHeight);
            zone[i]->knob->setVisible (true);
        }

        body.removeFromLeft (zoneWidth (cols));
    };

    const auto controls = shownParameters (false), outputs = shownParameters (true);
    place (controls);
    dividerX = -1;

    if (! outputs.empty())
    {
        // Output hugs the right edge; the card is always wide enough for the divider's gaps.
        body.removeFromLeft (juce::jmax (0, body.getWidth() - zoneWidth (columns ((int) outputs.size()))));

        if (! controls.empty())
            dividerX = body.getX() - knobGap - 1;

        place (outputs);
    }
}

void NativeDeviceCard::paint (juce::Graphics& g)
{
    auto& theme = themeManager.getTheme();
    const auto bounds = getLocalBounds().toFloat();
    const auto radius = theme.radiusXl;
    const auto folded = size == DeviceSize::folded;

    g.setColour (theme.bgTrack);
    g.fillRoundedRectangle (bounds, radius);

    // The header (the whole strip when folded) in the device colour.
    {
        juce::Graphics::ScopedSaveState save (g);
        juce::Path clip;
        clip.addRoundedRectangle (bounds, radius);
        g.reduceClipRegion (clip);
        g.setColour (colour);
        g.fillRect (getTitleBar());
    }

    g.setColour (theme.textOnAccent);
    g.setFont (themeManager.font (TypeStyle { theme.label.size, false, 700 }));

    if (folded)
    {
        // The name runs down the strip.
        juce::Graphics::ScopedSaveState save (g);
        const auto area = getLocalBounds().withTrimmedTop (headerHeight + iconButton + 4).withTrimmedBottom (iconButton + 8);
        g.addTransform (juce::AffineTransform::rotation (juce::MathConstants<float>::halfPi, (float) area.getCentreX(),
                                                         (float) area.getCentreY()));
        g.drawText (plugin.name, area.withSizeKeepingCentre (area.getHeight(), area.getWidth()), juce::Justification::centredLeft, true);
    }
    else
    {
        const auto name = juce::Rectangle<int>::leftTopRightBottom (power.getRight() + 6, 0, preset.getX() - 4, headerHeight);
        g.drawText (plugin.name, name, juce::Justification::centredLeft, true);
    }

    if (dividerX >= 0)
    {
        g.setColour (theme.borderSoft);
        g.fillRect (dividerX, headerHeight + bodyPadding, 1, getHeight() - headerHeight - 2 * bodyPadding);
    }

    g.setColour (theme.border);
    g.drawRoundedRectangle (bounds.reduced (0.5f), radius, 1.0f);
}

} // namespace resamper
