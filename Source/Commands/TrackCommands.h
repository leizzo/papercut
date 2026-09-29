#pragma once

#include "CommandRegistry.h"
#include "Engine/ApplicationModel.h"

namespace resamper
{

struct AppCommandHost;

/** Registers the track Commands:

    track.add    track.addMidi   track.remove
    track.setVolume  track.setPan  track.toggleMute  track.toggleSolo
    track.setInput   track.toggleArm   track.setColour   track.select
    track.toggleMuteAt (args: argument = track index)   track.toggleSoloSelected
*/
void registerTrackCommands (CommandRegistry&, ApplicationModel&, AppCommandHost&);

/** Arguments for track.toggleMute, track.toggleSolo, track.toggleArm and track.select. */
juce::var trackArgs (const juce::String& trackId);

/** Arguments for track.setVolume. A continuous gesture (a fader drag) passes
    continuesGesture for every value after its first, making the whole gesture
    one undo step. */
juce::var trackVolumeArgs (const juce::String& trackId, double db, bool continuesGesture = false);

/** Arguments for track.setPan (-1 left to 1 right); continuesGesture as for trackVolumeArgs. */
juce::var trackPanArgs (const juce::String& trackId, double pan, bool continuesGesture = false);

/** Arguments for track.setColour: an index into the track palette. */
juce::var trackColourArgs (const juce::String& trackId, int colourIndex);

/** Arguments for track.setInput: an input named by ApplicationModel::getAudioInputs(),
    or empty for none. */
juce::var trackInputArgs (const juce::String& trackId, const juce::String& inputName);

} // namespace resamper
