#pragma once

#include "CommandRegistry.h"
#include "Engine/PluginRack.h"

namespace papercut
{

struct AppCommandHost;

/** Registers the plug-in Commands:

    plugin.scan
    plugin.insert             args: trackId, plugin (a type name or catalogue path), chain ("device" | "mixer")
    plugin.remove             args: trackId, pluginId
    plugin.move               args: trackId, pluginId, index (within the plug-in's chain)
    plugin.setBypassed        args: trackId, pluginId, bypassed
    plugin.moveToDeviceChain  args: trackId, pluginId (a mixer insert)
    plugin.copyInsert         args: trackId, pluginId, toTrackId, index (on toTrackId's mixer chain)
    plugin.setParameter       args: pluginId, parameterId, value, continuesGesture
    plugin.replace            args: trackId, pluginId, plugin (a type name or catalogue path)
*/
void registerPluginCommands (CommandRegistry&, PluginRack&, AppCommandHost&);

/** Arguments for plugin.insert: at the end of that chain of the track. */
juce::var pluginInsertArgs (const juce::String& trackId, const juce::String& plugin,
                            PluginChain = PluginChain::device);

/** Arguments naming one plug-in on a track: plugin.remove, plugin.moveToDeviceChain. */
juce::var pluginArgs (const juce::String& trackId, const juce::String& pluginId);

/** Arguments for plugin.move. index is within the plug-in's own chain. */
juce::var pluginMoveArgs (const juce::String& trackId, const juce::String& pluginId, int index);

/** Arguments for plugin.setBypassed. */
juce::var pluginBypassArgs (const juce::String& trackId, const juce::String& pluginId, bool bypassed);

/** Arguments for plugin.setParameter; continuesGesture joins a knob drag into one undo step. */
juce::var pluginParameterArgs (const juce::String& pluginId, const juce::String& parameterId, float value,
                               bool continuesGesture = false);

/** Arguments for plugin.replace: a new plug-in in pluginId's place. */
juce::var pluginReplaceArgs (const juce::String& trackId, const juce::String& pluginId, const juce::String& plugin);

/** Arguments for plugin.copyInsert. */
juce::var pluginCopyArgs (const juce::String& fromTrackId, const juce::String& pluginId,
                          const juce::String& toTrackId, int index);

} // namespace papercut
