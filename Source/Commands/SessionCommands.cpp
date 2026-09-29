#include "SessionCommands.h"

#include "AppCommandHost.h"
#include "Engine/Session.h"

namespace resamper
{

void registerSessionCommands (CommandRegistry& registry, Session& session, AppCommandHost& host)
{
    registry.add (cmd::sessionSetSceneCount, { "Set Scene Count" }, [&session, &host] (const int& count)
    {
        host.report (session.setSceneCount (count));
    });

    registry.add (cmd::sessionRenameScene, { "Rename Scene" }, [&session] (const SceneNameArgs& a)
    {
        session.renameScene (a.index, a.name);
    });

    registry.add (cmd::sessionAddSlotClip, { "Add Slot Clip..." }, [&session, &host] (const SlotArgs& a)
    {
        if (! host.chooseAudioFile)
            return;

        host.chooseAudioFile ([&session, &host, a] (const juce::File& file)
        {
            host.report (session.addSlotClip (a.trackId, a.scene, file));
        });
    });

    registry.add (cmd::sessionAddMidiSlotClip, { "Add MIDI Slot Clip" }, [&session, &host] (const SlotArgs& a)
    {
        host.report (session.addMidiSlotClip (a.trackId, a.scene));
    });

    registry.add (cmd::sessionClearSlot, { "Clear Slot" }, [&session] (const SlotArgs& a) { session.clearSlot (a.trackId, a.scene); });
    registry.add (cmd::sessionLaunchSlot, { "Launch Slot" }, [&session] (const SlotArgs& a) { session.launchSlot (a.trackId, a.scene); });
    registry.add (cmd::sessionLaunchScene, { "Launch Scene" }, [&session] (const int& index) { session.launchScene (index); });
    registry.add (cmd::sessionStopAll, { "Stop All Slots" }, [&session] { session.stopAll(); });

    registry.add (cmd::sessionRecordToArrangement, { "Record into Arrangement" }, [&session, &host]
    {
        host.report (session.recordIntoArrangement());
    });
}

} // namespace resamper
