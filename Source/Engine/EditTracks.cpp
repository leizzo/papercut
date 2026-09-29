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

te::AudioTrack* findAudioTrack (const te::Edit& edit, const juce::String& trackId)
{
    return te::findAudioTrackForID (edit, te::EditItemID::fromString (trackId));
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

bool isReturnTrack (const te::AudioTrack& track)
{
    return ! track.pluginList.getPluginsOfType<te::AuxReturnPlugin>().isEmpty();
}

} // namespace resamper
