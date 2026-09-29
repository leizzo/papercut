#pragma once

#include "CommandRegistry.h"

namespace resamper
{

class Session;
struct AppCommandHost;

/** A scene's new name. */
struct SceneNameArgs
{
    int index = 0;
    juce::String name;
};

/** One slot: a track's clip in a scene. */
struct SlotArgs
{
    juce::String trackId;
    int scene = 0;
};

namespace cmd
{
    inline constexpr CommandRef<int> sessionSetSceneCount { "session.setSceneCount" };
    inline constexpr CommandRef<SceneNameArgs> sessionRenameScene { "session.renameScene" };
    inline constexpr CommandRef<SlotArgs> sessionAddSlotClip { "session.addSlotClip" };    ///< asks AppCommandHost::chooseAudioFile, as clip.add does
    inline constexpr CommandRef<SlotArgs> sessionAddMidiSlotClip { "session.addMidiSlotClip" };
    inline constexpr CommandRef<SlotArgs> sessionClearSlot { "session.clearSlot" };
    inline constexpr CommandRef<SlotArgs> sessionLaunchSlot { "session.launchSlot" };
    inline constexpr CommandRef<int> sessionLaunchScene { "session.launchScene" };          ///< the scene's index
    inline constexpr CommandRef<> sessionStopAll { "session.stopAll" };
    inline constexpr CommandRef<> sessionRecordToArrangement { "session.recordToArrangement" };
}

/** Registers the Session Commands above. */
void registerSessionCommands (CommandRegistry&, Session&, AppCommandHost&);

} // namespace resamper
