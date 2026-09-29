#pragma once

#include "CommandRegistry.h"
#include "Engine/ApplicationModel.h"

namespace resamper
{

struct AppCommandHost;

/** Registers edit.undo  edit.redo  edit.delete  edit.deselectAll. */
void registerEditCommands (CommandRegistry&, ApplicationModel&, AppCommandHost&);

} // namespace resamper
