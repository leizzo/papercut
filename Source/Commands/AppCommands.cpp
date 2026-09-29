#include "AppCommands.h"

namespace resamper
{

void registerAppCommands (CommandRegistry& registry, ApplicationModel& model, AppCommandHost& host)
{
    registerProjectCommands (registry, model, host);
    registerTrackCommands (registry, model, host);
    registerClipCommands (registry, model, host);
    registerNoteCommands (registry, model, host);
    registerEditCommands (registry, model, host);
    registerTransportCommands (registry, model, host);
}

} // namespace resamper
