#pragma once

#include "Engine/PluginRack.h"
#include "UI/Controls/Controls.h"

namespace resamper
{

class CommandRegistry;

/** One plug-in instance's floating window (PRD §9.6, design `PluginWindow`).

    Non-modal, radius 10, elevation L3. Everything except the vendor UI is
    drawn by the host, identically for every vendor:

    - title bar: plug icon, Track › Plug-in, vendor, format badge, a drag
      area, Pin and Close;
    - host toolbar: Bypass, the preset menu (prev / next, name, save), A/B and
      Copy A→B, undo / redo, latency, CPU and the sandbox status;
    - the vendor UI at its native size (times the UI scale), never restyled or
      overlaid; while the plug-in instantiates, a host-drawn loading state,
      and after loadTimeoutMs an error state with Retry and Run in-process;
    - footer: who renders the UI, its format and version, in- or
      out-of-process, the UI scale and, if the plug-in resizes, a grip.

    The window draws its own shadow in a transparent margin around the frame;
    "position" always means the frame's top-left on the desktop. It changes
    the plug-in only through Commands. Where it goes, whether it shows and
    what closing does are PluginWindows' rules: the window reports through
    its callbacks and the manager decides. */
class PluginWindow : public juce::Component,
                     private juce::Timer,
                     private juce::ComponentListener,
                     private ThemeManager::Listener
{
public:
    PluginWindow (PluginRack&, CommandRegistry&, ThemeManager&, const PluginInfo&, const juce::String& trackName);
    ~PluginWindow() override;

    static constexpr int defaultLoadTimeoutMs = 10000;

    const juce::String& getPluginId() const noexcept   { return plugin.id; }
    const juce::String& getTrackId() const noexcept    { return plugin.trackId; }

    /** The plug-in's latest state (name, bypass, preset, A/B, latency) and its track's name. */
    void setState (const PluginInfo&, const juce::String& trackName);

    void setPinned (bool);
    bool isPinned() const noexcept                   { return pinned; }

    /** 100, 150 or 200 (percent). */
    void setUiScale (int percent);
    int getUiScale() const noexcept                  { return uiScale; }

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

    /** The window without its shadow margin, in desktop coordinates. */
    juce::Rectangle<int> getFrameScreenBounds() const;
    void setFramePosition (juce::Point<int> topLeftOnDesktop);

    /** Whether keyboard focus is in this window (host chrome or vendor UI). */
    bool hasFocusInside() const;

    /** Moves keyboard focus from the vendor UI to the host chrome. */
    void focusHost();

    // What the window reports to its manager.
    std::function<void()> onCloseRequested;          ///< Close, Esc on the chrome, Mod+W
    std::function<void()> onActivated;               ///< clicked anywhere (vendor UI too): its track is selected
    std::function<void()> onMoved;                   ///< a title-bar drag ended
    std::function<void (bool pinned)> onPinChanged;
    std::function<void (int percent)> onUiScaleChanged;
    std::function<void()> onToggleAll;               ///< Mod+Alt+P
    std::function<void()> onRunInProcess;            ///< the error state's Run in-process

    void paint (juce::Graphics&) override;
    void resized() override;
    bool hitTest (int x, int y) override;
    bool keyPressed (const juce::KeyPress&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

private:
    class ChromeButton;
    class Readout;
    class Grip;
    struct ClickWatch;

    PluginRack& rack;
    CommandRegistry& commands;
    ThemeManager& themeManager;
    PluginInfo plugin;
    juce::String trackName;
    bool pinned = false;
    int uiScale = 100;
    Status status = Status::loading;
    int loadTimeoutMs = defaultLoadTimeoutMs;
    juce::uint32 loadStartedAt = 0;
    juce::String cpuText;
    juce::BorderSize<int> margin;

    std::unique_ptr<juce::Component> vendor;
    std::unique_ptr<ChromeButton> pin, close, bypass, previousPreset, presetName, nextPreset, savePreset,
                                  slotA, slotB, copyAToB, undo, redo, retry, runInProcess;
    std::unique_ptr<Readout> stats, sandbox, footerInfo;
    std::unique_ptr<Segmented> scale;
    std::unique_ptr<Grip> grip;
    std::unique_ptr<ClickWatch> clickWatch;
    juce::ComponentDragger dragger;
    bool dragging = false;

    juce::Rectangle<int> frame() const;
    juce::Rectangle<int> titleBar() const;
    juce::Rectangle<int> toolbar() const;
    juce::Rectangle<int> vendorArea() const;
    juce::Rectangle<int> footer() const;
    juce::Point<int> vendorSize() const;

    void updateSize();
    void updateTexts();
    void updateAlwaysOnTop();
    void loadVendor();
    void showPresetMenu();
    void askPresetName();
    void stepPreset (int delta);
    void layoutToolbar (juce::Rectangle<int>);
    void layoutFooter (juce::Rectangle<int>);

    void timerCallback() override;
    void componentMovedOrResized (juce::Component&, bool wasMoved, bool wasResized) override;
    void themeChanged() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginWindow)
};

} // namespace resamper
