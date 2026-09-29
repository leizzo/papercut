#pragma once

#include "CommandRegistry.h"

namespace resamper
{

class Session;
struct AppCommandHost;

/** Registers Session commands. File choice for session.addSlotClip goes through
    AppCommandHost::chooseAudioFile, the same way clip.add does.

    session.setSceneCount   count
    session.renameScene     index, name
    session.addSlotClip     trackId, scene
    session.addMidiSlotClip trackId, scene
    session.clearSlot       trackId, scene
    session.launchSlot      trackId, scene
    session.launchScene     index
    session.stopAll
    session.recordToArrangement
*/
void registerSessionCommands (CommandRegistry&, Session&, AppCommandHost&);

/** Arguments for session.setSceneCount. */
juce::var sessionSceneCountArgs (int count);

/** Arguments for session.renameScene. */
juce::var sessionRenameSceneArgs (int index, const juce::String& name);

/** Arguments for session.addSlotClip, session.clearSlot and session.launchSlot. */
juce::var sessionSlotArgs (const juce::String& trackId, int sceneIndex);

/** Arguments for session.launchScene. */
juce::var sessionSceneArgs (int index);

} // namespace resamper
