#pragma once

#include "CommandRegistry.h"

namespace papercut
{

class PluginRack;
struct AppCommandHost;

/** Registers the plug-in Commands:

    plugin.scan
    plugin.insert   args: trackId, plugin (a type name or catalogue path)
    plugin.remove   args: trackId, pluginId
    plugin.move     args: trackId, pluginId, index
*/
void registerPluginCommands (CommandRegistry&, PluginRack&, AppCommandHost&);

/** Arguments for plugin.insert. */
juce::var pluginInsertArgs (const juce::String& trackId, const juce::String& plugin);

/** Arguments for plugin.remove. */
juce::var pluginRemoveArgs (const juce::String& trackId, const juce::String& pluginId);

/** Arguments for plugin.move. index is among that track's inserts. */
juce::var pluginMoveArgs (const juce::String& trackId, const juce::String& pluginId, int index);

} // namespace papercut
