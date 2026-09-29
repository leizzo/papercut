#include "Commands/AppCommands.h"
#include "Commands/AutomationCommands.h"
#include "Commands/MixerCommands.h"
#include "Commands/PluginCommands.h"
#include "Commands/ProductionCommands.h"
#include "Commands/SessionCommands.h"
#include "Engine/ApplicationModel.h"
#include "Engine/Automation.h"
#include "Engine/EngineManager.h"
#include "Engine/Mixer.h"
#include "Engine/PluginRack.h"
#include "Engine/Production.h"
#include "Engine/ProjectManager.h"
#include "Engine/SamplePreview.h"
#include "Engine/Session.h"
#include "Engine/Shaper.h"
#include "UI/Layout/LayoutSource.h"
#include "UI/MainWindow/MainWindow.h"
#include "UI/State/UIStateStore.h"
#include "UI/Theme/ThemeManager.h"

namespace resamper
{

namespace
{
    juce::File lastProjectFile()
    {
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("Resamper")
                   .getChildFile ("last-project.txt");
    }

    void rememberProjectFolder (const juce::File& folder)
    {
        auto file = lastProjectFile();

        if (file.getParentDirectory().createDirectory().failed())
            return;

        file.replaceWithText (folder.getFullPathName());
    }

    juce::File rememberedProjectFolder()
    {
        auto file = lastProjectFile();

        if (! file.existsAsFile())
            return {};

        return file.loadFileAsString().trim();
    }
}

class ResamperApplication : public juce::JUCEApplication,
                            private juce::Timer
{
public:
    const juce::String getApplicationName() override       { return "Resamper"; }
    const juce::String getApplicationVersion() override    { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override             { return false; }

    void initialise (const juce::String&) override
    {
        if (auto r = theme.load(); r.failed())
        {
            // The embedded theme is part of the build; failing here is a packaging bug.
            reportError ("Cannot start: " + r.getErrorMessage());
            quit();
            return;
        }

        juce::LookAndFeel::setDefaultLookAndFeel (&theme.getLookAndFeel());

        engine = std::make_unique<EngineManager> (getApplicationName(), EngineManager::AudioDevice::initialise);
        projects = std::make_unique<ProjectManager> (*engine);
        model = std::make_unique<ApplicationModel> (*projects);
        production = std::make_unique<Production> (*projects);
        plugins = std::make_unique<PluginRack> (*projects);
        mixer = std::make_unique<Mixer> (*projects);
        session = std::make_unique<Session> (*projects);
        automation = std::make_unique<Automation> (*projects);
        shaper = std::make_unique<Shaper> (*projects);
        preview = std::make_unique<SamplePreview> (*engine);

        wireCommandHost();
        registerAppCommands (commands, *model, commandHost);
        registerProductionCommands (commands, *production, *model, theme, commandHost);
        registerPluginCommands (commands, *plugins, commandHost);
        registerMixerCommands (commands, *mixer, commandHost);
        registerSessionCommands (commands, *session, commandHost);
        registerAutomationCommands (commands, *automation, *shaper, commandHost);

        mainWindow = std::make_unique<MainWindow> (getApplicationName(),
            MainComponent::Services { *model, commands, theme, uiState, layoutSource,
                                      engine->describeActiveAudioDevice(),
                                      commandHost.reportError, *plugins, *mixer, *preview });

        offerRecovery();
        startTimer (Production::autosaveIntervalMs);
    }

    void shutdown() override
    {
        stopTimer();
        mainWindow.reset();
        chooser.reset();

        if (model != nullptr)
            model->stop();

        model.reset();
        preview.reset();
        shaper.reset();
        automation.reset();
        session.reset();
        mixer.reset();
        plugins.reset();
        production.reset();
        projects.reset();
        engine.reset();
        juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
    }

    void systemRequestedQuit() override   { quit(); }

    void timerCallback() override
    {
        if (projects == nullptr)
            return;

        if (commands.invoke ("project.autosave"))
            rememberProjectFolder (projects->getProjectFolder());
    }

    void offerRecovery()
    {
        const auto folder = rememberedProjectFolder();

        if (! Production::hasNewerRecovery (folder) || model == nullptr)
            return;

        const auto recovery = folder.getChildFile ("Recovery");
        const auto message = "A newer recovery copy of \"" + folder.getFileName()
                             + "\" was found. Open it?";

        juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::QuestionIcon,
                                            "Resamper", message, "Open Recovery", "Skip",
                                            mainWindow.get(),
                                            juce::ModalCallbackFunction::create ([this, recovery] (int result)
                                            {
                                                if (result != 1 || model == nullptr)
                                                    return;

                                                juce::var recovered;

                                                if (auto r = model->openProject (recovery, recovered); r.wasOk())
                                                {
                                                    uiState.restore (recovered);
                                                    rememberProjectFolder (recovery);
                                                }
                                                else
                                                {
                                                    reportError (r.getErrorMessage());
                                                }
                                            }));
    }

private:
    LayoutSource layoutSource;
    ThemeManager theme { layoutSource, "themes/dark.json" };
    UIStateStore uiState;

    std::unique_ptr<EngineManager> engine;
    std::unique_ptr<ProjectManager> projects;
    std::unique_ptr<ApplicationModel> model;
    std::unique_ptr<Production> production;
    std::unique_ptr<PluginRack> plugins;
    std::unique_ptr<Mixer> mixer;
    std::unique_ptr<Session> session;
    std::unique_ptr<Automation> automation;
    std::unique_ptr<Shaper> shaper;
    std::unique_ptr<SamplePreview> preview;

    CommandRegistry commands;
    AppCommandHost commandHost;
    std::unique_ptr<juce::FileChooser> chooser;
    std::unique_ptr<MainWindow> mainWindow;

    void choose (const juce::String& title, const juce::String& patterns, int flags, AppCommandHost::FileCallback callback)
    {
        chooser = std::make_unique<juce::FileChooser> (title, juce::File::getSpecialLocation (juce::File::userMusicDirectory), patterns);
        chooser->launchAsync (flags, [cb = std::move (callback)] (const juce::FileChooser& fc)
        {
            if (auto result = fc.getResult(); result != juce::File())
                cb (result);
        });
    }

    void wireCommandHost()
    {
        using FB = juce::FileBrowserComponent;

        commandHost.chooseAudioFile = [this] (auto cb)
        {
            choose ("Add Audio Clip", "*.wav;*.aif;*.aiff;*.flac", FB::openMode | FB::canSelectFiles, std::move (cb));
        };

        commandHost.chooseProjectToOpen = [this] (auto cb)
        {
            choose ("Open Project Folder", {}, FB::openMode | FB::canSelectDirectories, std::move (cb));
        };

        commandHost.chooseProjectSaveLocation = [this] (auto cb)
        {
            choose ("Save Project As (creates a folder)", {}, FB::saveMode | FB::canSelectFiles, std::move (cb));
        };

        commandHost.captureUIState = [this] { return uiState.toVar(); };
        commandHost.restoreUIState = [this] (const juce::var& v) { uiState.restore (v); };
        // Errors are toasts, not modal dialogs (PRD §16.7); before the window exists, a dialog.
        commandHost.reportError = [this] (const juce::String& message)
        {
            if (mainWindow != nullptr)
                mainWindow->showToast (message, false, true);
            else
                reportError (message);
        };

        commandHost.notify = [this] (const juce::String& message, bool undoable)
        {
            if (mainWindow != nullptr)
                mainWindow->showToast (message, undoable);
        };
    }

    static void reportError (const juce::String& message)
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Resamper", message);
    }
};

} // namespace resamper

START_JUCE_APPLICATION (resamper::ResamperApplication)
