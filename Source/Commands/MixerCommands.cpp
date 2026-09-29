#include "MixerCommands.h"
#include "AppCommandHost.h"
#include "ArgKeys.h"

namespace resamper
{

namespace
{
    bool isNumber (const juce::var& v)
    {
        return v.isInt() || v.isInt64() || v.isDouble();
    }

    /** args[name] if present, else fallback: mixer.addReturn and mixer.addBus name what they add. */
    juce::String nameOr (const juce::var& args, const juce::String& fallback)
    {
        if (auto* obj = args.getDynamicObject(); obj != nullptr && obj->hasProperty (ArgKeys::name))
            return args[ArgKeys::name].toString();

        return fallback;
    }
}

void registerMixerCommands (CommandRegistry& registry, Mixer& mixer, AppCommandHost& host)
{
    registry.add ({ "mixer.addReturn", "Add Return" }, [&mixer, &host] (const juce::var& args)
    {
        host.report (mixer.addReturn (nameOr (args, "Return")));
    });

    registry.add ({ "mixer.addSend", "Add Send" }, [&mixer, &host] (const juce::var& args)
    {
        if (isNumber (args[ArgKeys::bus]))
            host.report (mixer.addSend (args[ArgKeys::trackId].toString(), (int) args[ArgKeys::bus]));
    });

    // A send fader: one undo step per drag (see sendGainArgs).
    registry.add ({ "mixer.setSendGain", "Set Send Gain" }, [&mixer] (const juce::var& args)
    {
        if (args[ArgKeys::value].isDouble() || args[ArgKeys::value].isInt())
            mixer.setSendGain (args[ArgKeys::trackId].toString(), args[ArgKeys::sendId].toString(),
                               args[ArgKeys::value], (bool) args[ArgKeys::continuesGesture]);
    });

    registry.add ({ "mixer.setSendMuted", "Mute Send" }, [&mixer] (const juce::var& args)
    {
        if (args[ArgKeys::muted].isBool())
            mixer.setSendMuted (args[ArgKeys::trackId].toString(), args[ArgKeys::sendId].toString(),
                                (bool) args[ArgKeys::muted]);
    });

    registry.add ({ "mixer.addBus", "Add Bus" }, [&mixer, &host] (const juce::var& args)
    {
        host.report (mixer.addBus (nameOr (args, "Bus")));
    });

    registry.add ({ "mixer.moveToBus", "Move to Bus" }, [&mixer] (const juce::var& args)
    {
        mixer.moveTrackToBus (args[ArgKeys::trackId].toString(), args[ArgKeys::busTrackId].toString());
    });

    // The master fader: one undo step per drag (see masterVolumeArgs).
    registry.add ({ "mixer.setMasterVolume", "Set Master Volume" }, [&mixer] (const juce::var& args)
    {
        if (args[ArgKeys::value].isDouble() || args[ArgKeys::value].isInt())
            mixer.setMasterVolume (args[ArgKeys::value], (bool) args[ArgKeys::continuesGesture]);
    });
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

} // namespace resamper
