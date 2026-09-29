#include "EditCommands.h"

namespace resamper
{

void registerEditCommands (CommandRegistry& registry, ApplicationModel& model)
{
    registry.add ({ "edit.undo", "Undo", [&model] { return model.canUndo(); } }, [&model] { model.undo(); });
    registry.add ({ "edit.redo", "Redo", [&model] { return model.canRedo(); } }, [&model] { model.redo(); });

    // Delete / Backspace: the selected notes (Piano Roll), else the selected clips.
    registry.add ({ "edit.delete", "Delete",
                    [&model] { return model.hasSelectedNotes() || ! model.getSelectedClipIds().isEmpty(); } },
                  [&model, &registry]
    {
        registry.invoke (model.hasSelectedNotes() ? "note.delete" : "clip.delete");
    });

    registry.add ({ "edit.deselectAll", "Deselect All" }, [&model] { model.deselectAll(); });
}

} // namespace resamper
