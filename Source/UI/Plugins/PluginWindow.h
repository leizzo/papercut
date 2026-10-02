#pragma once

#include "FloatingDeviceWindow.h"

namespace resamper
{

class CommandRegistry;

/** One plug-in instance's floating window (PRD §9.6, design `PluginWindow`).

    Non-modal, radius 10, elevation L3 (FloatingDeviceWindow). Everything
    except the vendor UI is drawn by the host, identically for every vendor:

    - title bar: plug icon, Track › Plug-in, vendor, format badge, a drag
      area, Pin and Close;
    - host toolbar: Bypass, the preset menu (prev / next, name, save), A/B and
      Copy A→B, undo / redo, Parameters, latency, CPU and the sandbox status;
    - the vendor UI at its native size (times the UI scale), never restyled;
      the toolbar's Parameters swaps it for a host-drawn panel of the
      plug-in's parameters in the same place, and back. While the plug-in
      instantiates, a host-drawn loading state, and after loadTimeoutMs an
      error state with Retry and Run in-process;
    - footer: who renders the UI, its format and version, in- or
      out-of-process, the UI scale and, if the plug-in resizes, a grip.

    It changes the plug-in only through Commands. Where it goes, whether it
    shows and what closing does are PluginWindows' rules: the window reports
    through its callbacks and the manager decides. */
class PluginWindow : public FloatingDeviceWindow,
                     private juce::Timer,
                     private juce::ComponentListener,
                     private ThemeManager::Listener
{
public:
    PluginWindow (PluginRack&, CommandRegistry&, ThemeManager&, const PluginInfo&, const juce::String& trackName);
    ~PluginWindow() override;

    static constexpr int defaultLoadTimeoutMs = 10000;

    /** The plug-in's latest state (name, bypass, preset, A/B, latency) and its track's name. */
    void setState (const PluginInfo&, const juce::String& trackName) override;

    /** 100, 150 or 200 (percent). */
    void setUiScale (int percent) override;
    int getUiScale() const override                  { return uiScale; }

    enum class Status { loading, ready, failed };
    Status getStatus() const noexcept                { return status; }

    /** How long instantiating may take before the error state shows. */
    void setLoadTimeoutMs (int ms)                   { loadTimeoutMs = ms; }

    /** Starts instantiating again: the loading state, then the vendor UI. */
    void retryLoading();

    /** Whether the window can resize its vendor UI (the plug-in resizes). */
    bool hasResizeGrip() const;

    /** The vendor UI, once loaded; nullptr while loading or failed. */
    juce::Component* getVendorComponent() const noexcept   { return vendor.get(); }

    /** Shows the plug-in's parameters in place of its vendor UI, or the vendor UI again. */
    void showParameters (bool);

    /** Whether the parameters show in place of the vendor UI. */
    bool isShowingParameters() const noexcept        { return parameterPanel != nullptr; }

    std::function<void()> onRunInProcess;            ///< the error state's Run in-process

    void resized() override;

private:
    class Readout;
    class Grip;

    PluginRack& rack;
    CommandRegistry& commands;
    int uiScale = 100;
    Status status = Status::loading;
    int loadTimeoutMs = defaultLoadTimeoutMs;
    juce::uint32 loadStartedAt = 0;
    juce::String cpuText;

    std::unique_ptr<juce::Component> vendor, parameterPanel;
    std::unique_ptr<ChromeButton> bypass, previousPreset, presetName, nextPreset, savePreset,
                                  slotA, slotB, copyAToB, undo, redo, parameters, retry, runInProcess;
    std::unique_ptr<Readout> stats, sandbox, footerInfo;
    std::unique_ptr<Segmented> scale;
    std::unique_ptr<Grip> grip;

    juce::Rectangle<int> toolbar() const;
    juce::Rectangle<int> vendorArea() const;
    juce::Rectangle<int> footer() const;
    juce::Point<int> vendorSize() const;

    void updateSize();
    void updateTexts();
    void loadVendor();
    void showPresetMenu();
    void askPresetName();
    void stepPreset (int delta);
    void layoutToolbar (juce::Rectangle<int>);
    void layoutFooter (juce::Rectangle<int>);

    void paintBody (juce::Graphics&) override;
    void paintTitle (juce::Graphics&) override;
    bool releaseContentFocus() override;

    void timerCallback() override;
    void componentMovedOrResized (juce::Component&, bool wasMoved, bool wasResized) override;
    void themeChanged() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginWindow)
};

} // namespace resamper
