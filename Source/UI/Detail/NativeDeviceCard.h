#pragma once

#include "DeviceCard.h"
#include "UI/Controls/ContinuousControl.h"

#include <vector>

namespace resamper
{

/** `DeviceCard/Native`, the v2 contract (PRD §9.2.1a): a built-in device,
    edited inline with Resamper controls.

    One 28 px `DeviceHeader` in the device colour, in the same order on every
    device: power, name, preset, A/B, Mods (its count), fold, expand, options.
    The body runs its zones left to right, Controls then Output: Mix and Out
    are always last, behind a divider. A knob with automation shows a red dot.

    Folded is a 28 px strip with the power, the name running down it and the
    Mods count. Compact (the default) shows the first controls and the outputs
    and never scrolls. Expanded docks across the detail view and shows every
    parameter. A device's colour comes from its type, so the same device looks
    the same on every track.

    The preset menu, A/B compare and the Mods Drawer come with their own
    tickets; until then those buttons show their state and are disabled. */
class NativeDeviceCard : public DeviceCard
{
public:
    static constexpr int headerHeight = 28, foldedWidth = 28, maxCompactControls = 4;

    NativeDeviceCard (CommandRegistry&, PluginRack&, ThemeManager&, const juce::String& trackId, const PluginInfo&);

    void setState (const PluginInfo&, DeviceSize) override;
    int getPreferredWidth (int dockedWidth) const override;
    void focusFirstControl() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Parameter
    {
        juce::String id;
        bool output = false;   ///< in the Output zone: Mix, Wet / Dry, Out
        std::unique_ptr<Knob> knob;
    };

    juce::Colour colour;
    DeviceSize size = DeviceSize::compact;
    DevicePowerButton power;
    DeviceHeaderButton preset, ab, mods, fold, expand, options;
    std::vector<Parameter> parameters;
    std::vector<juce::String> parameterIds;   ///< in the device's order, to tell when it changes
    int dividerX = -1;

    juce::Rectangle<int> getTitleBar() const override;
    void addMenuItems (juce::PopupMenu&) override;

    void rebuild (const std::vector<PluginParameter>&);

    /** The knobs this size shows, Controls first, then Output. */
    std::vector<Parameter*> shownParameters (bool output);
    int columns (int knobs) const;
};

} // namespace resamper
