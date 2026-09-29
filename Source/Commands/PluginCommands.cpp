#include "PluginCommands.h"

#include "AppCommandHost.h"
#include "Engine/PluginRack.h"

namespace resamper
{

namespace
{
    /** Reports a message that isn't from a Result. */
    void reportMessage (const AppCommandHost& host, const juce::String& message)
    {
        if (host.reportError)
            host.reportError (message);
    }
}

void registerPluginCommands (CommandRegistry& registry, PluginRack& rack, AppCommandHost& host)
{
    registry.add (cmd::pluginScan, { "Scan Plug-ins", [&rack] { return ! rack.isScanning(); } }, [&rack] { rack.startScan(); });

    registry.add (cmd::pluginInsert, { "Insert Plug-in" }, [&rack, &host] (const PluginInsertArgs& a)
    {
        if (a.trackId.isEmpty() || a.plugin.isEmpty())
            reportMessage (host, "Plug-in insert needs a track and a plug-in");
        else
            host.report (rack.insert (a.trackId, a.plugin, a.chain));
    });

    registry.add (cmd::pluginRemove, { "Remove Plug-in" }, [&rack, &host] (const PluginArgs& a)
    {
        if (a.trackId.isEmpty() || a.pluginId.isEmpty())
            reportMessage (host, "Plug-in remove needs a track and a plug-in");
        else if (! rack.remove (a.trackId, a.pluginId))
            reportMessage (host, "Couldn't remove the plug-in");
    });

    registry.add (cmd::pluginMove, { "Move Plug-in" }, [&rack, &host] (const PluginMoveArgs& a)
    {
        if (a.trackId.isEmpty() || a.pluginId.isEmpty())
            reportMessage (host, "Plug-in move needs a track and a plug-in");
        else if (! rack.move (a.trackId, a.pluginId, a.index))
            reportMessage (host, "Couldn't move the plug-in");
    });

    registry.add (cmd::pluginSetBypassed, { "Bypass Plug-in" }, [&rack] (const PluginBypassArgs& a)
    {
        rack.setBypassed (a.trackId, a.pluginId, a.bypassed);
    });

    registry.add (cmd::pluginMoveToDeviceChain, { "Move to Track Chain" }, [&rack, &host] (const PluginArgs& a)
    {
        const auto result = rack.moveToDeviceChain (a.trackId, a.pluginId);
        host.report (result);

        if (result.wasOk() && host.notify)
            host.notify ("Moved to the track chain", true);
    });

    registry.add (cmd::pluginCopyInsert, { "Copy Insert" }, [&rack, &host] (const PluginCopyArgs& a)
    {
        host.report (rack.copyInsert (a.fromTrackId, a.pluginId, a.toTrackId, a.index));
    });

    registry.add (cmd::pluginSetParameter, { "Change Parameter" }, [&rack] (const PluginParameterArgs& a)
    {
        rack.setParameter (a.pluginId, a.parameterId, a.value, a.continuesGesture);
    });

    registry.add (cmd::pluginReplace, { "Replace Plug-in" }, [&rack, &host] (const PluginReplaceArgs& a)
    {
        host.report (rack.replace (a.trackId, a.pluginId, a.plugin));
    });
}

} // namespace resamper
