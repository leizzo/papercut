#include "EditTracks.h"

namespace te = tracktion;

namespace resamper
{

namespace
{
    /** On the track's ValueTree. Saved in project files: never rename. */
    const juce::Identifier trackKindProperty { "resamperKind" };
    const juce::String midiKindValue { "midi" };
}

const juce::Identifier colourProperty { "resamperColour" };

te::AudioTrack* findAudioTrack (const te::Edit& edit, const juce::String& trackId)
{
    return te::findAudioTrackForID (edit, te::EditItemID::fromString (trackId));
}

bool isStripTrack (const te::Track& track)
{
    return dynamic_cast<const te::AudioTrack*> (&track) != nullptr || isBus (track);
}

te::Track* findStripTrack (const te::Edit& edit, const juce::String& trackId)
{
    auto* track = te::findTrackForID (edit, te::EditItemID::fromString (trackId));
    return track != nullptr && isStripTrack (*track) ? track : nullptr;
}

bool isBus (const te::Track& track)
{
    auto* folder = dynamic_cast<const te::FolderTrack*> (&track);
    return folder != nullptr && folder->isSubmixFolder();
}

te::VolumeAndPanPlugin* faderOf (te::Track& track)
{
    if (auto* audio = dynamic_cast<te::AudioTrack*> (&track))
        return audio->getVolumePlugin();

    if (auto* folder = dynamic_cast<te::FolderTrack*> (&track))
        return folder->getVolumePlugin();

    return nullptr;
}

TrackKind trackKindOf (const te::Track& track)
{
    return track.state[trackKindProperty].toString() == midiKindValue ? TrackKind::midi : TrackKind::audio;
}

bool isMidi (const te::Track& track)
{
    return trackKindOf (track) == TrackKind::midi;
}

void markMidi (te::Track& track, juce::UndoManager* undoManager)
{
    track.state.setProperty (trackKindProperty, midiKindValue, undoManager);
}

int colourOf (const te::Track& track)
{
    if (auto colour = track.state[colourProperty]; colour.isInt())
        return juce::jlimit (0, ApplicationModel::trackPaletteSize - 1, (int) colour);

    if (auto* audio = dynamic_cast<const te::AudioTrack*> (&track))
        return te::getAudioTracks (track.edit).indexOf (const_cast<te::AudioTrack*> (audio)) % ApplicationModel::trackPaletteSize;

    const auto children = track.getAllSubTracks (false);
    return children.isEmpty() ? 0 : colourOf (*children.getFirst());
}

bool isReturnTrack (const te::AudioTrack& track)
{
    return ! track.pluginList.getPluginsOfType<te::AuxReturnPlugin>().isEmpty();
}

} // namespace resamper
