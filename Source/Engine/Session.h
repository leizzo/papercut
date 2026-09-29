#pragma once

#include <juce_core/juce_core.h>
#include <vector>

namespace resamper
{

class ProjectManager;

/** One clip slot in a scene row. A vacant slot has an empty clipId and hasClip false. */
struct SlotInfo
{
    int sceneIndex = 0;
    juce::String clipId;     ///< empty if vacant
    juce::String name;
    bool hasClip = false;
    bool queued = false;     ///< LaunchHandle queue is playQueued
    bool playing = false;    ///< LaunchHandle play state, if the graph has advanced it
};

/** One scene: a row of clip slots across tracks. */
struct SceneInfo
{
    int index = 0;
    juce::String name;
    int occupiedSlots = 0;
};

/** Facade over the Edit's scenes and clip slots.

    State lives on the Edit. Every call re-reads ProjectManager::getEdit().
    Undo is Engine Undo: setSceneCount, renameScene, addSlotClip and clearSlot
    each begin one transaction when they change something. Launch and stop are
    transport-like and never begin a transaction. recordIntoArrangement is one
    undo step.

    Headless tests have no audio device. launchSlot only queues playback;
    playing becomes true if an audio thread advances the LaunchHandle.
*/
class Session
{
public:
    explicit Session (ProjectManager&);

    /** Ensures at least count scenes, and the same number of slots on existing audio tracks.
        Does nothing when the Edit already has that many. Does not shrink. */
    juce::Result setSceneCount (int count);

    std::vector<SceneInfo> getScenes() const;

    /** Scene names live on the engine Scene. Returns false if the index is unknown or the name is unchanged. */
    bool renameScene (int index, const juce::String& name);

    /** Puts a WAV in an audio track's slot for the scene.
        Fails if the track is MIDI, the scene does not exist, or the file is missing.
        The clip is parented to the ClipSlot, so it is not an Arrangement clip. */
    juce::Result addSlotClip (const juce::String& trackId, int sceneIndex, const juce::File& audioFile);

    /** Puts an empty one-bar MIDI clip in a MIDI track's slot.
        Fails if the track is not MIDI or the scene does not exist.
        The clip is parented to the ClipSlot, so it is not an Arrangement clip. */
    juce::Result addMidiSlotClip (const juce::String& trackId, int sceneIndex);

    /** Removes the slot's clip. Returns false if the slot was already vacant or unknown. */
    bool clearSlot (const juce::String& trackId, int sceneIndex);

    std::vector<SlotInfo> getSlots (const juce::String& trackId) const;

    /** Queues the slot to play, and sets the track to play slot clips. Not undoable.
        Returns false if the slot is vacant. */
    bool launchSlot (const juce::String& trackId, int sceneIndex);

    /** Clears a queued launch, or queues a stop if the slot is already playing. Not undoable.
        Returns false if the slot was neither queued nor playing. */
    bool stopSlot (const juce::String& trackId, int sceneIndex);

    /** Queues every occupied slot in that row. An empty scene returns false and changes nothing.
        Not undoable. Does not launch or stop any other row. */
    bool launchScene (int sceneIndex);

    /** Stops every queued or playing slot. Not undoable. Returns false if nothing was running. */
    bool stopAll();

    /** For each slot clip that is playing, inserts an Arrangement clip of the same
        kind at the playhead. A queued slot is left alone. Audio keeps the slot's
        source file and offset; it is not copied. MIDI notes are copied onto a new
        MIDI clip. Length is the slot clip's length, clamped to the Edit.
        One undo step. */
    juce::Result recordIntoArrangement();

private:
    ProjectManager& projects;

    JUCE_DECLARE_NON_COPYABLE (Session)
};

} // namespace resamper
