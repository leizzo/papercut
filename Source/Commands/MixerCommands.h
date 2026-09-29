#pragma once

#include "CommandRegistry.h"
#include "Engine/Mixer.h"

namespace resamper
{

struct AppCommandHost;

/** Registers the mixer Commands. They delegate to Mixer; undo stays in the engine.

    mixer.addReturn        name (optional; default "Return")
    mixer.addSend          trackId, bus
    mixer.setSendGain      trackId, sendId, value, continuesGesture
    mixer.setSendMuted     trackId, sendId, muted
    mixer.addBus           name
    mixer.moveToBus        trackId, busTrackId
    mixer.setMasterVolume  value, continuesGesture
*/
void registerMixerCommands (CommandRegistry&, Mixer&, AppCommandHost&);

/** Arguments for mixer.addReturn. Invoked with no args, the Command uses "Return". */
juce::var returnArgs (const juce::String& name = "Return");

/** Arguments for mixer.addSend: the source track and the return's bus number. */
juce::var sendArgs (const juce::String& trackId, int bus);

/** Arguments for mixer.setSendGain. continuesGesture joins a drag into one undo step. */
juce::var sendGainArgs (const juce::String& trackId, const juce::String& sendId, double gainDb, bool continuesGesture = false);

/** Arguments for mixer.setSendMuted. */
juce::var sendMutedArgs (const juce::String& trackId, const juce::String& sendId, bool muted);

/** Arguments for mixer.addBus. */
juce::var busArgs (const juce::String& name);

/** Arguments for mixer.moveToBus. */
juce::var moveToBusArgs (const juce::String& trackId, const juce::String& busTrackId);

/** Arguments for mixer.setMasterVolume. continuesGesture as for sendGainArgs. */
juce::var masterVolumeArgs (double db, bool continuesGesture = false);

} // namespace resamper
