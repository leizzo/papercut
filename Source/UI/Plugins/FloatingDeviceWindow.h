#pragma once

#include "Engine/PluginRack.h"
#include "UI/Controls/Controls.h"

namespace resamper
{

/** A window floating over Resamper's main window for one device on a track:
    a plug-in's window (PluginWindow) or a native device's floating Expanded
    editor (NativeDeviceWindow). PluginWindows opens both and gives both the
    same rules (PRD §9.6, #70): one per instance, Pin, hidden while its track
    isn't selected unless pinned, a click inside selects its track.

    What the two share is drawn and handled here:

    - the frame: radius 10, elevation L3, its shadow drawn in a transparent
      margin around it; "position" always means the frame's top-left on the
      desktop, and clicks on the shadow fall through;
    - the title bar ($bg-panel, bottom border) with Pin and Close at its
      right; what is left of it is the drag area;
    - pinned windows stay on top; unpinned ones float while Resamper is the
      front app, so the main window never buries them and they never cover
      another app;
    - Esc and Mod+W close, Mod+Alt+P shows / hides every window; any other
      key its content doesn't use goes on (onUnhandledKey), so Resamper's
      shortcuts (Space plays) work with a device window in front.

    The window reports through its callbacks; where it goes, whether it
    shows and what closing does are PluginWindows' rules. */
class FloatingDeviceWindow : public juce::Component
{
public:
    ~FloatingDeviceWindow() override;

    const juce::String& getPluginId() const noexcept   { return plugin.id; }
    const juce::String& getTrackId() const noexcept    { return plugin.trackId; }

    /** The device's latest state and its track's name. */
    virtual void setState (const PluginInfo&, const juce::String& trackName) = 0;

    void setPinned (bool);
    bool isPinned() const noexcept                   { return pinned; }

    /** The UI scale in percent (100, 150 or 200). Only a plug-in's window
        offers one; any other stays at 100. */
    virtual void setUiScale (int) {}
    virtual int getUiScale() const                   { return 100; }

    /** The window without its shadow margin, in desktop coordinates. */
    juce::Rectangle<int> getFrameScreenBounds() const;
    void setFramePosition (juce::Point<int> topLeftOnDesktop);

    /** Whether keyboard focus is in this window. */
    bool hasFocusInside() const;

    /** Moves keyboard focus to the host chrome. */
    void focusHost();

    // What the window reports to its manager.
    std::function<void()> onCloseRequested;          ///< Close, Esc on the chrome, Mod+W
    std::function<void()> onActivated;               ///< clicked anywhere (its content too): its track is selected
    std::function<void()> onMoved;                   ///< a title-bar drag ended
    std::function<void (bool pinned)> onPinChanged;
    std::function<void (int percent)> onUiScaleChanged;
    std::function<void()> onToggleAll;               ///< Mod+Alt+P
    std::function<bool (const juce::KeyPress&)> onUnhandledKey;   ///< a key neither its content nor the window used

    void paint (juce::Graphics&) override;
    bool hitTest (int x, int y) override;
    bool keyPressed (const juce::KeyPress&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

protected:
    /** componentId names the kind of window ("PluginWindow", "NativeDeviceWindow"). */
    FloatingDeviceWindow (ThemeManager&, const PluginInfo&, const juce::String& trackName, const juce::String& componentId);

    /** The chrome's buttons: an icon (Pin, Close, the preset steps, undo /
        redo), the framed Bypass (power + On / Off, accent when on), a text
        button (the preset name, Copy A→B, Retry, Run in-process) or an A/B
        slot (accent when selected). Each is focusable and says what it does (§18). */
    class ChromeButton : public ThemedButton
    {
    public:
        enum class Kind { icon, bypass, text, slot };

        ChromeButton (ThemeManager&, const juce::String& name, Kind, std::optional<Icon> = {});

        bool accentWhenOn = false;   ///< an icon drawn in the accent while on (Pin)

        void paintButton (juce::Graphics&, bool highlighted, bool down) override;

    private:
        Kind kind;
        std::optional<Icon> icon;
    };

    /** The design's corner for the chrome's framed controls (Bypass, the preset name, the A/B well). */
    static constexpr float controlRadius = 5.0f;

    /** Title bar padding 0 8 0 12, gap 7; an icon button's target. */
    static constexpr int titlePaddingLeft = 12, titlePaddingRight = 8, gap = 7, iconTarget = 22;

    ThemeManager& themeManager;
    PluginInfo plugin;
    juce::String trackName;

    /** The window without its shadow margin, in its own coordinates. */
    juce::Rectangle<int> frame() const;
    juce::Rectangle<int> titleBar() const;

    /** The title bar left of Pin: where the title is drawn and a drag moves the window. */
    juce::Rectangle<int> titleArea() const;

    /** Sizes the frame, keeping its top-left where it is on the desktop. */
    void setFrameSize (int width, int height);

    /** Places Pin and Close at the title bar's right; call from resized(). */
    void layoutTitleBar();

    /** Draws one part of the title from area's left (at its text width), then a gap. */
    void drawTitleText (juce::Graphics&, juce::Rectangle<int>& area, const juce::String&, const TypeStyle&, juce::Colour) const;

    /** Draws "Track › Device" from area's left. */
    void drawTrackAndName (juce::Graphics&, juce::Rectangle<int>& area) const;

    /** Paints what is under the title bar, clipped to the frame. */
    virtual void paintBody (juce::Graphics&) {}

    /** Paints the title (titleArea()) over the title bar. */
    virtual void paintTitle (juce::Graphics&) = 0;

    /** Esc pressed while focus is in the window's content: hand focus back to
        the chrome and return true, or return false to close the window. */
    virtual bool releaseContentFocus()               { return false; }

private:
    struct ClickWatch;

    std::unique_ptr<ChromeButton> pin, close;
    std::unique_ptr<ClickWatch> clickWatch;
    juce::TimedCallback foregroundWatch;
    juce::BorderSize<int> margin;
    juce::ComponentDragger dragger;
    bool pinned = false, dragging = false;

    void updateAlwaysOnTop();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FloatingDeviceWindow)
};

} // namespace resamper
