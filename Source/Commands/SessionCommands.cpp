#include "SessionCommands.h"

#include "AppCommands.h"
#include "Engine/Session.h"

namespace resamper
{

namespace
{
    const juce::Identifier countKey ("count"), indexKey ("index"), nameKey ("name"),
                           trackIdKey ("trackId"), sceneKey ("scene");

    class SessionCommand : public Command
    {
    public:
        SessionCommand (juce::String id, juce::String name, Session& s, AppCommandHost& h)
            : Command (std::move (id), std::move (name)), session (s), host (h) {}

    protected:
        Session& session;
        AppCommandHost& host;

        void report (const juce::Result& result) const
        {
            if (result.failed() && host.reportError)
                host.reportError (result.getErrorMessage());
        }

        static bool readInt (const juce::var& args, const juce::Identifier& key, int& value)
        {
            auto stored = args[key];

            if (! stored.isInt() && ! stored.isInt64() && ! stored.isDouble())
                return false;

            value = (int) stored;
            return true;
        }
    };

    struct SetSceneCountCommand : SessionCommand
    {
        SetSceneCountCommand (Session& s, AppCommandHost& h) : SessionCommand ("session.setSceneCount", "Set Scene Count", s, h) {}

        void execute (const juce::var& args) override
        {
            int count = 0;

            if (readInt (args, countKey, count))
                report (session.setSceneCount (count));
        }
    };

    struct RenameSceneCommand : SessionCommand
    {
        RenameSceneCommand (Session& s, AppCommandHost& h) : SessionCommand ("session.renameScene", "Rename Scene", s, h) {}

        void execute (const juce::var& args) override
        {
            int index = 0;

            if (readInt (args, indexKey, index) && args[nameKey].isString())
                session.renameScene (index, args[nameKey].toString());
        }
    };

    struct AddSlotClipCommand : SessionCommand
    {
        AddSlotClipCommand (Session& s, AppCommandHost& h) : SessionCommand ("session.addSlotClip", "Add Slot Clip...", s, h) {}

        void execute (const juce::var& args) override
        {
            int scene = 0;

            if (! readInt (args, sceneKey, scene) || ! host.chooseAudioFile)
                return;

            const auto trackId = args[trackIdKey].toString();

            host.chooseAudioFile ([this, trackId, scene] (const juce::File& file)
            {
                report (session.addSlotClip (trackId, scene, file));
            });
        }
    };

    struct AddMidiSlotClipCommand : SessionCommand
    {
        AddMidiSlotClipCommand (Session& s, AppCommandHost& h)
            : SessionCommand ("session.addMidiSlotClip", "Add MIDI Slot Clip", s, h) {}

        void execute (const juce::var& args) override
        {
            int scene = 0;

            if (readInt (args, sceneKey, scene))
                report (session.addMidiSlotClip (args[trackIdKey].toString(), scene));
        }
    };

    struct ClearSlotCommand : SessionCommand
    {
        ClearSlotCommand (Session& s, AppCommandHost& h) : SessionCommand ("session.clearSlot", "Clear Slot", s, h) {}

        void execute (const juce::var& args) override
        {
            int scene = 0;

            if (readInt (args, sceneKey, scene))
                session.clearSlot (args[trackIdKey].toString(), scene);
        }
    };

    struct LaunchSlotCommand : SessionCommand
    {
        LaunchSlotCommand (Session& s, AppCommandHost& h) : SessionCommand ("session.launchSlot", "Launch Slot", s, h) {}

        void execute (const juce::var& args) override
        {
            int scene = 0;

            if (readInt (args, sceneKey, scene))
                session.launchSlot (args[trackIdKey].toString(), scene);
        }
    };

    struct LaunchSceneCommand : SessionCommand
    {
        LaunchSceneCommand (Session& s, AppCommandHost& h) : SessionCommand ("session.launchScene", "Launch Scene", s, h) {}

        void execute (const juce::var& args) override
        {
            int index = 0;

            if (readInt (args, indexKey, index))
                session.launchScene (index);
        }
    };

    struct StopAllCommand : SessionCommand
    {
        StopAllCommand (Session& s, AppCommandHost& h) : SessionCommand ("session.stopAll", "Stop All Slots", s, h) {}

        void execute (const juce::var&) override   { session.stopAll(); }
    };

    struct RecordToArrangementCommand : SessionCommand
    {
        RecordToArrangementCommand (Session& s, AppCommandHost& h)
            : SessionCommand ("session.recordToArrangement", "Record into Arrangement", s, h) {}

        void execute (const juce::var&) override   { report (session.recordIntoArrangement()); }
    };
}

void registerSessionCommands (CommandRegistry& registry, Session& session, AppCommandHost& host)
{
    registry.add (std::make_unique<SetSceneCountCommand> (session, host));
    registry.add (std::make_unique<RenameSceneCommand> (session, host));
    registry.add (std::make_unique<AddSlotClipCommand> (session, host));
    registry.add (std::make_unique<AddMidiSlotClipCommand> (session, host));
    registry.add (std::make_unique<ClearSlotCommand> (session, host));
    registry.add (std::make_unique<LaunchSlotCommand> (session, host));
    registry.add (std::make_unique<LaunchSceneCommand> (session, host));
    registry.add (std::make_unique<StopAllCommand> (session, host));
    registry.add (std::make_unique<RecordToArrangementCommand> (session, host));
}

juce::var sessionSceneCountArgs (int count)
{
    auto args = new juce::DynamicObject();
    args->setProperty (countKey, count);
    return args;
}

juce::var sessionRenameSceneArgs (int index, const juce::String& name)
{
    auto args = new juce::DynamicObject();
    args->setProperty (indexKey, index);
    args->setProperty (nameKey, name);
    return args;
}

juce::var sessionSlotArgs (const juce::String& trackId, int sceneIndex)
{
    auto args = new juce::DynamicObject();
    args->setProperty (trackIdKey, trackId);
    args->setProperty (sceneKey, sceneIndex);
    return args;
}

juce::var sessionSceneArgs (int index)
{
    auto args = new juce::DynamicObject();
    args->setProperty (indexKey, index);
    return args;
}

} // namespace resamper
