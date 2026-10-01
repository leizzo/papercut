#pragma once

#include "DeviceCard.h"

#include <vector>

namespace resamper
{

/** `DeviceCard/Plugin` (PRD §9.2.2): a scanned VST3 / AU / CLAP plug-in,
    214 x 164, drawn to the design's component exactly. The host never draws
    the plug-in's own controls here; they live in its window.

    A neutral title bar: power (accent ring and dot), plug icon, name with the
    vendor under it, and an outlined format badge. The body has the Open
    plug-in window button (reading "Window open · focus" in accent while it
    is, with an accent-dim outline on the card) and up to 4 pinned parameters
    (name, mini bar, mono value), edited inline. The status footer shows CPU,
    reported latency and the sandbox state. Double-clicking the title opens or
    focuses the window.

    Pinning: the pin button in the Pinned Parameters header starts learning
    and opens the window; each parameter the user then takes hold of in the
    window is pinned, until the pin button is clicked again or 4 are pinned.
    The card's menu pins and unpins by name too.

    A missing plug-in keeps its name, gets a red dashed outline and a Missing
    badge, and offers Locate (point at its file) and Replace instead of its window. */
class PluginDeviceCard : public DeviceCard,
                         private juce::Timer
{
public:
    static constexpr int width = 214;

    PluginDeviceCard (CommandRegistry&, PluginRack&, ThemeManager&, const juce::String& trackId, const PluginInfo&);
    ~PluginDeviceCard() override;

    void setState (const PluginInfo&) override;
    int getPreferredWidth (int) const override   { return width; }
    void setWindowOpen (bool) override;

    /** Whether touching a parameter in the window pins it. */
    bool isLearningPins() const noexcept         { return touchWatch != nullptr; }

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    class CardButton;
    class PinnedParameter;

    DevicePowerButton power;
    std::unique_ptr<CardButton> openWindow, locate, replace, pinLearn;
    std::vector<std::unique_ptr<PinnedParameter>> pins;
    std::unique_ptr<PluginRack::TouchWatch> touchWatch;
    bool windowOpen = false;
    juce::String cpuText;

    juce::Rectangle<int> getTitleBar() const override;
    void addMenuItems (juce::PopupMenu&) override;
    void timerCallback() override;

    void rebuildPins();
    void setLearningPins (bool);
    void showReplaceMenu();

    juce::Rectangle<int> body() const;
    juce::Rectangle<int> pinnedHeader() const;

    /** VST3, AU or CLAP: the badge's text. */
    juce::String formatBadge() const;
};

} // namespace resamper
