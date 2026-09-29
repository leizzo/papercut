#include "SessionCommands.h"

#include "AppCommandHost.h"
#include "ArgKeys.h"
#include "Engine/Session.h"

namespace resamper
{

namespace
{
    bool readInt (const juce::var& args, const juce::Identifier& key, int& value)
    {
        auto stored = args[key];

        if (! stored.isInt() && ! stored.isInt64() && ! stored.isDouble())
            return false;

        value = (int) stored;
        return true;
    }
}

void registerSessionCommands (CommandRegistry& registry, Session& session, AppCommandHost& host)
{
    registry.add ({ "session.setSceneCount", "Set Scene Count" }, [&session, &host] (const juce::var& args)
    {
        int count = 0;

        if (readInt (args, ArgKeys::count, count))
            host.report (session.setSceneCount (count));
    });

    registry.add ({ "session.renameScene", "Rename Scene" }, [&session] (const juce::var& args)
    {
        int index = 0;

        if (readInt (args, ArgKeys::index, index) && args[ArgKeys::name].isString())
            session.renameScene (index, args[ArgKeys::name].toString());
    });

    registry.add ({ "session.addSlotClip", "Add Slot Clip..." }, [&session, &host] (const juce::var& args)
    {
        int scene = 0;

        if (! readInt (args, ArgKeys::scene, scene) || ! host.chooseAudioFile)
            return;

        const auto trackId = args[ArgKeys::trackId].toString();

        host.chooseAudioFile ([&session, &host, trackId, scene] (const juce::File& file)
        {
            host.report (session.addSlotClip (trackId, scene, file));
        });
    });

    registry.add ({ "session.addMidiSlotClip", "Add MIDI Slot Clip" }, [&session, &host] (const juce::var& args)
    {
        int scene = 0;

        if (readInt (args, ArgKeys::scene, scene))
            host.report (session.addMidiSlotClip (args[ArgKeys::trackId].toString(), scene));
    });

    registry.add ({ "session.clearSlot", "Clear Slot" }, [&session] (const juce::var& args)
    {
        int scene = 0;

        if (readInt (args, ArgKeys::scene, scene))
            session.clearSlot (args[ArgKeys::trackId].toString(), scene);
    });

    registry.add ({ "session.launchSlot", "Launch Slot" }, [&session] (const juce::var& args)
    {
        int scene = 0;

        if (readInt (args, ArgKeys::scene, scene))
            session.launchSlot (args[ArgKeys::trackId].toString(), scene);
    });

    registry.add ({ "session.launchScene", "Launch Scene" }, [&session] (const juce::var& args)
    {
        int index = 0;

        if (readInt (args, ArgKeys::index, index))
            session.launchScene (index);
    });

    registry.add ({ "session.stopAll", "Stop All Slots" }, [&session] { session.stopAll(); });

    registry.add ({ "session.recordToArrangement", "Record into Arrangement" }, [&session, &host]
    {
        host.report (session.recordIntoArrangement());
    });
}

juce::var sessionSceneCountArgs (int count)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::count, count);
    return args;
}

juce::var sessionRenameSceneArgs (int index, const juce::String& name)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::index, index);
    args->setProperty (ArgKeys::name, name);
    return args;
}

juce::var sessionSlotArgs (const juce::String& trackId, int sceneIndex)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::trackId, trackId);
    args->setProperty (ArgKeys::scene, sceneIndex);
    return args;
}

juce::var sessionSceneArgs (int index)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::index, index);
    return args;
}

} // namespace resamper
