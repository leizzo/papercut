#include "AutomationCommands.h"
#include "AppCommandHost.h"

namespace resamper
{

void registerAutomationCommands (CommandRegistry& registry, Automation& automation, Shaper& shaper, AppCommandHost& host)
{
    registry.add (cmd::automationAddPoint, { "Add Automation Point" }, [&automation] (const AutomationPointArgs& a)
    {
        automation.addPoint (a.trackId, a.parameter, a.timeSeconds, a.value);
    });

    registry.add (cmd::automationMovePoint, { "Move Automation Point" }, [&automation] (const AutomationMoveArgs& a)
    {
        automation.movePoint (a.trackId, a.parameter, a.index, a.timeSeconds, a.value);
    });

    registry.add (cmd::automationRemovePoint, { "Remove Automation Point" }, [&automation] (const AutomationRemoveArgs& a)
    {
        automation.removePoint (a.trackId, a.parameter, a.index);
    });

    registry.add (cmd::automationClear, { "Clear Automation" }, [&automation] (const AutomationLaneArgs& a)
    {
        automation.clear (a.trackId, a.parameter);
    });

    registry.add (cmd::shaperAdd, { "Add Shaper" }, [&shaper, &host] (const ShaperAddArgs& a)
    {
        host.report (shaper.add (a.trackId, a.parameter, a.mode));
    });

    registry.add (cmd::shaperRemove, { "Remove Shaper" }, [&shaper] (const ShaperArgs& a) { shaper.remove (a.shaperId); });

    registry.add (cmd::shaperSetLoop, { "Set Shaper Loop" }, [&shaper] (const ShaperLoopArgs& a)
    {
        shaper.setLoop (a.shaperId, a.lengthBeats, a.shape, a.depth);
    });

    registry.add (cmd::shaperSetAudioTrigger, { "Set Shaper Audio Trigger" }, [&shaper] (const ShaperTriggerArgs& a)
    {
        shaper.setAudioTrigger (a.shaperId, a.attack, a.hold, a.release, a.thresholdDb, a.depth);
    });
}

} // namespace resamper
