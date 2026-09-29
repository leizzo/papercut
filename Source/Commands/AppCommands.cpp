#include "AppCommands.h"

namespace resamper
{

void registerAppCommands (CommandRegistry& registry, ApplicationModel& model, AppCommandHost& host)
{
    registerProjectCommands (registry, model, host);
    registerTrackCommands (registry, model);
    registerClipCommands (registry, model, host);
    registerNoteCommands (registry, model);
    registerEditCommands (registry, model);
    registerTransportCommands (registry, model, host);
}

} // namespace resamper
