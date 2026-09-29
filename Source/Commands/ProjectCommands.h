#pragma once

#include "CommandRegistry.h"
#include "Engine/ApplicationModel.h"

namespace resamper
{

struct AppCommandHost;

namespace cmd
{
    inline constexpr CommandRef<> projectNew { "project.new" };
    inline constexpr CommandRef<> projectOpen { "project.open" };
    inline constexpr CommandRef<> projectSave { "project.save" };
    inline constexpr CommandRef<> projectSaveAs { "project.saveAs" };
}

/** Registers the project Commands above. */
void registerProjectCommands (CommandRegistry&, ApplicationModel&, AppCommandHost&);

} // namespace resamper
