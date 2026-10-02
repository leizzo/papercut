#include "PluginCommands.h"

#include "AppCommandHost.h"
#include "EditCommands.h"
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

    void announceAdded (const AppCommandHost& host, const juce::String& trackId, const juce::String& pluginId)
    {
        if (host.pluginAdded && pluginId.isNotEmpty())
            host.pluginAdded (trackId, pluginId);
    }
}

void registerPluginCommands (CommandRegistry& registry, PluginRack& rack, AppCommandHost& host)
{
    registry.add (cmd::pluginScan, { "Scan Plug-ins", [&rack] { return ! rack.isScanning(); } }, [&rack] { rack.startScan(); });

    registry.add (cmd::pluginRetryScan, { "Retry Plug-in Scan" }, [&rack, &host] (const PluginPathArgs& a)
    {
        host.report (rack.retryScan (a.plugin));
    });

    registry.add (cmd::pluginInsert, { "Insert Plug-in" }, [&rack, &host] (const PluginInsertArgs& a)
    {
        if (a.trackId.isEmpty() || a.plugin.isEmpty())
        {
            reportMessage (host, "Plug-in insert needs a track and a plug-in");
            return;
        }

        juce::String added;
        const auto result = rack.insert (a.trackId, a.plugin, a.chain, &added);
        host.report (result);

        if (result.wasOk())
            announceAdded (host, a.trackId, added);
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
        juce::String added;
        const auto result = rack.replace (a.trackId, a.pluginId, a.plugin, &added);
        host.report (result);

        if (result.wasOk())
            announceAdded (host, a.trackId, added);
    });

    // Takes the plug-in back out: the insert itself undone while it is still
    // the newest step, else a removal of its own. One undo step either way.
    registry.add (cmd::pluginUndoInsert, { "Undo Insert" }, [&registry, &rack, &host] (const PluginArgs& a)
    {
        if (rack.isNewestStepInsertOf (a.pluginId))
            registry.invoke (cmd::editUndo);
        else if (rack.contains (a.pluginId) && ! rack.remove (a.trackId, a.pluginId))
            reportMessage (host, "Couldn't remove the plug-in");
    });

    registry.add (cmd::pluginSetWindow, { "Plug-in Window" }, [&rack, &host] (const PluginWindowArgs& a)
    {
        host.report (rack.setWindowState (a.pluginId, a.window));
    });

    registry.add (cmd::pluginReload, { "Reload Plug-in" }, [&rack, &host] (const PluginArgs& a)
    {
        host.report (rack.reload (a.pluginId));
    });

    registry.add (cmd::pluginSelectPreset, { "Select Preset" }, [&rack, &host] (const PluginPresetArgs& a)
    {
        host.report (rack.selectPreset (a.pluginId, a.index));
    });

    registry.add (cmd::pluginSavePreset, { "Save Preset" }, [&rack, &host] (const PluginSavePresetArgs& a)
    {
        host.report (rack.savePreset (a.pluginId, a.name));
    });

    registry.add (cmd::pluginSelectAB, { "A/B Compare" }, [&rack, &host] (const PluginABArgs& a)
    {
        host.report (rack.selectABSlot (a.pluginId, a.slot));
    });

    registry.add (cmd::pluginCopyAToB, { "Copy A to B" }, [&rack, &host] (const PluginArgs& a)
    {
        host.report (rack.copyAToB (a.pluginId));
    });

    registry.add (cmd::pluginSetPinned, { "Pin Parameter" }, [&rack, &host] (const PluginPinArgs& a)
    {
        host.report (rack.setPinned (a.pluginId, a.parameterId, a.pinned));
    });

    registry.add (cmd::pluginSetSize, { "Resize Device" }, [&rack, &host] (const PluginSizeArgs& a)
    {
        host.report (rack.setSize (a.pluginId, a.size));
    });

    registry.add (cmd::pluginLocate, { "Locate Plug-in" }, [&rack, &host] (const PluginArgs& a)
    {
        if (! host.choosePluginFile)
            return;

        host.choosePluginFile ([&rack, &host, id = a.pluginId] (const juce::File& file)
        {
            host.report (rack.locate (id, file));
        });
    });
}

} // namespace resamper
