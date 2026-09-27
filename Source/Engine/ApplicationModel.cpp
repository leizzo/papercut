#include "ApplicationModel.h"
#include "ClipWaveformImpl.h"
#include "ProjectManager.h"

#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion;

namespace papercut
{

struct ApplicationModel::Impl : private juce::ValueTree::Listener,
                                private juce::ChangeListener,
                                private juce::AsyncUpdater
{
    explicit Impl (ProjectManager& pm) : projectManager (pm)
    {
        selectionManager.addChangeListener (this);
        attach();
    }

    ~Impl() override
    {
        detach();
        selectionManager.removeChangeListener (this);
    }

    te::Edit& edit() const          { return projectManager.getEdit(); }
    juce::UndoManager& undoManager() { return edit().getUndoManager(); }

    //==============================================================================
    /** Must wrap every replacement of the current Edit. */
    template <typename Fn>
    auto replacingEdit (Fn&& fn)
    {
        detach();
        auto result = fn();
        attach();
        openGestureKey = {};
        notifyChanged();
        return result;
    }

    void attach()
    {
        selectionManager.edit = &edit();
        editState = edit().state;
        editState.addListener (this);
    }

    void detach()
    {
        editState.removeListener (this);
        editState = {};
        selectionManager.deselectAll();
        selectionManager.edit = nullptr;
    }

    //==============================================================================
    /** Starts an Engine Undo step. Every model mutation goes through here. */
    void beginUndoStep (const juce::String& name)
    {
        openGestureKey = {};
        undoManager().beginNewTransaction (name);
    }

    /** Starts an undo step for one call of a continuous gesture (a fader drag),
        or joins the previous call's step if that was the same gesture, keyed
        e.g. by parameter and track, with nothing undoable since. */
    void beginGestureStep (const juce::String& name, const juce::String& gestureKey, bool continues)
    {
        if (! continues || openGestureKey != gestureKey)
            beginUndoStep (name);

        openGestureKey = gestureKey;
    }

    /** Changes a track's volume/pan through the engine's parameter, which records
        the change in the Edit's UndoManager (consecutive writes within one undo
        step coalesce). Returns whether the value changed. */
    template <typename Get, typename Set>
    bool changeVolumePlugin (const juce::String& trackId, const juce::String& stepName, bool continues, Get get, Set set)
    {
        auto* track = findTrack (trackId);
        auto* plugin = track != nullptr ? track->getVolumePlugin() : nullptr;

        if (plugin == nullptr)
            return false;

        // Undoing the first change of a default value would remove the property,
        // leaving nothing for syncVolumeParametersFromState to compare against.
        for (auto* value : { &plugin->volume, &plugin->pan })
            if (value->isUsingDefault())
                plugin->state.setProperty (value->getPropertyID(), value->get(), nullptr);

        // An undo step that ends up empty records nothing.
        const auto before = get (*plugin);
        beginGestureStep (stepName, stepName + ":" + trackId, continues);
        set (*plugin);
        return get (*plugin) != before;
    }

    /** Undo and redo restore plugin state, but a parameter never re-reads its
        state by itself (the engine expects changes through the parameter). Only
        stale parameters are touched: re-reading writes the state back, and a
        write after an undo would clear the redo history. */
    void syncVolumeParametersFromState()
    {
        for (auto* t : te::getAudioTracks (edit()))
            if (auto* plugin = t->getVolumePlugin())
                for (auto [parameter, value] : { std::pair (plugin->volParam.get(), &plugin->volume),
                                                 std::pair (plugin->panParam.get(), &plugin->pan) })
                    if (parameter->getCurrentValue() != value->get())
                        parameter->updateFromAttachedValue();
    }

    //==============================================================================
    te::TransportControl& transport() const   { return edit().getTransport(); }

    /** The Edit's audio inputs. Allocates the playback context, which owns them. */
    juce::Array<te::InputDeviceInstance*> audioInputs() const
    {
        transport().ensureContextAllocated();
        juce::Array<te::InputDeviceInstance*> inputs;

        if (auto* context = edit().getCurrentPlaybackContext())
            for (auto* input : context->getAllInputs())
                if (input->getInputDevice().getDeviceType() == te::InputDevice::waveDevice)
                    inputs.add (input);

        return inputs;
    }

    te::InputDeviceInstance* findInput (const juce::String& name) const
    {
        for (auto* input : audioInputs())
            if (input->getInputDevice().getName() == name)
                return input;

        return nullptr;
    }

    /** The input a track records from (a track has at most one). */
    te::InputDeviceInstance* inputOf (const te::AudioTrack& track) const
    {
        return inputOf (track, audioInputs());
    }

    te::InputDeviceInstance* inputOf (const te::AudioTrack& track, const juce::Array<te::InputDeviceInstance*>& inputs) const
    {
        for (auto* input : inputs)
            if (input->getTargets().contains (track.itemID))
                return input;

        return nullptr;
    }

    /** A take's audio file; takes store their source as a clip does. */
    juce::File takeFile (juce::ValueTree take) const
    {
        return te::SourceFileReference (edit(), take, te::IDs::source).getFile();
    }

    static juce::ValueTree takesOf (const te::Clip& clip)   { return clip.state.getChildWithName (te::IDs::TAKES); }

    int currentTakeOf (te::WaveAudioClip& clip) const
    {
        const auto takes = takesOf (clip);
        const auto playing = clip.getSourceFileReference().getFile();

        for (int i = 0; i < takes.getNumChildren(); ++i)
            if (takeFile (takes.getChild (i)) == playing)
                return i;

        return -1;
    }

    /** A loop recording lists its first pass twice among its takes (the engine
        adds the file both before and after cutting the later passes out of it). */
    void removeDuplicateTakes()
    {
        for (auto* t : te::getAudioTracks (edit()))
        {
            for (auto* c : t->getClips())
            {
                auto takes = takesOf (*c);
                juce::Array<juce::File> seen;

                for (int i = 0; i < takes.getNumChildren();)
                {
                    if (auto file = takeFile (takes.getChild (i)); seen.contains (file))
                    {
                        takes.removeChild (i, &undoManager());
                    }
                    else
                    {
                        seen.add (file);
                        ++i;
                    }
                }
            }
        }
    }

    //==============================================================================
    te::AudioTrack* findTrack (const juce::String& id) const
    {
        for (auto* t : te::getAudioTracks (edit()))
            if (t->itemID.toString() == id)
                return t;

        return nullptr;
    }

    te::WaveAudioClip* findClip (const juce::String& id) const
    {
        for (auto* t : te::getAudioTracks (edit()))
            for (auto* c : t->getClips())
                if (c->itemID.toString() == id)
                    return dynamic_cast<te::WaveAudioClip*> (c);

        return nullptr;
    }

    te::WaveAudioClip* selectedClip() const
    {
        auto selected = selectionManager.getItemsOfType<te::WaveAudioClip>();
        return selected.isEmpty() ? nullptr : selected.getFirst();
    }

    te::AudioTrack* selectedTrack() const
    {
        auto selected = selectionManager.getItemsOfType<te::AudioTrack>();
        return selected.isEmpty() ? nullptr : selected.getFirst();
    }

    /** Where a new clip goes: the selected track, or the selected clip's track. */
    te::AudioTrack* insertionTrack() const
    {
        if (auto* track = selectedTrack())
            return track;

        auto* clip = selectedClip();
        return clip != nullptr ? dynamic_cast<te::AudioTrack*> (clip->getTrack()) : nullptr;
    }

    /** Runs fn, then re-selects the clip that was selected before if fn replaced
        its object: re-parenting a clip, or undoing that, rebuilds it from its state. */
    template <typename Fn>
    auto keepingClipSelection (Fn&& fn)
    {
        auto* before = selectedClip();
        const auto id = before != nullptr ? before->itemID.toString() : juce::String();
        auto result = fn();

        if (id.isNotEmpty() && selectedClip() == nullptr)
            if (auto* clip = findClip (id))
                selectionManager.selectOnly (clip);

        return result;
    }

    //==============================================================================
    ProjectManager& projectManager;
    te::SelectionManager selectionManager { projectManager.getEdit().engine };
    juce::ValueTree editState;
    juce::String openGestureKey;   ///< the gesture whose undo step is still open, if any
    juce::ListenerList<ApplicationModel::Listener> listeners;

    void notifyChanged()    { triggerAsyncUpdate(); }

private:
    static bool isTransportState (const juce::ValueTree& v)
    {
        for (auto t = v; t.isValid(); t = t.getParent())
            if (t.hasType (te::IDs::TRANSPORT))
                return true;

        return false;
    }

    void valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier&) override
    {
        // Transport position/state churn during playback is not a model change.
        if (! isTransportState (v))
            triggerAsyncUpdate();
    }

    void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) override          { triggerAsyncUpdate(); }
    void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) override   { triggerAsyncUpdate(); }
    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override           { triggerAsyncUpdate(); }
    void changeListenerCallback (juce::ChangeBroadcaster*) override                 { triggerAsyncUpdate(); }

    void handleAsyncUpdate() override
    {
        listeners.call ([] (ApplicationModel::Listener& l) { l.modelChanged(); });
    }
};

//==============================================================================
ApplicationModel::ApplicationModel (ProjectManager& pm)
    : impl (std::make_unique<Impl> (pm))
{
}

ApplicationModel::~ApplicationModel() = default;

//==============================================================================
void ApplicationModel::newProject()
{
    impl->replacingEdit ([this] { impl->projectManager.newProject(); return true; });
}

juce::Result ApplicationModel::openProject (const juce::File& folder, juce::var& uiState)
{
    return impl->replacingEdit ([&] { return impl->projectManager.open (folder, uiState); });
}

juce::Result ApplicationModel::saveProject (const juce::var& uiState)
{
    return impl->projectManager.save (uiState);
}

juce::Result ApplicationModel::saveProjectAs (const juce::File& folder, const juce::var& uiState)
{
    auto r = impl->projectManager.saveAs (folder, uiState);
    impl->notifyChanged();   // the Project name changed
    return r;
}

bool ApplicationModel::isProjectUntitled() const        { return impl->projectManager.isUntitled(); }
juce::String ApplicationModel::getProjectName() const   { return impl->projectManager.getProjectName(); }

//==============================================================================
void ApplicationModel::addAudioTrack()
{
    auto& edit = impl->edit();
    impl->beginUndoStep ("Add Track");
    edit.insertNewAudioTrack (te::TrackInsertPoint::getEndOfTracks (edit), nullptr);
}

bool ApplicationModel::removeTrack()
{
    auto* track = impl->selectedTrack();

    if (track == nullptr)
        track = te::getAudioTracks (impl->edit()).getLast();

    if (track == nullptr)
        return false;

    impl->beginUndoStep ("Remove Track");
    impl->edit().deleteTrack (track);
    return true;
}

bool ApplicationModel::setTrackVolume (const juce::String& trackId, double db, bool continuesGesture)
{
    // The engine stores a fader position, not dB.
    const auto position = te::decibelsToVolumeFaderPosition ((float) juce::jlimit (minVolumeDb, maxVolumeDb, db));

    return impl->changeVolumePlugin (trackId, "Set Volume", continuesGesture,
                                     [] (te::VolumeAndPanPlugin& p) { return p.getSliderPos(); },
                                     [&] (te::VolumeAndPanPlugin& p) { p.setSliderPos (position); });
}

bool ApplicationModel::setTrackPan (const juce::String& trackId, double pan, bool continuesGesture)
{
    return impl->changeVolumePlugin (trackId, "Set Pan", continuesGesture,
                                     [] (te::VolumeAndPanPlugin& p) { return p.getPan(); },
                                     [&] (te::VolumeAndPanPlugin& p) { p.setPan ((float) juce::jlimit (-1.0, 1.0, pan)); });
}

bool ApplicationModel::setTrackMuted (const juce::String& trackId, bool muted)
{
    auto* track = impl->findTrack (trackId);

    if (track == nullptr || track->isMuted (false) == muted)
        return false;

    track->setMute (muted);
    return true;
}

bool ApplicationModel::setTrackSolo (const juce::String& trackId, bool solo)
{
    auto* track = impl->findTrack (trackId);

    if (track == nullptr || track->isSolo (false) == solo)
        return false;

    track->setSolo (solo);
    return true;
}

//==============================================================================
juce::StringArray ApplicationModel::getAudioInputs() const
{
    juce::StringArray names;

    for (auto* input : impl->audioInputs())
        names.add (input->getInputDevice().getName());

    return names;
}

bool ApplicationModel::setTrackInput (const juce::String& trackId, const juce::String& inputName)
{
    auto* track = impl->findTrack (trackId);
    auto* current = track != nullptr ? impl->inputOf (*track) : nullptr;
    auto* input = inputName.isEmpty() ? nullptr : impl->findInput (inputName);

    if (track == nullptr || input == current || (input == nullptr && inputName.isNotEmpty()))
        return false;

    // Inputs live outside the UndoManager, like mute and solo.
    const bool armed = current != nullptr && current->isRecordingEnabled (track->itemID);

    if (current != nullptr)
        (void) current->removeTarget (track->itemID, nullptr);

    if (input != nullptr)
        if (auto destination = input->setTarget (track->itemID, false, nullptr); destination.has_value())
            (*destination)->recordEnabled = armed;

    return true;
}

bool ApplicationModel::setTrackArmed (const juce::String& trackId, bool armed)
{
    auto* track = impl->findTrack (trackId);

    if (track == nullptr)
        return false;

    auto* input = impl->inputOf (*track);

    if (input == nullptr && armed)
        if (setTrackInput (trackId, getAudioInputs()[0]))
            input = impl->inputOf (*track);

    if (input == nullptr || input->isRecordingEnabled (track->itemID) == armed)
        return false;

    input->setRecordingEnabled (track->itemID, armed);
    return true;
}

juce::Result ApplicationModel::insertAudioClip (const juce::File& file)
{
    auto& edit = impl->edit();
    te::AudioFile audioFile (edit.engine, file);

    if (! audioFile.isValid())
        return juce::Result::fail ("Not a readable audio file: " + file.getFullPathName());

    impl->beginUndoStep ("Insert Clip");

    auto* track = impl->insertionTrack();

    if (track == nullptr)
    {
        edit.ensureNumberOfAudioTracks (1);
        track = te::getAudioTracks (edit).getFirst();
    }

    auto start = te::TimePosition();

    for (auto* c : track->getClips())
        start = std::max (start, c->getPosition().getEnd());

    auto clip = track->insertWaveClip (file.getFileNameWithoutExtension(), file,
                                       { { start, te::TimeDuration::fromSeconds (audioFile.getLength()) }, {} },
                                       false);

    if (clip == nullptr)
        return juce::Result::fail ("The engine refused the clip: " + file.getFullPathName());

    // Absolute, so a Save As into another folder cannot break the reference.
    clip->getSourceFileReference().setToFile (file, te::SourceFileReference::PathStyle::alwaysAbsolute, false);

    // Tempo-tagged loops (e.g. ACID WAVs) play from a time-stretched proxy. The
    // engine only starts rendering it when a playback graph is built, i.e. on
    // Play, and stops the transport when it lands. Start it now so the clip is
    // ready (waveform and audio) by the time the user presses Play.
    clip->beginRenderingNewProxyIfNeeded();
    return juce::Result::ok();
}

bool ApplicationModel::moveClip (const juce::String& clipId, double startSeconds, const juce::String& trackId)
{
    auto* clip = impl->findClip (clipId);
    auto* track = trackId.isEmpty() ? (clip != nullptr ? clip->getClipTrack() : nullptr)
                                    : impl->findTrack (trackId);

    if (clip == nullptr || track == nullptr)
        return false;

    const auto start = te::TimePosition::fromSeconds (std::max (0.0, startSeconds));
    const bool changesTrack = track != clip->getClipTrack();

    if (! changesTrack && start == clip->getPosition().getStart())
        return false;

    return impl->keepingClipSelection ([&]
    {
        impl->beginUndoStep ("Move Clip");

        if (changesTrack)
            clip->moveTo (*track);

        // Re-parenting may rebuild the clip object; look it up again by ID.
        if (auto* moved = impl->findClip (clipId))
            moved->setStart (start, false, true);

        return true;
    });
}

bool ApplicationModel::resizeClip (const juce::String& clipId, double startSeconds, double endSeconds)
{
    auto* clip = impl->findClip (clipId);

    if (clip == nullptr)
        return false;

    const auto pos = clip->getPosition();
    const auto sourceStart = pos.getStart() - pos.getOffset();
    const auto sourceEnd = sourceStart + clip->getMaximumLength();

    const auto start = std::max ({ te::TimePosition::fromSeconds (startSeconds), sourceStart, te::TimePosition() });
    const auto end = std::min (te::TimePosition::fromSeconds (endSeconds), sourceEnd);

    if (end <= start || (start == pos.getStart() && end == pos.getEnd()))
        return false;

    impl->beginUndoStep ("Resize Clip");
    clip->setPosition ({ { start, end }, pos.getOffset() + (start - pos.getStart()) });
    return true;
}

bool ApplicationModel::canSplitClip (const juce::String& clipId, double timeSeconds) const
{
    // The engine won't cut within a millisecond of either edge.
    auto* clip = impl->findClip (clipId);
    return clip != nullptr && clip->getPosition().time.reduced (te::TimeDuration::fromSeconds (0.001)).contains (te::TimePosition::fromSeconds (timeSeconds));
}

bool ApplicationModel::splitClip (const juce::String& clipId, double timeSeconds)
{
    if (! canSplitClip (clipId, timeSeconds))
        return false;

    return impl->keepingClipSelection ([&]
    {
        impl->beginUndoStep ("Split Clip");
        auto* clip = impl->findClip (clipId);
        return clip->getClipTrack()->splitClip (*clip, te::TimePosition::fromSeconds (timeSeconds)) != nullptr;
    });
}

bool ApplicationModel::setClipTake (const juce::String& clipId, int takeIndex)
{
    // Not WaveAudioClip::setCurrentTake: that only knows takes that are items of a
    // te::Project, and deletes the file takes a loop recording makes here.
    auto* clip = impl->findClip (clipId);

    if (clip == nullptr || takeIndex == impl->currentTakeOf (*clip))
        return false;

    const auto take = Impl::takesOf (*clip).getChild (takeIndex);

    if (! take.isValid())
        return false;

    impl->beginUndoStep ("Switch Take");
    clip->state.setProperty (te::IDs::source, take[te::IDs::source], &impl->undoManager());
    return true;
}

//==============================================================================
bool ApplicationModel::undo()
{
    impl->openGestureKey = {};
    const auto undone = impl->keepingClipSelection ([this] { return impl->undoManager().undo(); });
    impl->syncVolumeParametersFromState();
    return undone;
}

bool ApplicationModel::redo()
{
    impl->openGestureKey = {};
    const auto redone = impl->keepingClipSelection ([this] { return impl->undoManager().redo(); });
    impl->syncVolumeParametersFromState();
    return redone;
}

bool ApplicationModel::canUndo() const   { return impl->undoManager().canUndo(); }
bool ApplicationModel::canRedo() const   { return impl->undoManager().canRedo(); }

//==============================================================================
void ApplicationModel::selectTrack (const juce::String& trackId)
{
    if (auto* track = impl->findTrack (trackId))
        impl->selectionManager.selectOnly (track);
    else
        impl->selectionManager.deselectAll();
}

void ApplicationModel::selectClip (const juce::String& clipId)
{
    if (auto* clip = impl->findClip (clipId))
        impl->selectionManager.selectOnly (clip);
    else
        impl->selectionManager.deselectAll();
}

juce::String ApplicationModel::getSelectedClipId() const
{
    auto* clip = impl->selectedClip();
    return clip != nullptr ? clip->itemID.toString() : juce::String();
}

//==============================================================================
void ApplicationModel::play()
{
    impl->edit().getTransport().play (false);
}

void ApplicationModel::stop()
{
    if (! isRecording())
    {
        impl->transport().stop (false, false);
        return;
    }

    // Stopping turns the recording into clips.
    impl->beginUndoStep ("Record");
    impl->transport().stop (false, false);
    impl->removeDuplicateTakes();
}

juce::Result ApplicationModel::record()
{
    // The engine checks these too, but only tells its UIBehaviour.
    auto tracks = getTracks();

    if (std::none_of (tracks.begin(), tracks.end(), [] (const TrackInfo& t) { return t.armed; }))
        return juce::Result::fail ("Arm a track to record");

    const auto loop = getLoopRange();

    if (isLooping() && loop.end - loop.start < minLoopRecordingSeconds)
        return juce::Result::fail ("To record in a loop, make the loop at least "
                                   + juce::String (minLoopRecordingSeconds) + " seconds long");

    if (! isRecording())
        impl->transport().record (false);

    return juce::Result::ok();
}

void ApplicationModel::returnToStart()
{
    // A recording ends through stop(), which makes it one undo step.
    if (isRecording())
        stop();

    impl->edit().getTransport().setPosition (te::TimePosition());
}

bool ApplicationModel::isPlaying() const
{
    return impl->edit().getTransport().isPlaying();
}

bool ApplicationModel::isRecording() const
{
    return impl->edit().getTransport().isRecording();
}

// The loop lives in transport state, which sends no modelChanged() by itself.
// Fixed while recording: the engine cuts a loop recording into takes by the
// loop it finds when the recording stops.
void ApplicationModel::setLooping (bool shouldLoop)
{
    if (isRecording())
        return;

    impl->transport().looping = shouldLoop;
    impl->notifyChanged();
}

bool ApplicationModel::isLooping() const
{
    return impl->transport().looping;
}

bool ApplicationModel::setLoopRange (double startSeconds, double endSeconds)
{
    const auto start = std::max (0.0, startSeconds);

    if (endSeconds <= start || isRecording())
        return false;

    impl->transport().setLoopRange ({ te::TimePosition::fromSeconds (start), te::TimePosition::fromSeconds (endSeconds) });
    impl->notifyChanged();
    return true;
}

TimeRangeSeconds ApplicationModel::getLoopRange() const
{
    const auto range = impl->transport().getLoopRange();
    return { range.getStart().inSeconds(), range.getEnd().inSeconds() };
}

double ApplicationModel::getTransportPositionSeconds() const
{
    return impl->edit().getTransport().getPosition().inSeconds();
}

//==============================================================================
std::vector<TrackInfo> ApplicationModel::getTracks() const
{
    std::vector<TrackInfo> tracks;
    const auto inputs = impl->audioInputs();

    for (auto* t : te::getAudioTracks (impl->edit()))
    {
        TrackInfo info;
        info.id = t->itemID.toString();
        info.name = t->getName();
        info.selected = impl->selectionManager.isSelected (t);
        info.muted = t->isMuted (false);
        info.solo = t->isSolo (false);

        if (auto* input = impl->inputOf (*t, inputs))
        {
            info.input = input->getInputDevice().getName();
            info.armed = input->isRecordingEnabled (t->itemID);
        }

        if (auto* volume = t->getVolumePlugin())
        {
            info.volumeDb = juce::jmax (minVolumeDb, (double) volume->getVolumeDb());
            info.pan = volume->getPan();
        }

        for (auto* c : t->getClips())
        {
            if (auto* wave = dynamic_cast<te::WaveAudioClip*> (c))
            {
                const auto pos = wave->getPosition();
                info.clips.push_back ({ wave->itemID.toString(),
                                        wave->getName(),
                                        pos.getStart().inSeconds(),
                                        pos.getLength().inSeconds(),
                                        pos.getOffset().inSeconds(),
                                        wave->getMaximumLength().inSeconds(),
                                        wave->getOriginalFile(),
                                        impl->selectionManager.isSelected (wave),
                                        Impl::takesOf (*wave).getNumChildren(),
                                        impl->currentTakeOf (*wave) });
            }
        }

        tracks.push_back (std::move (info));
    }

    return tracks;
}

std::unique_ptr<ClipWaveform> ApplicationModel::createWaveform (const juce::String& clipId,
                                                                juce::Component& repaintTarget) const
{
    if (auto* clip = impl->findClip (clipId))
        return std::make_unique<ClipWaveform> (
            std::make_unique<ClipWaveform::Impl> (clip->edit.engine, clip->getPlaybackFile().getFile(), repaintTarget));

    return nullptr;
}

std::vector<RecordingInfo> ApplicationModel::getRecordings() const
{
    std::vector<RecordingInfo> recordings;

    if (! isRecording())
        return recordings;

    // Unlooped: while loop recording, the recording runs on through every pass.
    auto* context = impl->edit().getCurrentPlaybackContext();

    if (context == nullptr)
        return recordings;

    const auto now = context->getUnloopedPosition().inSeconds();
    const auto loop = getLoopRange();

    for (auto* t : te::getAudioTracks (impl->edit()))
    {
        if (auto* input = impl->inputOf (*t); input != nullptr && input->isRecording (t->itemID))
        {
            const auto start = input->getPunchInTime (t->itemID).inSeconds();
            auto length = now - start;

            // A loop recording's later passes become takes over the same range.
            if (isLooping())
                length = std::min (length, loop.end - start);

            recordings.push_back ({ t->itemID.toString(), start, std::max (0.0, length) });
        }
    }

    return recordings;
}

std::unique_ptr<ClipWaveform> ApplicationModel::createRecordingWaveform (const juce::String& trackId) const
{
    auto* track = impl->findTrack (trackId);
    auto* input = track != nullptr ? impl->inputOf (*track) : nullptr;

    if (input == nullptr || ! input->isRecording (track->itemID))
        return nullptr;

    auto thumbnail = impl->edit().engine.getRecordingThumbnailManager().getThumbnailFor (input->getRecordingFile (track->itemID));
    return std::make_unique<ClipWaveform> (std::make_unique<ClipWaveform::Impl> (std::move (thumbnail)));
}

void ApplicationModel::addListener (Listener* l)      { impl->listeners.add (l); }
void ApplicationModel::removeListener (Listener* l)   { impl->listeners.remove (l); }

} // namespace papercut
