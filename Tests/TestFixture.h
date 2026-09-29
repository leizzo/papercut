#pragma once

#include "Engine/EngineManager.h"
#include "Engine/ProjectManager.h"
#include "Engine/ApplicationModel.h"
#include "Commands/AppCommands.h"

namespace resamper::test
{

/** The run's single headless EngineManager (no audio device). */
EngineManager& getEngineManager();

/** Writes a sine-wave WAV file and returns it. A non-zero acidTempo adds an
    ACID loop chunk, as tempo-tagged sample-library loops carry. */
juce::File writeSineWav (const juce::File& file, double seconds, int numChannels = 2, double acidTempo = 0);

/** A fresh untitled Project with the Command registry wired exactly as the app
    wires it, except that file choosers and UI State are plain fields. */
struct Fixture
{
    Fixture();
    ~Fixture();

    ProjectManager projects { getEngineManager() };
    ApplicationModel model { projects };
    CommandRegistry commands;
    AppCommandHost host;

    // What the choosers "pick". An invalid File means the user cancelled.
    juce::File audioFileToChoose, projectToOpen, projectSaveLocation;

    // Stand-in for the UI State store.
    juce::var uiState;
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
