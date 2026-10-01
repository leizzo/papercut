#pragma once

#include "DeviceCard.h"

#include <vector>

namespace resamper
{

/** `DeviceCard/Plugin` (PRD §9.2.2): a scanned VST3 / AU / CLAP plug-in,
    214 x 164. The host never draws the plug-in's own controls here; they live
    in its window.

    A neutral title bar: power (accent ring and dot), plug icon, name with the
    vendor under it, and an outlined format badge. The body has the Open
    plug-in window button (reading "Window open · focus" while it is, with an
    accent-dim outline on the card) and up to 4 pinned parameters (name, mini
    bar, mono value), edited inline. The status footer shows CPU, reported
    latency and the sandbox state. Double-clicking the title opens or focuses
    the window; the menu pins and unpins parameters.

    A missing plug-in keeps its name, gets a red dashed outline and a Missing
    badge, and offers Locate (scan again) and Replace instead of its window. */
class PluginDeviceCard : public DeviceCard,
                         private juce::Timer
{
public:
    static constexpr int width = 214, titleHeight = 34, footerHeight = 20;

    PluginDeviceCard (CommandRegistry&, PluginRack&, ThemeManager&, const juce::String& trackId, const PluginInfo&);
    ~PluginDeviceCard() override;

    void setState (const PluginInfo&, DeviceSize) override;
    int getPreferredWidth (int) const override   { return width; }
    void setWindowOpen (bool) override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    class PinnedParameter;

    DevicePowerButton power;
    Button openWindow, locate, replace;
    std::vector<std::unique_ptr<PinnedParameter>> pins;
    bool windowOpen = false;
    juce::String cpuText;

    juce::Rectangle<int> getTitleBar() const override;
    void addMenuItems (juce::PopupMenu&) override;
    void timerCallback() override;

    void rebuildPins();
    void showReplaceMenu();

    /** VST3, AU or CLAP: the badge's text. */
    juce::String formatBadge() const;
};

} // namespace resamper
