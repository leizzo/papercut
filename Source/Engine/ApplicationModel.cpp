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
    impl->undoManager().beginNewTransaction ("Add Track");
    edit.insertNewAudioTrack (te::TrackInsertPoint::getEndOfTracks (edit), nullptr);
}

bool ApplicationModel::removeTrack()
{
    auto* track = impl->selectedTrack();

    if (track == nullptr)
        track = te::getAudioTracks (impl->edit()).getLast();

    if (track == nullptr)
        return false;

    impl->undoManager().beginNewTransaction ("Remove Track");
    impl->edit().deleteTrack (track);
    return true;
}

juce::Result ApplicationModel::insertAudioClip (const juce::File& file)
{
    auto& edit = impl->edit();
    te::AudioFile audioFile (edit.engine, file);

    if (! audioFile.isValid())
        return juce::Result::fail ("Not a readable audio file: " + file.getFullPathName());

    impl->undoManager().beginNewTransaction ("Insert Clip");

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
        impl->undoManager().beginNewTransaction ("Move Clip");

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

    impl->undoManager().beginNewTransaction ("Resize Clip");
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
        impl->undoManager().beginNewTransaction ("Split Clip");
        auto* clip = impl->findClip (clipId);
        return clip->getClipTrack()->splitClip (*clip, te::TimePosition::fromSeconds (timeSeconds)) != nullptr;
    });
}

//==============================================================================
bool ApplicationModel::undo()
{
    return impl->keepingClipSelection ([this] { return impl->undoManager().undo(); });
}

bool ApplicationModel::redo()
{
    return impl->keepingClipSelection ([this] { return impl->undoManager().redo(); });
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
    impl->edit().getTransport().stop (false, false);
}

void ApplicationModel::returnToStart()
{
    impl->edit().getTransport().setPosition (te::TimePosition());
}

bool ApplicationModel::isPlaying() const
{
    return impl->edit().getTransport().isPlaying();
}

double ApplicationModel::getTransportPositionSeconds() const
{
    return impl->edit().getTransport().getPosition().inSeconds();
}

//==============================================================================
std::vector<TrackInfo> ApplicationModel::getTracks() const
{
    std::vector<TrackInfo> tracks;
    for (auto* t : te::getAudioTracks (impl->edit()))
    {
        TrackInfo info { t->itemID.toString(), t->getName(), impl->selectionManager.isSelected (t), {} };

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
                                        impl->selectionManager.isSelected (wave) });
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

void ApplicationModel::addListener (Listener* l)      { impl->listeners.add (l); }
void ApplicationModel::removeListener (Listener* l)   { impl->listeners.remove (l); }

} // namespace papercut
