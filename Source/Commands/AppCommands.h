#pragma once

#include "CommandRegistry.h"
#include "Engine/ApplicationModel.h"

#include <functional>

namespace papercut
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
};

/** Registers every model-facing Command:

    project.new  project.open  project.save  project.saveAs
    track.add    track.addMidi   track.remove
    track.setVolume  track.setPan  track.toggleMute  track.toggleSolo
    track.setInput   track.toggleArm
    clip.add     clip.addMidi  clip.move     clip.resize   clip.split   clip.setTake
    note.add     note.delete   note.move     note.resize   note.setVelocity   note.quantize
    edit.undo    edit.redo
    transport.play  transport.stop  transport.togglePlay  transport.returnToStart
    transport.setPosition
    transport.record  transport.toggleLoop  transport.setLoopRange
*/
void registerAppCommands (CommandRegistry&, ApplicationModel&, AppCommandHost&);

/** Arguments for track.toggleMute, track.toggleSolo and track.toggleArm. */
juce::var trackArgs (const juce::String& trackId);

/** Arguments for track.setVolume. A continuous gesture (a fader drag) passes
    continuesGesture for every value after its first, making the whole gesture
    one undo step. */
juce::var trackVolumeArgs (const juce::String& trackId, double db, bool continuesGesture = false);

/** Arguments for track.setPan (-1 left to 1 right); continuesGesture as for trackVolumeArgs. */
juce::var trackPanArgs (const juce::String& trackId, double pan, bool continuesGesture = false);

/** Arguments for track.setInput: an input named by ApplicationModel::getAudioInputs(),
    or empty for none. */
juce::var trackInputArgs (const juce::String& trackId, const juce::String& inputName);

/** Arguments for clip.move: the clip, its new start, and optionally the track to
    move it to. Invoked with anything else, clip.move does nothing. */
juce::var clipMoveArgs (const juce::String& clipId, double startSeconds, const juce::String& trackId = {});

/** Arguments for clip.resize: the clip and its new edges. */
juce::var clipResizeArgs (const juce::String& clipId, double startSeconds, double endSeconds);

/** Arguments for clip.setTake: the clip and a 0-based take index. */
juce::var clipTakeArgs (const juce::String& clipId, int takeIndex);

/** Arguments for note.add. Times are seconds from the clip's start. Velocity
    defaults to ApplicationModel::defaultNoteVelocity. */
juce::var noteAddArgs (const juce::String& clipId, double startSeconds, double lengthSeconds, int pitch,
                       int velocity = ApplicationModel::defaultNoteVelocity);

/** Arguments for note.move: every named note shifts by the same amount. */
juce::var noteMoveArgs (const juce::String& clipId, const juce::StringArray& noteIds, double deltaSeconds, int deltaPitch);

/** Arguments for note.resize: one note's new edges, seconds from the clip's start. */
juce::var noteResizeArgs (const juce::String& clipId, const juce::String& noteId, double startSeconds, double endSeconds);

/** Arguments for note.setVelocity. continuesGesture joins a drag into one undo step. */
juce::var noteVelocityArgs (const juce::String& clipId, int velocity, bool continuesGesture = false);

/** Arguments for note.quantize. grid is "1/4", "1/8" or "1/16". */
juce::var noteQuantizeArgs (const juce::String& clipId, const juce::String& grid);

/** Arguments for transport.setLoopRange, which also turns looping on. */
juce::var loopRangeArgs (double startSeconds, double endSeconds);

/** Arguments for transport.setPosition: where the playhead moves, in seconds. */
juce::var transportPositionArgs (double seconds);

} // namespace papercut
