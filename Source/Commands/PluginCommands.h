#pragma once

#include "CommandRegistry.h"
#include "Engine/PluginRack.h"

namespace resamper
{

struct AppCommandHost;

/** A new plug-in at the end of a track's chain. plugin is a type name or a catalogue path. */
struct PluginInsertArgs
{
    juce::String trackId;
    juce::String plugin;
    PluginChain chain = PluginChain::device;
};

/** Names one plug-in on a track. */
struct PluginArgs
{
    juce::String trackId;
    juce::String pluginId;
};

/** A plug-in's new index within its own chain. */
struct PluginMoveArgs
{
    juce::String trackId;
    juce::String pluginId;
    int index = 0;
};

/** Bypasses a plug-in, or brings it back. */
struct PluginBypassArgs
{
    juce::String trackId;
    juce::String pluginId;
    bool bypassed = false;
};

/** A copy of a plug-in onto toTrackId's mixer chain, at index. */
struct PluginCopyArgs
{
    juce::String fromTrackId;
    juce::String pluginId;
    juce::String toTrackId;
    int index = 0;
};

/** continuesGesture joins a knob drag into one undo step. */
struct PluginParameterArgs
{
    juce::String pluginId;
    juce::String parameterId;
    float value = 0;
    bool continuesGesture = false;
};

/** A new plug-in in pluginId's place. plugin is a type name or a catalogue path. */
struct PluginReplaceArgs
{
    juce::String trackId;
    juce::String pluginId;
    juce::String plugin;
};

namespace cmd
{
    inline constexpr CommandRef<> pluginScan { "plugin.scan" };
    inline constexpr CommandRef<PluginInsertArgs> pluginInsert { "plugin.insert" };
    inline constexpr CommandRef<PluginArgs> pluginRemove { "plugin.remove" };
    inline constexpr CommandRef<PluginMoveArgs> pluginMove { "plugin.move" };
    inline constexpr CommandRef<PluginBypassArgs> pluginSetBypassed { "plugin.setBypassed" };
    inline constexpr CommandRef<PluginArgs> pluginMoveToDeviceChain { "plugin.moveToDeviceChain" };   ///< a mixer insert
    inline constexpr CommandRef<PluginCopyArgs> pluginCopyInsert { "plugin.copyInsert" };
    inline constexpr CommandRef<PluginParameterArgs> pluginSetParameter { "plugin.setParameter" };
    inline constexpr CommandRef<PluginReplaceArgs> pluginReplace { "plugin.replace" };
}

/** Registers the plug-in Commands above. */
void registerPluginCommands (CommandRegistry&, PluginRack&, AppCommandHost&);

} // namespace resamper
