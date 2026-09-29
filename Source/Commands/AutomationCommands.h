#pragma once

#include "CommandRegistry.h"
#include "Engine/Automation.h"
#include "Engine/Shaper.h"

#include <vector>

namespace resamper
{

struct AppCommandHost;

/** An automation lane: a track's parameter. */
struct AutomationLaneArgs
{
    juce::String trackId;
    juce::String parameter;
};

/** A new point on a lane. */
struct AutomationPointArgs
{
    juce::String trackId;
    juce::String parameter;
    double timeSeconds = 0;
    float value = 0;
};

/** A point's new position. */
struct AutomationMoveArgs
{
    juce::String trackId;
    juce::String parameter;
    int index = 0;
    double timeSeconds = 0;
    float value = 0;
};

/** The point at index on a lane. */
struct AutomationRemoveArgs
{
    juce::String trackId;
    juce::String parameter;
    int index = 0;
};

/** A new Shaper on a track's parameter. */
struct ShaperAddArgs
{
    juce::String trackId;
    juce::String parameter;
    ShaperMode mode = ShaperMode::loop;
};

/** Names one Shaper. */
struct ShaperArgs
{
    juce::String shaperId;
};

/** A loop Shaper's shape; left out, a rising ramp. */
struct ShaperLoopArgs
{
    juce::String shaperId;
    double lengthBeats = 0;
    float depth = 0;
    std::vector<ShaperShapePoint> shape { { 0.0f, 0.0f }, { 1.0f, 1.0f } };
};

/** An audio-trigger Shaper's envelope and depth. */
struct ShaperTriggerArgs
{
    juce::String shaperId;
    float attack = 0, hold = 0, release = 0, thresholdDb = 0, depth = 0;
};

namespace cmd
{
    inline constexpr CommandRef<AutomationPointArgs> automationAddPoint { "automation.addPoint" };
    inline constexpr CommandRef<AutomationMoveArgs> automationMovePoint { "automation.movePoint" };
    inline constexpr CommandRef<AutomationRemoveArgs> automationRemovePoint { "automation.removePoint" };
    inline constexpr CommandRef<AutomationLaneArgs> automationClear { "automation.clear" };
    inline constexpr CommandRef<ShaperAddArgs> shaperAdd { "shaper.add" };
    inline constexpr CommandRef<ShaperArgs> shaperRemove { "shaper.remove" };
    inline constexpr CommandRef<ShaperLoopArgs> shaperSetLoop { "shaper.setLoop" };
    inline constexpr CommandRef<ShaperTriggerArgs> shaperSetAudioTrigger { "shaper.setAudioTrigger" };
}

/** Registers the automation and Shaper Commands above. */
void registerAutomationCommands (CommandRegistry&, Automation&, Shaper&, AppCommandHost&);

} // namespace resamper
