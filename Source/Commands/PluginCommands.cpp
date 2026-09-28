#include "PluginCommands.h"

#include "AppCommands.h"
#include "Engine/PluginRack.h"

namespace papercut
{

namespace ArgKeys
{
    const juce::Identifier trackId ("trackId"), plugin ("plugin"), pluginId ("pluginId"), index ("index"),
                           chain ("chain"), bypassed ("bypassed"), toTrackId ("toTrackId"),
                           parameterId ("parameterId"), value ("value"), continuesGesture ("continuesGesture");
}

namespace
{
    class PluginCommand : public Command
    {
    public:
        PluginCommand (juce::String commandId, juce::String name, PluginRack& r, AppCommandHost& h)
            : Command (std::move (commandId), std::move (name)), rack (r), host (h) {}

    protected:
        PluginRack& rack;
        AppCommandHost& host;

        void report (const juce::String& message) const
        {
            if (message.isNotEmpty() && host.reportError)
                host.reportError (message);
        }

        void report (const juce::Result& result) const
        {
            if (result.failed())
                report (result.getErrorMessage());
        }
    };

    struct ScanPluginsCommand : PluginCommand
    {
        ScanPluginsCommand (PluginRack& r, AppCommandHost& h) : PluginCommand ("plugin.scan", "Scan Plug-ins", r, h) {}

        void execute (const juce::var&) override { rack.startScan(); }
        bool isEnabled() const override          { return ! rack.isScanning(); }
    };

    struct InsertPluginCommand : PluginCommand
    {
        InsertPluginCommand (PluginRack& r, AppCommandHost& h) : PluginCommand ("plugin.insert", "Insert Plug-in", r, h) {}

        void execute (const juce::var& args) override
        {
            const auto id = args[ArgKeys::trackId].toString();
            const auto plugin = args[ArgKeys::plugin].toString();
            const auto chain = args[ArgKeys::chain].toString() == "mixer" ? PluginChain::mixer : PluginChain::device;

            if (id.isEmpty() || plugin.isEmpty())
                report ("Plug-in insert needs a track and a plug-in");
            else
                report (rack.insert (id, plugin, chain));
        }
    };

    struct RemovePluginCommand : PluginCommand
    {
        RemovePluginCommand (PluginRack& r, AppCommandHost& h) : PluginCommand ("plugin.remove", "Remove Plug-in", r, h) {}

        void execute (const juce::var& args) override
        {
            const auto id = args[ArgKeys::trackId].toString();
            const auto pluginId = args[ArgKeys::pluginId].toString();

            if (id.isEmpty() || pluginId.isEmpty())
                report ("Plug-in remove needs a track and a plug-in");
            else if (! rack.remove (id, pluginId))
                report ("Couldn't remove the plug-in");
        }
    };

    struct MovePluginCommand : PluginCommand
    {
        MovePluginCommand (PluginRack& r, AppCommandHost& h) : PluginCommand ("plugin.move", "Move Plug-in", r, h) {}

        void execute (const juce::var& args) override
        {
            const auto id = args[ArgKeys::trackId].toString();
            const auto pluginId = args[ArgKeys::pluginId].toString();
            const auto indexVar = args[ArgKeys::index];

            if (id.isEmpty() || pluginId.isEmpty() || ! (indexVar.isInt() || indexVar.isInt64() || indexVar.isDouble()))
                report ("Plug-in move needs a track, a plug-in and an index");
            else if (! rack.move (id, pluginId, (int) indexVar))
                report ("Couldn't move the plug-in");
        }
    };

    struct SetBypassedCommand : PluginCommand
    {
        SetBypassedCommand (PluginRack& r, AppCommandHost& h) : PluginCommand ("plugin.setBypassed", "Bypass Plug-in", r, h) {}

        void execute (const juce::var& args) override
        {
            rack.setBypassed (args[ArgKeys::trackId].toString(), args[ArgKeys::pluginId].toString(),
                              (bool) args[ArgKeys::bypassed]);
        }
    };

    struct MoveToDeviceChainCommand : PluginCommand
    {
        MoveToDeviceChainCommand (PluginRack& r, AppCommandHost& h)
            : PluginCommand ("plugin.moveToDeviceChain", "Move to Track Chain", r, h) {}

        void execute (const juce::var& args) override
        {
            const auto result = rack.moveToDeviceChain (args[ArgKeys::trackId].toString(), args[ArgKeys::pluginId].toString());
            report (result);

            if (result.wasOk() && host.notify)
                host.notify ("Moved to the track chain", true);
        }
    };

    struct ReplacePluginCommand : PluginCommand
    {
        ReplacePluginCommand (PluginRack& r, AppCommandHost& h) : PluginCommand ("plugin.replace", "Replace Plug-in", r, h) {}

        void execute (const juce::var& args) override
        {
            report (rack.replace (args[ArgKeys::trackId].toString(), args[ArgKeys::pluginId].toString(),
                                  args[ArgKeys::plugin].toString()));
        }
    };

    struct SetParameterCommand : PluginCommand
    {
        SetParameterCommand (PluginRack& r, AppCommandHost& h) : PluginCommand ("plugin.setParameter", "Change Parameter", r, h) {}

        void execute (const juce::var& args) override
        {
            rack.setParameter (args[ArgKeys::pluginId].toString(), args[ArgKeys::parameterId].toString(),
                               (float) args[ArgKeys::value], (bool) args[ArgKeys::continuesGesture]);
        }
    };

    struct CopyInsertCommand : PluginCommand
    {
        CopyInsertCommand (PluginRack& r, AppCommandHost& h) : PluginCommand ("plugin.copyInsert", "Copy Insert", r, h) {}

        void execute (const juce::var& args) override
        {
            report (rack.copyInsert (args[ArgKeys::trackId].toString(), args[ArgKeys::pluginId].toString(),
                                     args[ArgKeys::toTrackId].toString(), (int) args[ArgKeys::index]));
        }
    };
}

void registerPluginCommands (CommandRegistry& registry, PluginRack& rack, AppCommandHost& host)
{
    registry.add (std::make_unique<ScanPluginsCommand> (rack, host));
    registry.add (std::make_unique<InsertPluginCommand> (rack, host));
    registry.add (std::make_unique<RemovePluginCommand> (rack, host));
    registry.add (std::make_unique<MovePluginCommand> (rack, host));
    registry.add (std::make_unique<SetBypassedCommand> (rack, host));
    registry.add (std::make_unique<MoveToDeviceChainCommand> (rack, host));
    registry.add (std::make_unique<CopyInsertCommand> (rack, host));
    registry.add (std::make_unique<SetParameterCommand> (rack, host));
    registry.add (std::make_unique<ReplacePluginCommand> (rack, host));
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

} // namespace papercut
