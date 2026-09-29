#include "ResamperApp.h"
#include "Commands/AutomationCommands.h"
#include "Commands/MixerCommands.h"
#include "Commands/PluginCommands.h"
#include "Commands/ProductionCommands.h"
#include "Commands/SessionCommands.h"

namespace resamper
{

ResamperApp::ResamperApp (EngineManager& e, ThemeManager& t)
    : engine (e), theme (t)
{
    host.captureUIState = [this] { return uiState.toVar(); };
    host.restoreUIState = [this] (const juce::var& v) { uiState.restore (v); };

    registerAppCommands (commands, model, host);
    registerProductionCommands (commands, production, model, theme, host);
    registerPluginCommands (commands, plugins, host);
    registerMixerCommands (commands, mixer, host);
    registerSessionCommands (commands, session, host);
    registerAutomationCommands (commands, automation, shaper, host);
}

} // namespace resamper
