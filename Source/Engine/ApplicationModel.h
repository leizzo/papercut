#pragma once

#include "ClipWaveform.h"

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>

namespace papercut
{

class ProjectManager;

/** Read-only snapshot of a clip, for views. */
struct ClipInfo
{
    juce::String id;
    juce::String name;
    double startSeconds = 0;
    double lengthSeconds = 0;
    double sourceOffsetSeconds = 0;   ///< where in the source file the clip starts
    double sourceLengthSeconds = 0;   ///< how long the source audio lasts on the timeline
    juce::File file;
    bool selected = false;
    int numTakes = 0;       ///< passes of a loop recording; 0 for a clip without takes
    int currentTake = -1;   ///< the take playing, or -1
};

/** Read-only snapshot of an audio track, for views. */
struct TrackInfo
{
    juce::String id;
    juce::String name;
    bool selected = false;
    double volumeDb = 0;   ///< ApplicationModel::minVolumeDb is silence
    double pan = 0;        ///< -1 (left) to 1 (right)
    bool muted = false;
    bool solo = false;
    juce::String input;    ///< the audio input it records from, or empty
    bool armed = false;    ///< records its input when the transport records
    std::vector<ClipInfo> clips;
};

/** A recording in progress on one track. */
struct RecordingInfo
{
    juce::String trackId;
    double startSeconds = 0;
    double lengthSeconds = 0;
};

/** A time range on the timeline, in seconds. */
struct TimeRangeSeconds
{
    double start = 0, end = 0;
};

/** The facade over the current Edit (ADR-0001).

    Exposes app-level operations and read-only snapshots; owns no track or clip
    state of its own. Nothing above this layer sees a Tracktion header.

    Undoable (Engine Undo): adding/removing tracks, track volume and pan, and
    inserting, moving, resizing and splitting clips, switching a clip's take, and
    each recording — one undo step per call that changes something. Never
    undoable: transport (including the loop), selection, mute, solo, track input
    and arming (the engine keeps mute, solo and inputs out of its UndoManager). The track and clip
    operations return false, recording no undo step, when they would change
    nothing (unknown clip or track, same value or position, empty range).
*/
class ApplicationModel
{
public:
    struct Listener
    {
        virtual ~Listener() = default;

        /** Called asynchronously on the message thread after tracks, clips,
            selection or the current Project changed. */
        virtual void modelChanged() = 0;
    };

    explicit ApplicationModel (ProjectManager&);
    ~ApplicationModel();

    //==============================================================================
    // Project
    void newProject();
    juce::Result openProject (const juce::File& folder, juce::var& uiState);
    juce::Result saveProject (const juce::var& uiState);
    juce::Result saveProjectAs (const juce::File& folder, const juce::var& uiState);
    bool isProjectUntitled() const;
    juce::String getProjectName() const;

    //==============================================================================
    // Model mutations (each one is a single Engine Undo step)
    void addAudioTrack();

    /** Removes the selected track, or the last one if none is selected.
        Returns false if there is no track to remove. */
    bool removeTrack();

    /** Sets a track's volume, clamped to [minVolumeDb, maxVolumeDb].

        With continuesGesture, the change joins the undo step of the previous call
        if that was a volume change on the same track with nothing undoable in
        between — so a whole fader drag is one undo step. Otherwise it starts one. */
    bool setTrackVolume (const juce::String& trackId, double db, bool continuesGesture = false);

    /** Sets a track's pan, clamped to [-1, 1]; continuesGesture as for setTrackVolume. */
    bool setTrackPan (const juce::String& trackId, double pan, bool continuesGesture = false);

    /** Never undoable. */
    bool setTrackMuted (const juce::String& trackId, bool muted);
    bool setTrackSolo (const juce::String& trackId, bool solo);

    //==============================================================================
    // Audio inputs (never undoable)

    /** The engine's enabled audio inputs, by name. */
    juce::StringArray getAudioInputs() const;

    /** Makes the named input the track's only one; an empty name removes the
        track's input, which also disarms it. Returns false for an unknown track
        or input, or if nothing changed. */
    bool setTrackInput (const juce::String& trackId, const juce::String& inputName);

    /** Arms or disarms a track. Arming a track without an input first gives it
        the first audio input; it fails if there is none. */
    bool setTrackArmed (const juce::String& trackId, bool armed);

    /** The engine's fader range; minVolumeDb is silence. */
    static constexpr double minVolumeDb = -100.0, maxVolumeDb = 6.0;

    /** Inserts the file as a clip at the end of the selected track, or the
        selected clip's track (or the first track, creating one if the Edit has none). */
    juce::Result insertAudioClip (const juce::File&);

    /** Moves a clip to start at startSeconds (clamped to the Edit start), onto
        the track trackId if given. */
    bool moveClip (const juce::String& clipId, double startSeconds, const juce::String& trackId = {});

    /** Sets a clip's edges, clamped to its source audio. The audio stays where it
        is on the timeline: trimming the start advances the source offset. */
    bool resizeClip (const juce::String& clipId, double startSeconds, double endSeconds);

    /** Cuts a clip in two at timeSeconds; see canSplitClip. The left part keeps
        the clip's ID (and its selection). */
    bool splitClip (const juce::String& clipId, double timeSeconds);

    /** Whether timeSeconds lies far enough inside the clip for a cut. */
    bool canSplitClip (const juce::String& clipId, double timeSeconds) const;

    /** Makes one of a clip's takes (0-based) the one it plays. */
    bool setClipTake (const juce::String& clipId, int takeIndex);

    //==============================================================================
    // Engine Undo (a selected clip stays selected if it survives)
    bool undo();
    bool redo();
    bool canUndo() const;
    bool canRedo() const;

    //==============================================================================
    // Selection (engine SelectionManager; never undoable)
    void selectTrack (const juce::String& trackId);
    void selectClip (const juce::String& clipId);
    juce::String getSelectedClipId() const;

    //==============================================================================
    // Transport (never undoable)
    void play();

    /** Stops; a recording in progress becomes clips on its tracks, as one undo step. */
    void stop();

    /** Plays and records every armed track's input. While looping, each pass
        through the loop becomes a take of one clip. Fails, doing nothing, if no
        track is armed or the loop is shorter than minLoopRecordingSeconds. */
    juce::Result record();

    /** The engine won't loop-record a shorter loop. */
    static constexpr double minLoopRecordingSeconds = 2.0;

    /** Ends a recording in progress first, as stop() does. */
    void returnToStart();
    bool isPlaying() const;
    bool isRecording() const;
    double getTransportPositionSeconds() const;

    /** The loop is fixed while recording: these change nothing then. */
    void setLooping (bool);
    bool isLooping() const;

    /** Sets the loop; returns false for an empty range. */
    bool setLoopRange (double startSeconds, double endSeconds);
    TimeRangeSeconds getLoopRange() const;

    //==============================================================================
    // Queries
    std::vector<TrackInfo> getTracks() const;

    /** Creates a background-generated waveform for the clip; repaintTarget is
        repainted as data arrives. Returns nullptr for an unknown clip. */
    std::unique_ptr<ClipWaveform> createWaveform (const juce::String& clipId,
                                                  juce::Component& repaintTarget) const;

    /** The recordings in progress. Poll it: it changes with every audio block
        and sends no modelChanged(). */
    std::vector<RecordingInfo> getRecordings() const;

    /** The waveform of a track's recording in progress, filled in as audio
        arrives; repaint to see it.
        Returns nullptr if the track isn't recording. */
    std::unique_ptr<ClipWaveform> createRecordingWaveform (const juce::String& trackId) const;

    void addListener (Listener*);
    void removeListener (Listener*);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;

    JUCE_DECLARE_NON_COPYABLE (ApplicationModel)
};

} // namespace papercut
