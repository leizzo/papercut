#include "MixerCommands.h"
#include "AppCommands.h"

namespace papercut
{

namespace
{
    namespace ArgKeys
    {
        const juce::Identifier name ("name"), trackId ("trackId"), bus ("bus"), sendId ("sendId"),
                               value ("value"), continuesGesture ("continuesGesture"), muted ("muted"),
                               busTrackId ("busTrackId");
    }

    bool isNumber (const juce::var& v)
    {
        return v.isInt() || v.isInt64() || v.isDouble();
    }

    class MixerCommand : public Command
    {
    public:
        MixerCommand (juce::String id, juce::String commandName, Mixer& mx, AppCommandHost& h)
            : Command (std::move (id), std::move (commandName)), mixer (mx), host (h) {}

    protected:
        Mixer& mixer;
        AppCommandHost& host;

        void report (const juce::Result& r) const
        {
            if (r.failed() && host.reportError)
                host.reportError (r.getErrorMessage());
        }
    };

    struct AddReturnCommand : MixerCommand
    {
        AddReturnCommand (Mixer& m, AppCommandHost& h) : MixerCommand ("mixer.addReturn", "Add Return", m, h) {}

        void execute (const juce::var& args) override
        {
            auto name = juce::String ("Return");

            if (auto* obj = args.getDynamicObject(); obj != nullptr && obj->hasProperty (ArgKeys::name))
                name = args[ArgKeys::name].toString();

            report (mixer.addReturn (name));
        }
    };

    struct AddSendCommand : MixerCommand
    {
        AddSendCommand (Mixer& m, AppCommandHost& h) : MixerCommand ("mixer.addSend", "Add Send", m, h) {}

        void execute (const juce::var& args) override
        {
            if (isNumber (args[ArgKeys::bus]))
                report (mixer.addSend (args[ArgKeys::trackId].toString(), (int) args[ArgKeys::bus]));
        }
    };

    /** A send fader: one undo step per drag (see sendGainArgs). */
    struct SetSendGainCommand : MixerCommand
    {
        SetSendGainCommand (Mixer& m, AppCommandHost& h) : MixerCommand ("mixer.setSendGain", "Set Send Gain", m, h) {}

        void execute (const juce::var& args) override
        {
            if (args[ArgKeys::value].isDouble() || args[ArgKeys::value].isInt())
                mixer.setSendGain (args[ArgKeys::trackId].toString(), args[ArgKeys::sendId].toString(),
                                   args[ArgKeys::value], (bool) args[ArgKeys::continuesGesture]);
        }
    };

    struct SetSendMutedCommand : MixerCommand
    {
        SetSendMutedCommand (Mixer& m, AppCommandHost& h) : MixerCommand ("mixer.setSendMuted", "Mute Send", m, h) {}

        void execute (const juce::var& args) override
        {
            if (args[ArgKeys::muted].isBool())
                mixer.setSendMuted (args[ArgKeys::trackId].toString(), args[ArgKeys::sendId].toString(),
                                    (bool) args[ArgKeys::muted]);
        }
    };

    struct AddBusCommand : MixerCommand
    {
        AddBusCommand (Mixer& m, AppCommandHost& h) : MixerCommand ("mixer.addBus", "Add Bus", m, h) {}

        void execute (const juce::var& args) override
        {
            auto name = juce::String ("Bus");

            if (auto* obj = args.getDynamicObject(); obj != nullptr && obj->hasProperty (ArgKeys::name))
                name = args[ArgKeys::name].toString();

            report (mixer.addBus (name));
        }
    };

    struct MoveToBusCommand : MixerCommand
    {
        MoveToBusCommand (Mixer& m, AppCommandHost& h) : MixerCommand ("mixer.moveToBus", "Move to Bus", m, h) {}

        void execute (const juce::var& args) override
        {
            mixer.moveTrackToBus (args[ArgKeys::trackId].toString(), args[ArgKeys::busTrackId].toString());
        }
    };

    /** The master fader: one undo step per drag (see masterVolumeArgs). */
    struct SetMasterVolumeCommand : MixerCommand
    {
        SetMasterVolumeCommand (Mixer& m, AppCommandHost& h) : MixerCommand ("mixer.setMasterVolume", "Set Master Volume", m, h) {}

        void execute (const juce::var& args) override
        {
            if (args[ArgKeys::value].isDouble() || args[ArgKeys::value].isInt())
                mixer.setMasterVolume (args[ArgKeys::value], (bool) args[ArgKeys::continuesGesture]);
        }
    };

}

void registerMixerCommands (CommandRegistry& registry, Mixer& mixer, AppCommandHost& host)
{
    registry.add (std::make_unique<AddReturnCommand> (mixer, host));
    registry.add (std::make_unique<AddSendCommand> (mixer, host));
    registry.add (std::make_unique<SetSendGainCommand> (mixer, host));
    registry.add (std::make_unique<SetSendMutedCommand> (mixer, host));
    registry.add (std::make_unique<AddBusCommand> (mixer, host));
    registry.add (std::make_unique<MoveToBusCommand> (mixer, host));
    registry.add (std::make_unique<SetMasterVolumeCommand> (mixer, host));
}

juce::var returnArgs (const juce::String& name)
{
    auto args = new juce::DynamicObject();
    args->setProperty ("name", name);
    return args;
}

juce::var sendArgs (const juce::String& trackId, int bus)
{
    auto args = new juce::DynamicObject();
    args->setProperty ("trackId", trackId);
    args->setProperty ("bus", bus);
    return args;
}

juce::var sendGainArgs (const juce::String& trackId, const juce::String& sendId, double gainDb, bool continuesGesture)
{
    auto args = new juce::DynamicObject();
    args->setProperty ("trackId", trackId);
    args->setProperty ("sendId", sendId);
    args->setProperty ("value", gainDb);
    args->setProperty ("continuesGesture", continuesGesture);
    return args;
}

juce::var sendMutedArgs (const juce::String& trackId, const juce::String& sendId, bool muted)
{
    auto args = new juce::DynamicObject();
    args->setProperty ("trackId", trackId);
    args->setProperty ("sendId", sendId);
    args->setProperty ("muted", muted);
    return args;
}

juce::var busArgs (const juce::String& name)
{
    auto args = new juce::DynamicObject();
    args->setProperty ("name", name);
    return args;
}

juce::var moveToBusArgs (const juce::String& trackId, const juce::String& busTrackId)
{
    auto args = new juce::DynamicObject();
    args->setProperty ("trackId", trackId);
    args->setProperty ("busTrackId", busTrackId);
    return args;
}

juce::var masterVolumeArgs (double db, bool continuesGesture)
{
    auto args = new juce::DynamicObject();
    args->setProperty ("value", db);
    args->setProperty ("continuesGesture", continuesGesture);
    return args;
}

} // namespace papercut
