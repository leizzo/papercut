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
    track.add    track.remove
    track.setVolume  track.setPan  track.toggleMute  track.toggleSolo
    clip.add     clip.move     clip.resize   clip.split
    edit.undo    edit.redo
    transport.play  transport.stop  transport.togglePlay  transport.returnToStart
*/
void registerAppCommands (CommandRegistry&, ApplicationModel&, AppCommandHost&);

/** Arguments for track.toggleMute and track.toggleSolo. */
juce::var trackArgs (const juce::String& trackId);

/** Arguments for track.setVolume. A continuous gesture (a fader drag) passes
    continuesGesture for every value after its first, making the whole gesture
    one undo step. */
juce::var trackVolumeArgs (const juce::String& trackId, double db, bool continuesGesture = false);

/** Arguments for track.setPan (-1 left to 1 right); continuesGesture as for trackVolumeArgs. */
juce::var trackPanArgs (const juce::String& trackId, double pan, bool continuesGesture = false);

/** Arguments for clip.move: the clip, its new start, and optionally the track to
    move it to. Invoked with anything else, clip.move does nothing. */
juce::var clipMoveArgs (const juce::String& clipId, double startSeconds, const juce::String& trackId = {});

/** Arguments for clip.resize: the clip and its new edges. */
juce::var clipResizeArgs (const juce::String& clipId, double startSeconds, double endSeconds);

} // namespace papercut
