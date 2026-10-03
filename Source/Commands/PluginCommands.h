#pragma once

#include "CommandRegistry.h"
#include "Engine/PluginHosting.h"
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

/** Several parameters of one plug-in set as one change (an EQ node's
    frequency and gain); continuesGesture joins a drag into one undo step. */
struct PluginParametersArgs
{
    juce::String pluginId;
    std::vector<ParameterValue> values;
    bool continuesGesture = false;
};

/** Plays one EQ Eight band alone (band -1: stops). Monitoring, not an Edit change. */
struct PluginAuditionArgs
{
    juce::String pluginId;
    int band = -1;
};

/** A new plug-in in pluginId's place. plugin is a type name or a catalogue path. */
struct PluginReplaceArgs
{
    juce::String trackId;
    juce::String pluginId;
    juce::String plugin;
};

/** Pins a plug-in parameter to its card, or unpins it. */
struct PluginPinArgs
{
    juce::String pluginId;
    juce::String parameterId;
    bool pinned = true;
};

/** A native device card's new size. */
struct PluginSizeArgs
{
    juce::String pluginId;
    DeviceSize size = DeviceSize::compact;
};

/** A plug-in window's new state: where it is, pinned, open, its UI scale. */
struct PluginWindowArgs
{
    juce::String pluginId;
    PluginWindowState window;
};

/** A preset of the plug-in's menu, by index of PluginRack::getPresetNames. */
struct PluginPresetArgs
{
    juce::String pluginId;
    int index = 0;
};

/** The current state of a plug-in, saved as a named preset. */
struct PluginSavePresetArgs
{
    juce::String pluginId;
    juce::String name;
};

/** An A/B compare slot: 0 = A, 1 = B. */
struct PluginABArgs
{
    juce::String pluginId;
    int slot = 0;
};

/** Whether a plug-in runs in its sandbox (out of process) or in-process. */
struct PluginSandboxArgs
{
    juce::String pluginId;
    bool sandboxed = true;
};

/** A catalogue plug-in, by its path (what plugin.insert takes). */
struct PluginPathArgs
{
    juce::String plugin;
};

namespace cmd
{
    inline constexpr CommandRef<> pluginScan { "plugin.scan" };
    inline constexpr CommandRef<PluginPathArgs> pluginRetryScan { "plugin.retryScan" };   ///< one that failed to scan
    inline constexpr CommandRef<PluginInsertArgs> pluginInsert { "plugin.insert" };
    inline constexpr CommandRef<PluginArgs> pluginRemove { "plugin.remove" };
    inline constexpr CommandRef<PluginMoveArgs> pluginMove { "plugin.move" };
    inline constexpr CommandRef<PluginBypassArgs> pluginSetBypassed { "plugin.setBypassed" };
    inline constexpr CommandRef<PluginArgs> pluginMoveToDeviceChain { "plugin.moveToDeviceChain" };   ///< a mixer insert
    inline constexpr CommandRef<PluginCopyArgs> pluginCopyInsert { "plugin.copyInsert" };
    inline constexpr CommandRef<PluginParameterArgs> pluginSetParameter { "plugin.setParameter" };
    inline constexpr CommandRef<PluginParametersArgs> pluginSetParameters { "plugin.setParameters" };
    inline constexpr CommandRef<PluginAuditionArgs> pluginAudition { "plugin.audition" };   ///< never undoable
    inline constexpr CommandRef<PluginReplaceArgs> pluginReplace { "plugin.replace" };
    inline constexpr CommandRef<PluginPinArgs> pluginSetPinned { "plugin.setPinned" };
    inline constexpr CommandRef<PluginSizeArgs> pluginSetSize { "plugin.setSize" };       ///< a view: never undoable
    inline constexpr CommandRef<PluginArgs> pluginLocate { "plugin.locate" };             ///< asks for the missing plug-in's file
    inline constexpr CommandRef<PluginArgs> pluginUndoInsert { "plugin.undoInsert" };     ///< the "added" toast's Undo
    inline constexpr CommandRef<PluginWindowArgs> pluginSetWindow { "plugin.setWindow" }; ///< a view: never undoable
    inline constexpr CommandRef<PluginArgs> pluginReload { "plugin.reload" };             ///< Retry; a crashed plug-in's Reload
    inline constexpr CommandRef<PluginSandboxArgs> pluginSetSandboxed { "plugin.setSandboxed" };  ///< Run in-process; never undoable
    inline constexpr CommandRef<PluginPresetArgs> pluginSelectPreset { "plugin.selectPreset" };
    inline constexpr CommandRef<PluginSavePresetArgs> pluginSavePreset { "plugin.savePreset" };
    inline constexpr CommandRef<PluginABArgs> pluginSelectAB { "plugin.selectAB" };
    inline constexpr CommandRef<PluginArgs> pluginCopyAToB { "plugin.copyAToB" };
}

/** Registers the plug-in Commands above. plugin.insert and plugin.replace
    tell AppCommandHost::pluginAdded about the plug-in they add: the plug-in
    window's opening rule (PRD §9.6) hangs off that one place, whichever view
    the insert came from. plugin.reload and plugin.setSandboxed go to Plug-in Hosting. */
void registerPluginCommands (CommandRegistry&, PluginRack&, PluginHosting&, AppCommandHost&);

} // namespace resamper
