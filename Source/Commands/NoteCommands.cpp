#include "NoteCommands.h"
#include "AppCommands.h"
#include "ArgKeys.h"

namespace resamper
{

void registerNoteCommands (CommandRegistry& registry, ApplicationModel& model, AppCommandHost&)
{
    registry.add ({ "note.add", "Add Note" }, [&model] (const juce::var& args)
    {
        auto pitch = args[ArgKeys::pitch];
        auto velocity = args[ArgKeys::velocity];

        if (! args[ArgKeys::start].isDouble() || ! args[ArgKeys::length].isDouble()
            || ! (pitch.isInt() || pitch.isDouble())
            || ! (velocity.isVoid() || velocity.isInt() || velocity.isDouble()))
            return;

        const int vel = velocity.isVoid() ? ApplicationModel::defaultNoteVelocity : (int) velocity;
        model.addNote (args[ArgKeys::clipId].toString(), args[ArgKeys::start], args[ArgKeys::length], (int) pitch, vel);
    });

    registry.add ({ "note.delete", "Delete Notes", [&model] { return model.hasSelectedNotes(); } },
                  [&model] { model.deleteSelectedNotes(); });

    // A drag of one or more notes in the Piano Roll.
    registry.add ({ "note.move", "Move Notes" }, [&model] (const juce::var& args)
    {
        if (! args[ArgKeys::deltaSeconds].isDouble()
            || ! (args[ArgKeys::deltaPitch].isInt() || args[ArgKeys::deltaPitch].isDouble()))
            return;

        juce::StringArray ids;

        if (auto* list = args[ArgKeys::noteIds].getArray())
            for (auto& id : *list)
                ids.add (id.toString());

        model.moveNotes (args[ArgKeys::clipId].toString(), ids, args[ArgKeys::deltaSeconds], (int) args[ArgKeys::deltaPitch]);
    });

    // A drag on a note's edge in the Piano Roll.
    registry.add ({ "note.resize", "Resize Note" }, [&model] (const juce::var& args)
    {
        if (args[ArgKeys::start].isDouble() && args[ArgKeys::end].isDouble())
            model.resizeNote (args[ArgKeys::clipId].toString(), args[ArgKeys::noteId].toString(),
                              args[ArgKeys::start], args[ArgKeys::end]);
    });

    // A drag in the velocity lane.
    registry.add ({ "note.setVelocity", "Set Note Velocity" }, [&model] (const juce::var& args)
    {
        if (args[ArgKeys::velocity].isInt() || args[ArgKeys::velocity].isDouble())
            model.setNoteVelocity (args[ArgKeys::clipId].toString(), (int) args[ArgKeys::velocity],
                                   args[ArgKeys::continuesGesture]);
    });

    registry.add ({ "note.quantize", "Quantize Notes" }, [&model] (const juce::var& args)
    {
        auto grid = args[ArgKeys::grid].toString();

        if (grid.isNotEmpty())
            model.quantizeNotes (args[ArgKeys::clipId].toString(), grid);
    });

    // Up / Down (Shift: octave) in the Piano Roll.
    registry.add ({ "note.transposeSelected", "Transpose" }, [&model] (const juce::var& args)
    {
        const auto clipId = args[ArgKeys::clipId].toString();
        juce::StringArray selected;

        for (auto& track : model.getTracks())
            for (auto& clip : track.clips)
                if (clip.id == clipId)
                    for (auto& note : clip.notes)
                        if (note.selected)
                            selected.add (note.id);

        if (! selected.isEmpty())
            model.moveNotes (clipId, selected, 0.0, (int) args[ArgKeys::argument]);
    });

    // Mod+A in the Piano Roll. Never undoable.
    registry.add ({ "note.selectAll", "Select All Notes" }, [&model] (const juce::var& args)
    {
        juce::StringArray ids;

        for (auto& track : model.getTracks())
            for (auto& clip : track.clips)
                if (clip.id == args[ArgKeys::clipId].toString())
                    for (auto& note : clip.notes)
                        ids.add (note.id);

        model.selectNotes (ids);
    });
}

juce::var noteAddArgs (const juce::String& clipId, double startSeconds, double lengthSeconds, int pitch, int velocity)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::clipId, clipId);
    args->setProperty (ArgKeys::start, startSeconds);
    args->setProperty (ArgKeys::length, lengthSeconds);
    args->setProperty (ArgKeys::pitch, pitch);
    args->setProperty (ArgKeys::velocity, velocity);
    return args;
}

juce::var noteMoveArgs (const juce::String& clipId, const juce::StringArray& noteIds, double deltaSeconds, int deltaPitch)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::clipId, clipId);
    juce::Array<juce::var> ids;

    for (auto& id : noteIds)
        ids.add (id);

    args->setProperty (ArgKeys::noteIds, juce::var (ids));
    args->setProperty (ArgKeys::deltaSeconds, deltaSeconds);
    args->setProperty (ArgKeys::deltaPitch, deltaPitch);
    return args;
}

juce::var noteResizeArgs (const juce::String& clipId, const juce::String& noteId, double startSeconds, double endSeconds)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::clipId, clipId);
    args->setProperty (ArgKeys::noteId, noteId);
    args->setProperty (ArgKeys::start, startSeconds);
    args->setProperty (ArgKeys::end, endSeconds);
    return args;
}

juce::var noteVelocityArgs (const juce::String& clipId, int velocity, bool continuesGesture)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::clipId, clipId);
    args->setProperty (ArgKeys::velocity, velocity);
    args->setProperty (ArgKeys::continuesGesture, continuesGesture);
    return args;
}

juce::var noteQuantizeArgs (const juce::String& clipId, const juce::String& grid)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::clipId, clipId);
    args->setProperty (ArgKeys::grid, grid);
    return args;
}

} // namespace resamper
