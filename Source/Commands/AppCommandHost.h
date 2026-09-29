#pragma once

#include <juce_core/juce_core.h>

#include <functional>

namespace resamper
{

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

    /** A non-obvious outcome worth a toast (PRD §16.7); undoable offers Undo. */
    std::function<void (const juce::String& message, bool undoable)> notify;

    /** Passes a failed Result's message to reportError; a success does nothing. */
    void report (const juce::Result& r) const
    {
        if (r.failed() && reportError)
            reportError (r.getErrorMessage());
    }
};

} // namespace resamper
