#pragma once

#include "Engine/EngineManager.h"
#include "Engine/ProjectManager.h"
#include "Engine/ApplicationModel.h"
#include "Commands/AppCommands.h"

namespace papercut::test
{

/** The run's single headless EngineManager (no audio device). */
EngineManager& getEngineManager();

/** Writes a sine-wave WAV file and returns it. */
juce::File writeSineWav (const juce::File& file, double seconds, int numChannels = 2);

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
    juce::StringArray errors;

    juce::TemporaryFile scratch { juce::String() };
    juce::File scratchDir() const   { return scratch.getFile(); }

    bool invoke (const char* commandId)   { return commands.invoke (commandId); }
    int numTracks() const                 { return (int) model.getTracks().size(); }
};

} // namespace papercut::test
