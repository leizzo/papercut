#pragma once

#include "App/ResamperApp.h"
#include "UI/Layout/LayoutSource.h"
#include "UI/Theme/ThemeManager.h"

namespace resamper::test
{

/** The run's single headless EngineManager (no audio device). */
EngineManager& getEngineManager();

/** Writes a sine-wave WAV file and returns it. A non-zero acidTempo adds an
    ACID loop chunk, as tempo-tagged sample-library loops carry. */
juce::File writeSineWav (const juce::File& file, double seconds, int numChannels = 2, double acidTempo = 0);

/** A fresh untitled Project in the app exactly as the app builds it, every
    Command registered, except that file choosers and messages are plain
    fields. The Theme is not loaded: call theme.load() before building views. */
struct Fixture
{
    Fixture();
    ~Fixture();

    LayoutSource layoutSource;
    ThemeManager theme { layoutSource, "themes/dark.json" };
    ResamperApp app { getEngineManager(), theme };

    // The app's parts, by their short names.
    ProjectManager& projects = app.projects;
    ApplicationModel& model = app.model;
    Production& production = app.production;
    PluginRack& plugins = app.plugins;
    Mixer& mixer = app.mixer;
    Session& session = app.session;
    Automation& automation = app.automation;
    Shaper& shaper = app.shaper;
    SamplePreview& preview = app.preview;
    CommandRegistry& commands = app.commands;
    AppCommandHost& host = app.host;

    // What the choosers "pick". An invalid File means the user cancelled.
    juce::File audioFileToChoose, projectToOpen, projectSaveLocation;

    juce::StringArray errors, notifications;

    juce::TemporaryFile scratch { juce::String() };
    juce::File scratchDir() const   { return scratch.getFile(); }

    bool invoke (const char* commandId, const juce::var& args = {})   { return commands.invoke (commandId, args); }
    int numTracks() const                 { return (int) model.getTracks().size(); }
};

/** Renders the whole Edit offline, as it plays (honouring mute and solo), and
    returns the peak level (0 if nothing rendered). */
float renderPeak (Fixture&);

} // namespace resamper::test
