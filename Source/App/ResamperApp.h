#pragma once

#include "Commands/AppCommands.h"
#include "Commands/CommandRegistry.h"
#include "Engine/ApplicationModel.h"
#include "Engine/Automation.h"
#include "Engine/EngineManager.h"
#include "Engine/Mixer.h"
#include "Engine/PluginRack.h"
#include "Engine/Production.h"
#include "Engine/ProjectManager.h"
#include "Engine/SamplePreview.h"
#include "Engine/Session.h"
#include "Engine/Shaper.h"
#include "UI/State/UIStateStore.h"

namespace resamper
{

class ThemeManager;

/** The headless application: every engine facade over one Project, the UI
    State store, and every facade's Commands registered in one registry. The
    app, the tests and the snapshots all build this; the window takes it whole
    and adds only the Commands of its views.

    The audio device is the seam: the EngineManager passed in opens one (the
    app) or none (tests). The platform fills in the host's file choosers and
    messages; UI State capture and restore are wired to uiState. */
struct ResamperApp
{
    /** Opens an untitled Project on the engine and registers every facade's
        Commands. The theme is the one file.useTheme switches; the caller
        loads it and keeps it alive. */
    ResamperApp (EngineManager& engine, ThemeManager& theme);

    EngineManager& engine;
    ThemeManager& theme;

    ProjectManager projects { engine };
    ApplicationModel model { projects };
    Production production { projects };
    PluginRack plugins { projects };
    Mixer mixer { projects };
    Session session { projects };
    Automation automation { projects };
    Shaper shaper { projects };
    SamplePreview preview { engine };

    UIStateStore uiState;
    AppCommandHost host;
    CommandRegistry commands;   // last: its Commands refer to everything above

    JUCE_DECLARE_NON_COPYABLE (ResamperApp)
};

} // namespace resamper
