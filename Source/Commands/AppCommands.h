#pragma once

#include "CommandRegistry.h"

#include <functional>

namespace papercut
{

class ApplicationModel;

/** What model Commands need from the app beyond the Application Model.
    The app wires these to file choosers and the UI State store; tests wire
    them to fixed files and plain values. */
struct AppCommandHost
{
    using FileCallback = std::function<void (const juce::File&)>;

    /** Each chooser calls back with the chosen file, or never if cancelled. */
    std::function<void (FileCallback)> chooseAudioFile;
    std::function<void (FileCallback)> chooseProjectToOpen;
    std::function<void (FileCallback)> chooseProjectSaveLocation;

    std::function<juce::var()> captureUIState;
    std::function<void (const juce::var&)> restoreUIState;

    std::function<void (const juce::String& message)> reportError;
};

/** Registers every model-facing Command:

    project.new  project.open  project.save  project.saveAs
    track.add    track.remove  clip.add
    edit.undo    edit.redo
    transport.play  transport.stop  transport.togglePlay  transport.returnToStart
*/
void registerAppCommands (CommandRegistry&, ApplicationModel&, AppCommandHost&);

} // namespace papercut
