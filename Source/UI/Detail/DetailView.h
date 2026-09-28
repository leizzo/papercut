#pragma once

#include "DeviceCard.h"
#include "Engine/ApplicationModel.h"
#include "UI/State/ShellState.h"

namespace papercut
{

class CommandRegistry;

/** The bottom detail view (PRD §6.3), for the selected track or clip: the clip
    panel (230) and the track's device chain, a horizontal row of DeviceCards.
    Its top edge drags its height (120–420, kept in the shell's UI State).

    Browser devices dropped on the chain go to its end; a card dragged by its
    title bar moves within it. The whole content can be swapped for another
    inspector (the Automation and Folder / Bus inspectors, later). */
class DetailView : public juce::Component,
                   private ApplicationModel::Listener
{
public:
    DetailView (ApplicationModel&, PluginRack&, CommandRegistry&, ThemeManager&, ShellState&, juce::ValueTree uiState);
    ~DetailView() override;

    std::function<void (const juce::String& pluginId)> onOpenEditor;

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
    int heightAtDragStart = 0;

    void refresh();
    bool onResizeEdge (juce::Point<int>) const;
    void modelChanged() override   { refresh(); }
};

} // namespace papercut
