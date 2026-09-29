#pragma once

#include "AppCommandHost.h"
#include "ClipCommands.h"
#include "CommandRegistry.h"
#include "EditCommands.h"
#include "Engine/ApplicationModel.h"
#include "NoteCommands.h"
#include "ProjectCommands.h"
#include "TrackCommands.h"
#include "TransportCommands.h"


namespace resamper
{

/** Registers every model-facing Command: the project, track, clip, note,
    edit and transport Commands (see each area's header). */
void registerAppCommands (CommandRegistry&, ApplicationModel&, AppCommandHost&);

} // namespace resamper
