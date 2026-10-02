#pragma once

#include "PluginWindow.h"
#include "Engine/ApplicationModel.h"
#include "UI/MainWindow/Toasts.h"
#include "UI/State/Preferences.h"

#include <map>

namespace resamper
{

/** The plug-in windows of the Edit, and their rules (PRD §9.6 window behaviour).

    - One per instance: open() on a plug-in with a window brings it forward.
    - Placement: the first window centres over the anchor area (the
      arrangement); each further one cascades windowCascade px down-right. A
      window remembers its position on the plug-in, saved with the project; a
      position on a display that is gone comes back to the main display.
    - Pin: pinned windows stay on top and stay up whatever is selected.
      Unpinned windows hide while their track isn't selected (the preference
      "Show plug-in windows for selected track only") and come back when it
      is. Switching views hides nothing.
    - Selecting (clicking) a window selects its track; opening one does too.
    - closeFocused() (Esc on the chrome, Mod+W) and toggleAll() (Mod+Alt+P).
    - The opening rule: pluginAdded() opens an external plug-in's window,
      focused, when the user adds one (if the "Auto-open window on insert"
      preference is on), and offers the toast with Undo and that preference.
    - Each window's open / pinned / position / UI scale is written through
      plugin.setWindow; a window is never left over once its plug-in is gone,
      and a project's open windows come back when it loads.

    The rules are kept apart from what a window shows: windows are made in
    one place (createWindow), and the rules only use a PluginWindow's
    position, pin, focus and callbacks, never its vendor slot. Whether a
    plug-in runs in or out of process (the sandbox, #69) only changes what
    the window reports (PluginInfo::sandboxed) and what onRunInProcess does. */
class PluginWindows : private ApplicationModel::Listener,
                      private juce::ValueTree::Listener,
                      private juce::Timer
{
public:
    PluginWindows (ApplicationModel&, PluginRack&, CommandRegistry&, ThemeManager&, Preferences&);
    ~PluginWindows() override;

    /** The desktop area a first window centres over (the arrangement). The main display's when unset. */
    std::function<juce::Rectangle<int>()> getAnchorArea;

    /** Shows a toast; set by the window that hosts the toasts. */
    std::function<void (const juce::String& message, std::vector<Toasts::Action>)> showToast;

    /** Called after windows open or close (the cards' "Window open · focus"). */
    std::function<void()> onOpenWindowsChanged;

    /** The error state's Run in-process. Until the sandbox (#69) every plug-in
        is in-process, so by default this reloads the plug-in. */
    std::function<void (const juce::String& pluginId)> onRunInProcess;

    /** Opens the plug-in's window, or brings its window forward; focused
        unless focus is false. Does nothing for an unknown or missing plug-in. */
    void open (const juce::String& pluginId, bool focus = true);

    /** Closes a window (saved as closed). */
    void close (const juce::String& pluginId);

    /** Closes the plug-in window that has keyboard focus; false if none has. */
    bool closeFocused();

    /** Hides every plug-in window if any shows, else shows them all again. */
    void toggleAll();

    /** The opening rule: a plug-in the user just added. */
    void pluginAdded (const juce::String& trackId, const juce::String& pluginId);

    void setPinned (const juce::String& pluginId, bool);

    /** Plug-ins with a window, shown or hidden. */
    juce::StringArray getOpenPluginIds() const;
    bool isOpen (const juce::String& pluginId) const;
    bool isShowing (const juce::String& pluginId) const;
    PluginWindow* getWindow (const juce::String& pluginId) const;

    /** The loading timeout every window gets (10 s; tests shorten it). */
    void setLoadTimeoutMs (int);

    /** Applies the rules now (the model notifies asynchronously). */
    void refresh();

private:
    struct Entry
    {
        std::unique_ptr<PluginWindow> window;
        bool hiddenByUser = false;   ///< toggleAll hid it
    };

    ApplicationModel& model;
    PluginRack& rack;
    CommandRegistry& commands;
    ThemeManager& themeManager;
    Preferences& preferences;
    std::map<juce::String, Entry> windows;
    int loadTimeoutMs = PluginWindow::defaultLoadTimeoutMs;
    bool refreshing = false, refreshAgain = false;

    std::unique_ptr<PluginWindow> createWindow (const PluginInfo&);
    juce::String trackNameOf (const juce::String& trackId) const;
    juce::Point<int> placementFor (const PluginWindow&, const PluginWindowState&) const;
    static bool isOnADisplay (juce::Rectangle<int> frame);
    bool shouldShow (const Entry&) const;
    void saveState (const PluginWindow&, bool open);
    void selectTrackOf (const PluginWindow&);
    void announceAndFocus (PluginWindow&);
    void openWindowsChanged();

    void modelChanged() override;
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;
    void timerCallback() override;

    JUCE_DECLARE_WEAK_REFERENCEABLE (PluginWindows)
    JUCE_DECLARE_NON_COPYABLE (PluginWindows)
};

} // namespace resamper
