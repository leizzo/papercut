#pragma once

#include "CommandRegistry.h"
#include "Engine/ApplicationModel.h"

namespace resamper
{

struct AppCommandHost;

/** Registers the transport Commands:

    transport.play  transport.stop  transport.togglePlay  transport.returnToStart
    transport.setPosition
    transport.record  transport.toggleLoop  transport.setLoopRange
    transport.loopSelection  transport.playFromSelection
    transport.setTempo  transport.tapTempo  transport.setTimeSignature  transport.toggleMetronome
    transport.toggleCountIn
*/
void registerTransportCommands (CommandRegistry&, ApplicationModel&, AppCommandHost&);

/** Arguments for transport.record: withCountIn false records at once (Shift-click Rec). */
juce::var recordArgs (bool withCountIn);

/** Arguments for transport.setLoopRange, which also turns looping on. */
juce::var loopRangeArgs (double startSeconds, double endSeconds);

/** Arguments for transport.setPosition: where the playhead moves, in seconds. */
juce::var transportPositionArgs (double seconds);

/** Arguments for transport.setTempo; continuesGesture joins a drag into one undo step. */
juce::var tempoArgs (double bpm, bool continuesGesture = false);

/** Arguments for transport.setTimeSignature. */
juce::var timeSignatureArgs (int numerator, int denominator);

} // namespace resamper
