#pragma once

#include "CommandRegistry.h"
#include "Engine/ApplicationModel.h"

namespace resamper
{

/** Registers the note Commands:

    note.add     note.delete   note.move     note.resize   note.setVelocity   note.quantize
    note.transposeSelected (args: clipId, argument = semitones)   note.selectAll (args: clipId)
*/
void registerNoteCommands (CommandRegistry&, ApplicationModel&);

/** Arguments for note.add. Times are seconds from the clip's start. Velocity
    defaults to ApplicationModel::defaultNoteVelocity. */
juce::var noteAddArgs (const juce::String& clipId, double startSeconds, double lengthSeconds, int pitch,
                       int velocity = ApplicationModel::defaultNoteVelocity);

/** Arguments for note.move: every named note shifts by the same amount. */
juce::var noteMoveArgs (const juce::String& clipId, const juce::StringArray& noteIds, double deltaSeconds, int deltaPitch);

/** Arguments for note.resize: one note's new edges, seconds from the clip's start. */
juce::var noteResizeArgs (const juce::String& clipId, const juce::String& noteId, double startSeconds, double endSeconds);

/** Arguments for note.setVelocity. continuesGesture joins a drag into one undo step. */
juce::var noteVelocityArgs (const juce::String& clipId, int velocity, bool continuesGesture = false);

/** Arguments for note.quantize. grid is "1/4", "1/8" or "1/16". */
juce::var noteQuantizeArgs (const juce::String& clipId, const juce::String& grid);

} // namespace resamper
