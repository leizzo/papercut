#include "Commands/AppCommands.h"
#include "Engine/ApplicationModel.h"
#include "Engine/EngineManager.h"
#include "Engine/ProjectManager.h"
#include "UI/Layout/LayoutSource.h"
#include "UI/MainWindow/MainWindow.h"
#include "UI/State/UIStateStore.h"
#include "UI/Theme/ThemeManager.h"

namespace papercut
{

class PapercutApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override       { return "Papercut"; }
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

        wireCommandHost();
        registerAppCommands (commands, *model, commandHost);

        mainWindow = std::make_unique<MainWindow> (getApplicationName(),
            MainComponent::Services { *model, commands, theme, uiState, layoutSource,
                                      engine->describeActiveAudioDevice(),
                                      reportError });
    }

    void shutdown() override
    {
        mainWindow.reset();
        chooser.reset();

        if (model != nullptr)
            model->stop();

        model.reset();
        projects.reset();
        engine.reset();
        juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
    }

    void systemRequestedQuit() override   { quit(); }

private:
    LayoutSource layoutSource;
    ThemeManager theme { layoutSource, "themes/dark.json" };
    UIStateStore uiState;

    std::unique_ptr<EngineManager> engine;
    std::unique_ptr<ProjectManager> projects;
    std::unique_ptr<ApplicationModel> model;

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
        commandHost.reportError = reportError;
    }

    static void reportError (const juce::String& message)
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Papercut", message);
    }
};

} // namespace papercut

START_JUCE_APPLICATION (papercut::PapercutApplication)
