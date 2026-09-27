#include "PluginCommands.h"

#include "AppCommands.h"
#include "Engine/PluginRack.h"

namespace papercut
{

namespace ArgKeys
{
    const juce::Identifier trackId ("trackId"), plugin ("plugin"), pluginId ("pluginId"), index ("index");
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

            if (id.isEmpty() || plugin.isEmpty())
                report ("Plug-in insert needs a track and a plug-in");
            else
                report (rack.insert (id, plugin));
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
}

void registerPluginCommands (CommandRegistry& registry, PluginRack& rack, AppCommandHost& host)
{
    registry.add (std::make_unique<ScanPluginsCommand> (rack, host));
    registry.add (std::make_unique<InsertPluginCommand> (rack, host));
    registry.add (std::make_unique<RemovePluginCommand> (rack, host));
    registry.add (std::make_unique<MovePluginCommand> (rack, host));
}

juce::var pluginInsertArgs (const juce::String& trackId, const juce::String& plugin)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::trackId, trackId);
    args->setProperty (ArgKeys::plugin, plugin);
    return args;
}

juce::var pluginRemoveArgs (const juce::String& trackId, const juce::String& pluginId)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::trackId, trackId);
    args->setProperty (ArgKeys::pluginId, pluginId);
    return args;
}

juce::var pluginMoveArgs (const juce::String& trackId, const juce::String& pluginId, int index)
{
    auto args = pluginRemoveArgs (trackId, pluginId);
    args.getDynamicObject()->setProperty (ArgKeys::index, index);
    return args;
}

} // namespace papercut
