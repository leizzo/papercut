#pragma once

#include "Commands/CommandRegistry.h"
#include "UI/Arrangement/ArrangementView.h"
#include "UI/Layout/LayoutManager.h"
#include "UI/PianoRoll/PianoRollView.h"

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

    void updateStatusBar();

    void modelChanged() override;
    void themeChanged() override;
};

} // namespace papercut
