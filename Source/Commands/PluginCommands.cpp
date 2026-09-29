#include "PluginCommands.h"

#include "AppCommands.h"
#include "ArgKeys.h"
#include "Engine/PluginRack.h"

namespace resamper
{

namespace
{
    /** Reports a message that isn't from a Result: an empty one reports nothing. */
    void report (const AppCommandHost& host, const juce::String& message)
    {
        if (message.isNotEmpty() && host.reportError)
            host.reportError (message);
    }
}

void registerPluginCommands (CommandRegistry& registry, PluginRack& rack, AppCommandHost& host)
{
    registry.add ({ "plugin.scan", "Scan Plug-ins", [&rack] { return ! rack.isScanning(); } }, [&rack] { rack.startScan(); });

    registry.add ({ "plugin.insert", "Insert Plug-in" }, [&rack, &host] (const juce::var& args)
    {
        const auto id = args[ArgKeys::trackId].toString();
        const auto plugin = args[ArgKeys::plugin].toString();
        const auto chain = args[ArgKeys::chain].toString() == "mixer" ? PluginChain::mixer : PluginChain::device;

        if (id.isEmpty() || plugin.isEmpty())
            report (host, "Plug-in insert needs a track and a plug-in");
        else
            host.report (rack.insert (id, plugin, chain));
    });

    registry.add ({ "plugin.remove", "Remove Plug-in" }, [&rack, &host] (const juce::var& args)
    {
        const auto id = args[ArgKeys::trackId].toString();
        const auto pluginId = args[ArgKeys::pluginId].toString();

        if (id.isEmpty() || pluginId.isEmpty())
            report (host, "Plug-in remove needs a track and a plug-in");
        else if (! rack.remove (id, pluginId))
            report (host, "Couldn't remove the plug-in");
    });

    registry.add ({ "plugin.move", "Move Plug-in" }, [&rack, &host] (const juce::var& args)
    {
        const auto id = args[ArgKeys::trackId].toString();
        const auto pluginId = args[ArgKeys::pluginId].toString();
        const auto indexVar = args[ArgKeys::index];

        if (id.isEmpty() || pluginId.isEmpty() || ! (indexVar.isInt() || indexVar.isInt64() || indexVar.isDouble()))
            report (host, "Plug-in move needs a track, a plug-in and an index");
        else if (! rack.move (id, pluginId, (int) indexVar))
            report (host, "Couldn't move the plug-in");
    });

    registry.add ({ "plugin.setBypassed", "Bypass Plug-in" }, [&rack] (const juce::var& args)
    {
        rack.setBypassed (args[ArgKeys::trackId].toString(), args[ArgKeys::pluginId].toString(),
                          (bool) args[ArgKeys::bypassed]);
    });

    registry.add ({ "plugin.moveToDeviceChain", "Move to Track Chain" }, [&rack, &host] (const juce::var& args)
    {
        const auto result = rack.moveToDeviceChain (args[ArgKeys::trackId].toString(), args[ArgKeys::pluginId].toString());
        host.report (result);

        if (result.wasOk() && host.notify)
            host.notify ("Moved to the track chain", true);
    });

    registry.add ({ "plugin.copyInsert", "Copy Insert" }, [&rack, &host] (const juce::var& args)
    {
        host.report (rack.copyInsert (args[ArgKeys::trackId].toString(), args[ArgKeys::pluginId].toString(),
                                      args[ArgKeys::toTrackId].toString(), (int) args[ArgKeys::index]));
    });

    registry.add ({ "plugin.setParameter", "Change Parameter" }, [&rack, &host] (const juce::var& args)
    {
        const auto value = args[ArgKeys::value];

        // A missing value would otherwise read as 0 and zero the parameter.
        if (! (value.isInt() || value.isInt64() || value.isDouble()))
            report (host, "Parameter change needs a value");
        else
            rack.setParameter (args[ArgKeys::pluginId].toString(), args[ArgKeys::parameterId].toString(),
                               (float) value, (bool) args[ArgKeys::continuesGesture]);
    });

    registry.add ({ "plugin.replace", "Replace Plug-in" }, [&rack, &host] (const juce::var& args)
    {
        host.report (rack.replace (args[ArgKeys::trackId].toString(), args[ArgKeys::pluginId].toString(),
                                   args[ArgKeys::plugin].toString()));
    });
}

juce::var pluginInsertArgs (const juce::String& trackId, const juce::String& plugin, PluginChain chain)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::trackId, trackId);
    args->setProperty (ArgKeys::plugin, plugin);
    args->setProperty (ArgKeys::chain, chain == PluginChain::mixer ? "mixer" : "device");
    return args;
}

juce::var pluginArgs (const juce::String& trackId, const juce::String& pluginId)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::trackId, trackId);
    args->setProperty (ArgKeys::pluginId, pluginId);
    return args;
}

juce::var pluginMoveArgs (const juce::String& trackId, const juce::String& pluginId, int index)
{
    auto args = pluginArgs (trackId, pluginId);
    args.getDynamicObject()->setProperty (ArgKeys::index, index);
    return args;
}

juce::var pluginBypassArgs (const juce::String& trackId, const juce::String& pluginId, bool bypassed)
{
    auto args = pluginArgs (trackId, pluginId);
    args.getDynamicObject()->setProperty (ArgKeys::bypassed, bypassed);
    return args;
}

juce::var pluginCopyArgs (const juce::String& fromTrackId, const juce::String& pluginId,
                          const juce::String& toTrackId, int index)
{
    auto args = pluginMoveArgs (fromTrackId, pluginId, index);
    args.getDynamicObject()->setProperty (ArgKeys::toTrackId, toTrackId);
    return args;
}

juce::var pluginParameterArgs (const juce::String& pluginId, const juce::String& parameterId, float value,
                               bool continuesGesture)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::pluginId, pluginId);
    args->setProperty (ArgKeys::parameterId, parameterId);
    args->setProperty (ArgKeys::value, value);
    args->setProperty (ArgKeys::continuesGesture, continuesGesture);
    return args;
}

juce::var pluginReplaceArgs (const juce::String& trackId, const juce::String& pluginId, const juce::String& plugin)
{
    auto args = pluginArgs (trackId, pluginId);
    args.getDynamicObject()->setProperty (ArgKeys::plugin, plugin);
    return args;
}

} // namespace resamper
