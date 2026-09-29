#pragma once

#include "CommandRegistry.h"
#include "Engine/ApplicationModel.h"

namespace resamper
{

struct AppCommandHost;

/** Registers project.new  project.open  project.save  project.saveAs. */
void registerProjectCommands (CommandRegistry&, ApplicationModel&, AppCommandHost&);

} // namespace resamper
