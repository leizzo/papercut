#pragma once

#include "Commands/CommandRegistry.h"
#include "Engine/Automation.h"
#include "Engine/Shaper.h"
#include "UI/Arrangement/ArrangementView.h"
#include "UI/Layout/LayoutManager.h"
#include "UI/Mixer/MixerView.h"
#include "UI/PianoRoll/PianoRollView.h"
#include "UI/Plugins/InsertStrip.h"
#include "UI/Browser/Browser.h"
#include "Engine/SamplePreview.h"
#include "UI/Plugins/PluginEditorWindow.h"
#include "UI/Developer/DeveloperOverlay.h"
#include "UI/Developer/LayoutWatcher.h"
#include "UI/Session/SessionView.h"
#include "UI/State/ShellState.h"
#include "TopBar.h"

namespace papercut
{

class LayoutSource;

/** The MainWindow's content (PRD §5–6): the top bar, then the view the shell
    shows. Session and Arrange sit between the Browser (left) and the detail
    view (bottom); Mixer, Piano Roll and Editor fill the window. Views are kept
    alive while hidden, so each keeps its scroll and zoom. In Developer Mode a
    JSON status bar and the developer overlay sit at the bottom. Also the
    ApplicationCommandTarget that routes menus and keyboard shortcuts into the
    Command registry. */
class MainComponent : public juce::Component,
                      public juce::ApplicationCommandTarget,
                      public juce::DragAndDropContainer,
                      private ApplicationModel::Listener,
                      private ThemeManager::Listener,
                      private juce::ValueTree::Listener
{
public:
    struct Services
    {
        ApplicationModel& model;
        CommandRegistry& commands;
        ThemeManager& themeManager;
        UIStateStore& uiState;
        const LayoutSource& layoutSource;
        juce::String audioDeviceDescription;
        std::function<void (const juce::String&)> reportError;
        PluginRack& plugins;
        Mixer& mixer;
        Session& session;
        Automation& automation;
        Shaper& shaper;
        SamplePreview& preview;
    };

    MainComponent (Services, juce::ApplicationCommandManager&);
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // ApplicationCommandTarget
    juce::ApplicationCommandTarget* getNextCommandTarget() override   { return nullptr; }
    void getAllCommands (juce::Array<juce::CommandID>&) override;
    void getCommandInfo (juce::CommandID, juce::ApplicationCommandInfo&) override;
    bool perform (const InvocationInfo&) override;

private:
    Services services;
    juce::ApplicationCommandManager& commandManager;

    ShellState shell;
    ComponentFactory factory;
    LayoutManager layouts;
    LayoutHost statusBarHost { "statusbar", "layouts/statusbar.json" };
    TopBar topBar;
    ArrangementView arrangement;
    PianoRollView pianoRoll;
    Browser browser;
    InsertStrip insertStrip;
    juce::TextButton editPluginButton { "Edit" };
    SessionView sessionView;
    MixerView mixerView;

    /** A view that isn't built yet, or has nothing to show. */
    struct Placeholder : juce::Component
    {
        Placeholder (ThemeManager& tm, juce::String t) : themeManager (tm), text (std::move (t)) {}
        void paint (juce::Graphics&) override;
        ThemeManager& themeManager;
        juce::String text;
    };

    Placeholder editorPlaceholder, pianoRollPlaceholder;
    DeveloperOverlay developerOverlay;
    std::unique_ptr<LayoutWatcher> layoutWatch;
    std::unique_ptr<LayoutWatcher> themeWatch;
    std::unique_ptr<PluginEditorWindow> pluginEditor;

    void updateStatusBar();
    void showMenu (const juce::String& name, juce::Rectangle<int> screenArea);
    void openPianoRollForSelection();
    void mouseDown (const juce::MouseEvent&) override;
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;

    void modelChanged() override;
    void themeChanged() override;
};

} // namespace papercut
