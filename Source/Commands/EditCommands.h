#pragma once

#include "CommandRegistry.h"
#include "Engine/ApplicationModel.h"

namespace resamper
{

namespace cmd
{
    inline constexpr CommandRef<> editUndo { "edit.undo" };
    inline constexpr CommandRef<> editRedo { "edit.redo" };
    inline constexpr CommandRef<> editDelete { "edit.delete" };           ///< the selected notes, else the selected clips
    inline constexpr CommandRef<> editDeselectAll { "edit.deselectAll" };
}

/** Registers the edit Commands above. */
void registerEditCommands (CommandRegistry&, ApplicationModel&);

} // namespace resamper
