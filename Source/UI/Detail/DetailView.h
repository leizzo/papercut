#pragma once

#include "DeviceCard.h"
#include "Engine/ApplicationModel.h"
#include "UI/State/ShellState.h"

namespace resamper
{

class CommandRegistry;

/** The bottom detail view (PRD §6.3), for the selected track or clip: the clip
    panel (230) and the track's device chain, a horizontal row of DeviceCards.
    Its top edge drags its height (120–420, kept in the shell's UI State).

    The chain ends in a drop zone. Browser devices dropped on the chain go to
    its end through plugin.insert: a native device then takes keyboard focus;
    a plug-in's window opens by the opening rule (PluginWindows). A card
    dragged by its title bar moves within it. A native device's size is saved
    on the device (plugin.setSize). The whole content can be swapped for
    another inspector (the Automation and Folder / Bus inspectors, later). */
class DetailView : public juce::Component,
                   private ApplicationModel::Listener
{
public:
    DetailView (ApplicationModel&, PluginRack&, CommandRegistry&, ThemeManager&, ShellState&, juce::ValueTree uiState);
    ~DetailView() override;

    std::function<void (const juce::String& pluginId)> onOpenEditor;

    /** Opens a native device, expanded, in its own window (one at a time). It
        follows the device's state and closes when the device goes. */
    void openDeviceWindow (const juce::String& trackId, const juce::String& pluginId);

    /** The plug-ins whose windows are open; their cards say so. */
    void setOpenWindows (const juce::StringArray& pluginIds);

    /** Shows this inspector in place of the clip panel and chain; nullptr restores them. */
    void setInspector (juce::Component*);

    /** Scrolls the device chain to its start (the mixer's Track chain link). */
    void revealDeviceChain();

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    struct ClipPanel;
    struct Chain;
    struct DeviceWindow;

    ApplicationModel& model;
    PluginRack& rack;
    CommandRegistry& commands;
    ThemeManager& themeManager;
    ShellState& shell;
    juce::ValueTree state;

    std::unique_ptr<ClipPanel> clipPanel;
    std::unique_ptr<Chain> chain;
    juce::Viewport chainView;
    juce::Component* inspector = nullptr;
    juce::StringArray openWindowIds;
    std::unique_ptr<DeviceWindow> deviceWindow;
    int heightAtDragStart = 0;

    void refresh();
    void refreshDeviceWindow();
    bool onResizeEdge (juce::Point<int>) const;
    void modelChanged() override   { refresh(); }
};

} // namespace resamper
