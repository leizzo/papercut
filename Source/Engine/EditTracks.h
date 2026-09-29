#pragma once

#include "ApplicationModel.h"

#include <tracktion_engine/tracktion_engine.h>

// Track rules every facade shares: looking up an audio track by id, its Track
// Kind (a saved property) and whether it is a Return. Engine-internal: it
// names Tracktion types.

namespace resamper
{

/** The audio track with this id (an EditItemID string), or nullptr. */
tracktion::AudioTrack* findAudioTrack (const tracktion::Edit&, const juce::String& trackId);

/** A track's Track Kind. It is a saved property, not "whichever instrument is
    loaded"; absent (engine-made tracks, older projects) means audio. */
TrackKind trackKindOf (const tracktion::Track&);

/** Whether the track's Track Kind is MIDI. */
bool isMidi (const tracktion::Track&);

/** Gives a new track the MIDI Track Kind, the only way a track gets one. */
void markMidi (tracktion::Track&, juce::UndoManager*);

/** Whether the track is a Return: it holds an aux return. */
bool isReturnTrack (const tracktion::AudioTrack&);

} // namespace resamper
