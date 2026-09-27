#pragma once

#include "Commands/CommandRegistry.h"
#include "Engine/Automation.h"
#include "Engine/Shaper.h"
#include "UI/Arrangement/ArrangementView.h"
#include "UI/Layout/LayoutManager.h"
#include "UI/Mixer/MixerView.h"
#include "UI/PianoRoll/PianoRollView.h"
#include "UI/Plugins/InsertStrip.h"
#include "UI/Plugins/PluginBrowser.h"
#include "UI/Plugins/PluginEditorWindow.h"
#include "UI/Developer/DeveloperOverlay.h"
#include "UI/Developer/LayoutWatcher.h"
#include "UI/Session/SessionView.h"

namespace papercut
{

class LayoutSource;

/** The MainWindow's content: JSON-driven Transport and StatusBar around the
    hand-coded Arrangement, or the Piano Roll when a MIDI clip is open. Also the
    ApplicationCommandTarget that routes menus and keyboard shortcuts into the
    Command registry (ADR-0006). */
class MainComponent : public juce::Component,
                      public juce::ApplicationCommandTarget,
                      private ApplicationModel::Listener,
                      private ThemeManager::Listener
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

    ComponentFactory factory;
    LayoutManager layouts;
    LayoutHost transportHost { "transport", "layouts/transport.json" };
    LayoutHost statusBarHost { "statusbar", "layouts/statusbar.json" };
    ArrangementView arrangement;
    PianoRollView pianoRoll;
    PluginBrowser pluginBrowser;
    InsertStrip insertStrip;
    juce::TextButton editPluginButton { "Edit" };
    SessionView sessionView;
    MixerView mixerView;
    DeveloperOverlay developerOverlay;
    std::unique_ptr<LayoutWatcher> layoutWatch;
    std::unique_ptr<LayoutWatcher> themeWatch;
    std::unique_ptr<PluginEditorWindow> pluginEditor;

    void updateStatusBar();
    void mouseDown (const juce::MouseEvent&) override;

    void modelChanged() override;
    void themeChanged() override;
};

} // namespace papercut
