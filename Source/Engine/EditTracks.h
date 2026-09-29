#pragma once

#include "ApplicationModel.h"

#include <tracktion_engine/tracktion_engine.h>

namespace resamper
{

/** What a track is in the Edit, for every facade: the one place the app's
    track rules and their saved properties live. Engine-internal: it names
    Tracktion types. */

/** The audio track with this id (an EditItemID string), or nullptr. */
tracktion::AudioTrack* findAudioTrack (const tracktion::Edit&, const juce::String& trackId);

/** A track's Track Kind. It is a saved property, not "whichever instrument is
    loaded"; absent (engine-made tracks, older projects) means audio. */
TrackKind trackKindOf (const tracktion::Track&);

/** Gives a new track the MIDI Track Kind, the only way a track gets one. */
void markMidi (tracktion::Track&, juce::UndoManager*);

/** Whether the track is a Return: it holds an aux return. */
bool isReturn (const tracktion::AudioTrack&);

} // namespace resamper
