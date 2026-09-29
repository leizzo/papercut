#include "Session.h"
#include "ProjectManager.h"
#include "EditTracks.h"

#include <tracktion_engine/tracktion_engine.h>

#include <algorithm>
#include <optional>

namespace te = tracktion;

namespace resamper
{

namespace
{
    /** The slot list the track already has. Does not create the CLIPSLOTS node:
        creating it through the UndoManager while the track caches the list is
        how an undo step corrupts the Edit (see ProjectManager's getSceneList note). */
    te::ClipSlotList* existingSlots (te::AudioTrack& track)
    {
        if (! track.state.getChildWithName (te::IDs::CLIPSLOTS).isValid())
            return nullptr;

        return &track.getClipSlotList();
    }

    te::ClipSlot* slotAt (te::AudioTrack& track, int sceneIndex)
    {
        auto* slots = existingSlots (track);

        if (slots == nullptr || ! juce::isPositiveAndBelow (sceneIndex, slots->getClipSlots().size()))
            return nullptr;

        return slots->getClipSlots()[sceneIndex];
    }

    bool tracksHaveSlotCount (te::Edit& edit, int count)
    {
        for (auto* track : te::getAudioTracks (edit))
        {
            auto slots = track->state.getChildWithName (te::IDs::CLIPSLOTS);

            if (slots.getNumChildren() < count)
                return false;
        }

        return true;
    }

    bool isPlayQueued (te::Clip& clip)
    {
        if (auto handle = clip.getLaunchHandle())
            if (auto queued = handle->getQueuedStatus())
                return *queued == te::LaunchHandle::QueueState::playQueued;

        return false;
    }

    bool isSlotPlaying (te::Clip& clip)
    {
        if (auto handle = clip.getLaunchHandle())
            return handle->getPlayingStatus() == te::LaunchHandle::PlayState::playing;

        return false;
    }

    void fillSlot (SlotInfo& info, te::Clip& clip)
    {
        info.hasClip = true;
        info.clipId = clip.itemID.toString();
        info.name = clip.getName();
        info.queued = isPlayQueued (clip);
        info.playing = isSlotPlaying (clip);
    }

    /** The engine silences Arrangement clips on a track while it is set to play its slots. */
    void updatePlaySlotClips (te::AudioTrack& track)
    {
        auto* slots = existingSlots (track);
        bool active = false;

        if (slots != nullptr)
            for (auto* slot : slots->getClipSlots())
                if (slot != nullptr)
                    if (auto* clip = slot->getClip())
                        if (isPlayQueued (*clip) || isSlotPlaying (*clip))
                            active = true;

        track.playSlotClips = active;
    }

    te::TimePosition oneBarAfter (te::Edit& edit, te::TimePosition start)
    {
        auto& tempo = edit.tempoSequence;
        auto bars = tempo.toBarsAndBeats (start);
        ++bars.bars;
        return tempo.toTime (bars);
    }

    struct CapturedSlot
    {
        te::AudioTrack* track = nullptr;
        te::WaveAudioClip* wave = nullptr;
        te::MidiClip* midi = nullptr;
        double lengthSeconds = 0;
        double offsetSeconds = 0;
    };
}

Session::Session (ProjectManager& pm)
    : projects (pm)
{
}

juce::Result Session::setSceneCount (int count)
{
    if (count < 0)
        return juce::Result::fail ("Scene count cannot be negative");

    auto& edit = projects.getEdit();
    auto& scenes = edit.getSceneList();

    // SCENES itself was created with the Project, outside any user undo step.
    // Only the scene (and slot) children added here are undone.
    if (scenes.getNumScenes() >= count && tracksHaveSlotCount (edit, scenes.getNumScenes()))
        return juce::Result::ok();

    projects.getUndo().beginStep ("Set Scenes");
    scenes.ensureNumberOfScenes (std::max (count, scenes.getNumScenes()));
    return juce::Result::ok();
}

std::vector<SceneInfo> Session::getScenes() const
{
    auto& edit = projects.getEdit();
    std::vector<SceneInfo> result;
    auto scenes = edit.getSceneList().getScenes();
    result.reserve ((size_t) scenes.size());

    for (int index = 0; index < scenes.size(); ++index)
    {
        SceneInfo info;
        info.index = index;
        info.name = scenes[index]->name.get();

        for (auto* track : te::getAudioTracks (edit))
            if (auto* slot = slotAt (*track, index))
                if (slot->getClip() != nullptr)
                    ++info.occupiedSlots;

        result.push_back (std::move (info));
    }

    return result;
}

bool Session::renameScene (int index, const juce::String& name)
{
    auto& edit = projects.getEdit();
    auto scenes = edit.getSceneList().getScenes();

    if (! juce::isPositiveAndBelow (index, scenes.size()) || scenes[index]->name.get() == name)
        return false;

    projects.getUndo().beginStep ("Rename Scene");
    scenes[index]->name = name;
    return true;
}

juce::Result Session::addSlotClip (const juce::String& trackId, int sceneIndex, const juce::File& audioFile)
{
    auto& edit = projects.getEdit();
    auto* track = findAudioTrack (edit, trackId);

    if (track == nullptr)
        return juce::Result::fail ("Unknown track");

    if (trackKindOf (*track) == TrackKind::midi)
        return juce::Result::fail ("Slot clips go on audio tracks");

    te::AudioFile audio (edit.engine, audioFile);

    if (! audio.isValid())
        return juce::Result::fail ("Not a readable audio file: " + audioFile.getFullPathName());

    if (! juce::isPositiveAndBelow (sceneIndex, edit.getSceneList().getNumScenes()))
        return juce::Result::fail ("No such scene");

    const auto length = audio.getLength();

    if (length <= 0.0)
        return juce::Result::fail ("Not a readable audio file: " + audioFile.getFullPathName());

    // The track's slot list was created with the track. Don't create that node here:
    // undoing it would drop the list the track still caches.
    auto* slots = existingSlots (*track);

    if (slots == nullptr)
        return juce::Result::fail ("No slot for that scene");

    projects.getUndo().beginStep ("Add Slot Clip");
    slots->ensureNumberOfSlots (sceneIndex + 1);

    auto* slot = slotAt (*track, sceneIndex);

    if (slot == nullptr)
        return juce::Result::fail ("No slot for that scene");

    // Parent is the ClipSlot, not the track, so getClips() on the track stays empty.
    auto clip = te::insertWaveClip (*slot, audioFile.getFileNameWithoutExtension(), audioFile,
                                    { { te::TimePosition(), te::TimeDuration::fromSeconds (length) }, {} },
                                    te::DeleteExistingClips::yes);

    if (clip == nullptr)
        return juce::Result::fail ("The engine refused the clip: " + audioFile.getFullPathName());

    clip->getSourceFileReference().setToFile (audioFile, te::SourceFileReference::PathStyle::alwaysAbsolute, false);
    return juce::Result::ok();
}

juce::Result Session::addMidiSlotClip (const juce::String& trackId, int sceneIndex)
{
    auto& edit = projects.getEdit();
    auto* track = findAudioTrack (edit, trackId);

    if (track == nullptr)
        return juce::Result::fail ("Unknown track");

    if (trackKindOf (*track) != TrackKind::midi)
        return juce::Result::fail ("MIDI slot clips go on MIDI tracks");

    if (! juce::isPositiveAndBelow (sceneIndex, edit.getSceneList().getNumScenes()))
        return juce::Result::fail ("No such scene");

    auto* slots = existingSlots (*track);

    if (slots == nullptr)
        return juce::Result::fail ("No slot for that scene");

    projects.getUndo().beginStep ("Add Slot Clip");
    slots->ensureNumberOfSlots (sceneIndex + 1);

    auto* slot = slotAt (*track, sceneIndex);

    if (slot == nullptr)
        return juce::Result::fail ("No slot for that scene");

    const auto end = oneBarAfter (edit, te::TimePosition());
    auto clip = te::insertMIDIClip (*slot, "MIDI Clip", { te::TimePosition(), end });

    if (clip == nullptr)
        return juce::Result::fail ("The engine refused the MIDI clip");

    return juce::Result::ok();
}

bool Session::clearSlot (const juce::String& trackId, int sceneIndex)
{
    auto& edit = projects.getEdit();
    auto* track = findAudioTrack (edit, trackId);
    auto* slot = track != nullptr ? slotAt (*track, sceneIndex) : nullptr;
    auto* clip = slot != nullptr ? slot->getClip() : nullptr;

    if (clip == nullptr)
        return false;

    projects.getUndo().beginStep ("Clear Slot");
    clip->removeFromParent();
    return true;
}

std::vector<SlotInfo> Session::getSlots (const juce::String& trackId) const
{
    auto& edit = projects.getEdit();
    std::vector<SlotInfo> result;
    auto* track = findAudioTrack (edit, trackId);

    if (track == nullptr)
        return result;

    auto* slots = existingSlots (*track);
    const auto count = std::max (edit.getSceneList().getNumScenes(), slots != nullptr ? slots->getClipSlots().size() : 0);

    for (int index = 0; index < count; ++index)
    {
        SlotInfo info;
        info.sceneIndex = index;

        if (slots != nullptr && juce::isPositiveAndBelow (index, slots->getClipSlots().size()))
            if (auto* clip = slots->getClipSlots()[index]->getClip())
                fillSlot (info, *clip);

        result.push_back (std::move (info));
    }

    return result;
}

bool Session::launchSlot (const juce::String& trackId, int sceneIndex)
{
    auto* track = findAudioTrack (projects.getEdit(), trackId);
    auto* slot = track != nullptr ? slotAt (*track, sceneIndex) : nullptr;
    auto* clip = slot != nullptr ? slot->getClip() : nullptr;

    if (clip == nullptr)
        return false;

    if (auto handle = clip->getLaunchHandle())
        handle->play (std::nullopt);

    updatePlaySlotClips (*track);
    return true;
}

bool Session::stopSlot (const juce::String& trackId, int sceneIndex)
{
    auto* track = findAudioTrack (projects.getEdit(), trackId);
    auto* slot = track != nullptr ? slotAt (*track, sceneIndex) : nullptr;
    auto* clip = slot != nullptr ? slot->getClip() : nullptr;

    if (clip == nullptr || (! isPlayQueued (*clip) && ! isSlotPlaying (*clip)))
        return false;

    if (auto handle = clip->getLaunchHandle())
        handle->stop (std::nullopt);

    updatePlaySlotClips (*track);
    return true;
}

bool Session::launchScene (int sceneIndex)
{
    auto& edit = projects.getEdit();

    if (! juce::isPositiveAndBelow (sceneIndex, edit.getSceneList().getNumScenes()))
        return false;

    bool launched = false;

    for (auto* track : te::getAudioTracks (edit))
    {
        auto* slot = slotAt (*track, sceneIndex);
        auto* clip = slot != nullptr ? slot->getClip() : nullptr;

        if (clip == nullptr)
            continue;

        if (auto handle = clip->getLaunchHandle())
            handle->play (std::nullopt);

        updatePlaySlotClips (*track);
        launched = true;
    }

    return launched;
}

bool Session::stopAll()
{
    auto& edit = projects.getEdit();
    bool stopped = false;

    for (auto* track : te::getAudioTracks (edit))
    {
        auto* slots = existingSlots (*track);

        if (slots == nullptr)
            continue;

        for (auto* slot : slots->getClipSlots())
        {
            auto* clip = slot != nullptr ? slot->getClip() : nullptr;

            if (clip == nullptr || (! isPlayQueued (*clip) && ! isSlotPlaying (*clip)))
                continue;

            if (auto handle = clip->getLaunchHandle())
                handle->stop (std::nullopt);

            stopped = true;
        }

        updatePlaySlotClips (*track);
    }

    return stopped;
}

juce::Result Session::recordIntoArrangement()
{
    auto& edit = projects.getEdit();
    std::vector<CapturedSlot> captured;

    for (auto* track : te::getAudioTracks (edit))
    {
        auto* slots = existingSlots (*track);

        if (slots == nullptr)
            continue;

        for (auto* slot : slots->getClipSlots())
        {
            auto* clip = slot != nullptr ? slot->getClip() : nullptr;

            // Queued and not yet playing stays in the slot. The Arrangement
            // receives what is sounding, from the playhead.
            if (clip == nullptr || ! isSlotPlaying (*clip))
                continue;

            const auto position = clip->getPosition();
            const auto length = position.getLength().inSeconds();

            if (length <= 0.0)
                return juce::Result::fail ("Slot clip has no length");

            CapturedSlot cap;
            cap.track = track;
            cap.lengthSeconds = length;
            cap.offsetSeconds = position.getOffset().inSeconds();

            if (auto* wave = dynamic_cast<te::WaveAudioClip*> (clip))
            {
                if (! wave->getOriginalFile().existsAsFile())
                    return juce::Result::fail ("Slot clip has no audio file");

                cap.wave = wave;
            }
            else if (auto* midi = dynamic_cast<te::MidiClip*> (clip))
            {
                cap.midi = midi;
            }
            else
            {
                continue;
            }

            captured.push_back (cap);
        }
    }

    if (captured.empty())
        return juce::Result::fail ("No slot clips are playing");

    const auto startSeconds = std::max (0.0, edit.getTransport().getPosition().inSeconds());
    const auto maxEnd = te::Edit::getMaximumEditEnd().inSeconds();

    projects.getUndo().beginStep ("Record into Arrangement");
    int inserted = 0;

    for (auto& cap : captured)
    {
        auto length = cap.lengthSeconds;

        if (startSeconds >= maxEnd)
            length = 0.0;
        else if (startSeconds + length > maxEnd)
            length = maxEnd - startSeconds;

        if (length <= 0.0)
            continue;

        const auto start = te::TimePosition::fromSeconds (startSeconds);
        const auto range = te::TimeRange::between (start, start + te::TimeDuration::fromSeconds (length));

        if (cap.wave != nullptr)
        {
            // The slot already owns the source. The Arrangement clip points at
            // that file, from the slot's offset, instead of a full extra copy.
            auto clip = cap.track->insertWaveClip (cap.wave->getName(), cap.wave->getOriginalFile(),
                                                   { range, te::TimeDuration::fromSeconds (cap.offsetSeconds) },
                                                   false);

            if (clip == nullptr)
                continue;

            clip->getSourceFileReference().setToFile (cap.wave->getOriginalFile(),
                                                      te::SourceFileReference::PathStyle::alwaysAbsolute,
                                                      false);
            clip->beginRenderingNewProxyIfNeeded();
            ++inserted;
        }
        else if (cap.midi != nullptr)
        {
            auto clip = cap.track->insertMIDIClip (cap.midi->getName(), range, nullptr);

            if (clip == nullptr)
                continue;

            for (auto* note : cap.midi->getSequence().getNotes())
                clip->getSequence().addNote (*note, &edit.getUndoManager());

            ++inserted;
        }
    }

    if (inserted == 0)
        return juce::Result::fail ("The engine refused the arrangement clip");

    return juce::Result::ok();
}

} // namespace resamper
