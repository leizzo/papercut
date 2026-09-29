#pragma once

#include "CommandRegistry.h"
#include "Engine/ApplicationModel.h"

namespace resamper
{

struct AppCommandHost;

/** Registers the clip Commands:

    clip.add     clip.insertAt  clip.addMidi  clip.move     clip.resize   clip.split   clip.setTake
    clip.copy    clip.loopExtend  clip.rename  clip.reverse  clip.setColour
    clip.duplicate  clip.consolidate  clip.delete   (these three act on the selected clips)
*/
void registerClipCommands (CommandRegistry&, ApplicationModel&, AppCommandHost&);

/** Arguments for clip.move and clip.copy: the clip, its new start, and optionally the track to
    move it to. Invoked with anything else, clip.move does nothing. */
juce::var clipMoveArgs (const juce::String& clipId, double startSeconds, const juce::String& trackId = {});

/** Arguments for clip.insertAt: an audio file dropped on a track at a position. */
juce::var clipInsertAtArgs (const juce::File&, const juce::String& trackId, double startSeconds);

/** Arguments for clip.resize and clip.loopExtend: the clip and its new edges. */
juce::var clipResizeArgs (const juce::String& clipId, double startSeconds, double endSeconds);

/** Arguments naming one clip: clip.reverse. */
juce::var clipArgs (const juce::String& clipId);

/** Arguments for clip.rename. */
juce::var clipRenameArgs (const juce::String& clipId, const juce::String& name);

/** Arguments for clip.setColour: a palette index, or -1 for the track's colour. */
juce::var clipColourArgs (const juce::String& clipId, int colourIndex);

/** Arguments for clip.setTake: the clip and a 0-based take index. */
juce::var clipTakeArgs (const juce::String& clipId, int takeIndex);

} // namespace resamper
