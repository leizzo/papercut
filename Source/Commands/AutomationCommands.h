#pragma once

#include "CommandRegistry.h"
#include "Engine/Automation.h"
#include "Engine/Shaper.h"

#include <vector>

namespace papercut
{

struct AppCommandHost;

/** Registers automation and shaper Commands:

    automation.addPoint        trackId, parameter, time, value
    automation.movePoint       trackId, parameter, index, time, value
    automation.removePoint     trackId, parameter, index
    automation.clear           trackId, parameter
    shaper.add                 trackId, parameter, mode ("loop" or "audioTrigger")
    shaper.remove              shaperId
    shaper.setLoop             shaperId, lengthBeats, depth, shape (array of {time, value})
    shaper.setAudioTrigger     shaperId, attack, hold, release, thresholdDb, depth
*/
void registerAutomationCommands (CommandRegistry&, Automation&, Shaper&, AppCommandHost&);

juce::var automationPointArgs (const juce::String& trackId, const juce::String& parameter, double timeSeconds, float value);
juce::var automationMoveArgs (const juce::String& trackId, const juce::String& parameter, int index, double timeSeconds, float value);
juce::var automationRemoveArgs (const juce::String& trackId, const juce::String& parameter, int index);
juce::var automationClearArgs (const juce::String& trackId, const juce::String& parameter);

juce::var shaperAddArgs (const juce::String& trackId, const juce::String& parameter, ShaperMode mode);
juce::var shaperRemoveArgs (const juce::String& shaperId);
juce::var shaperSetLoopArgs (const juce::String& shaperId, double lengthBeats, float depth,
                             const std::vector<ShaperShapePoint>& shape);
juce::var shaperSetAudioTriggerArgs (const juce::String& shaperId, float attack, float hold, float release,
                                     float thresholdDb, float depth);

} // namespace papercut
