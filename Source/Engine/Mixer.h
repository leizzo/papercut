#pragma once

#include <juce_core/juce_core.h>
#include <vector>

namespace papercut
{

class ProjectManager;

/** One aux send on a track. gainDb uses the track fader range; -100 is silence. */
struct SendInfo
{
    juce::String id;
    int bus = 0;
    double gainDb = 0;
    bool muted = false;
};

/** An audio track that carries an aux return. bus matches that return. */
struct ReturnInfo
{
    juce::String trackId;
    int bus = 0;
    juce::String name;
};

/** A submix folder. childTrackIds are its direct children. */
struct BusInfo
{
    juce::String trackId;
    juce::String name;
    std::vector<juce::String> childTrackIds;
};

/** The Edit's master fader, not any track in ApplicationModel::getTracks(). */
struct MasterInfo
{
    double volumeDb = 0;
    double pan = 0;
};

/** A plug-in on a track's insert chain, ahead of that track's volume plug-in. */
struct InsertSummary
{
    juce::String id, name;
};

/** Facade over the current Edit's returns, sends, submix buses and master fader
    (ADR-0001, ADR-0012). Owns none of that state. Re-reads ProjectManager::getEdit()
    on every call. Nothing above this layer sees a Tracktion header.

    Undoable (Engine Undo): adding a return, a send or a bus, moving a track into
    a bus, send gain, and master volume and pan. A continued fader drag
    (continuesGesture) joins the previous step when that step is still the same
    gesture. A call that changes nothing returns false and opens no undo step.

    Send mute follows AuxSendPlugin, which writes the send gain through the
    UndoManager, so mute is one undo step — unlike track mute, which the engine
    keeps out of undo.
*/
class Mixer
{
public:
    explicit Mixer (ProjectManager&);

    /** An audio track with an aux return on the next free bus number. */
    juce::Result addReturn (const juce::String& name);

    std::vector<ReturnInfo> getReturns() const;

    /** An aux send on fromTrackId aimed at a return's bus. Fails, changing
        nothing, when the track or the bus's return is missing. */
    juce::Result addSend (const juce::String& fromTrackId, int bus);

    /** Clamped to ApplicationModel::minVolumeDb .. maxVolumeDb. continuesGesture
        as for ApplicationModel::setTrackVolume. */
    bool setSendGain (const juce::String& trackId, const juce::String& sendId, double gainDb, bool continuesGesture = false);

    /** One undo step when it changes something. See the class note. */
    bool setSendMuted (const juce::String& trackId, const juce::String& sendId, bool muted);

    std::vector<SendInfo> getSends (const juce::String& trackId) const;

    /** A submix folder track. */
    juce::Result addBus (const juce::String& name);

    /** Nests an existing audio or MIDI track inside the bus folder. */
    bool moveTrackToBus (const juce::String& trackId, const juce::String& busTrackId);

    std::vector<BusInfo> getBuses() const;

    MasterInfo getMaster() const;

    /** Master fader, not a track fader. continuesGesture as for setSendGain. */
    bool setMasterVolume (double db, bool continuesGesture = false);
    bool setMasterPan (double pan, bool continuesGesture = false);

    /** Inserts ahead of the track's volume plug-in. The volume plug-in, the
        level meter after it, and aux send/return routing are not inserts. */
    std::vector<InsertSummary> getInserts (const juce::String& trackId) const;

private:
    ProjectManager& projects;
    juce::String openGestureKey;

    void beginUndoStep (const juce::String& name);
    void beginGestureStep (const juce::String& name, const juce::String& gestureKey, bool continues);

    JUCE_DECLARE_NON_COPYABLE (Mixer)
};

} // namespace papercut
