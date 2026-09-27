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
    juce::File file;
};

/** Read-only snapshot of an audio track, for views. */
struct TrackInfo
{
    juce::String id;
    juce::String name;
    bool selected = false;
    std::vector<ClipInfo> clips;
};

/** The facade over the current Edit (ADR-0001).

    Exposes app-level operations and read-only snapshots; owns no track or clip
    state of its own. Nothing above this layer sees a Tracktion header.

    Undoable (Engine Undo): adding/removing tracks and inserting clips — one undo
    step per call. Never undoable: transport, selection.
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

    /** Inserts the file as a clip at the end of the selected track (or the first
        track, creating one if the Edit has none). */
    juce::Result insertAudioClip (const juce::File&);

    //==============================================================================
    // Engine Undo
    bool undo();
    bool redo();
    bool canUndo() const;
    bool canRedo() const;

    //==============================================================================
    // Selection (engine SelectionManager; never undoable)
    void selectTrack (const juce::String& trackId);

    //==============================================================================
    // Transport (never undoable)
    void play();
    void stop();
    void returnToStart();
    bool isPlaying() const;
    double getTransportPositionSeconds() const;

    //==============================================================================
    // Queries
    std::vector<TrackInfo> getTracks() const;

    /** Creates a background-generated waveform for the clip; repaintTarget is
        repainted as data arrives. Returns nullptr for an unknown clip. */
    std::unique_ptr<ClipWaveform> createWaveform (const juce::String& clipId,
                                                  juce::Component& repaintTarget) const;

    void addListener (Listener*);
    void removeListener (Listener*);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;

    JUCE_DECLARE_NON_COPYABLE (ApplicationModel)
};

} // namespace papercut
