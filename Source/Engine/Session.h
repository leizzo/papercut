#pragma once

#include <juce_core/juce_core.h>
#include <vector>

namespace papercut
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

/** Facade over the Edit's scenes and clip slots (ADR-0001, ADR-0012).

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

    /** Puts a WAV in that track's slot for the scene.
        Fails if the track is MIDI (papercutKind == "midi"), the scene does not exist, or the file is missing.
        The clip is parented to the ClipSlot, so it is not an Arrangement clip. */
    juce::Result addSlotClip (const juce::String& trackId, int sceneIndex, const juce::File& audioFile);

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

    /** For each playing or queued slot clip, inserts an Arrangement clip of the same audio
        at the playhead. Length is the source length, clamped to the source and to the Edit.
        One undo step. Does not need the audio thread: the slot's audio is copied onto the Arrangement. */
    juce::Result recordIntoArrangement();

private:
    ProjectManager& projects;

    JUCE_DECLARE_NON_COPYABLE (Session)
};

} // namespace papercut
