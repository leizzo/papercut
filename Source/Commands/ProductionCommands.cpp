#include "ProductionCommands.h"

#include "AppCommandHost.h"
#include "CommandRegistry.h"
#include "Engine/ApplicationModel.h"
#include "Engine/Production.h"
#include "UI/Theme/ThemeManager.h"

namespace resamper
{

namespace
{
    /** Opens a Project folder through the model, so it drops the old Edit's listener, and restores its UI State. */
    juce::Result openAndRestore (ApplicationModel& model, AppCommandHost& host, const juce::File& folder)
    {
        juce::var uiState;
        auto r = model.openProject (folder, uiState);

        if (r.wasOk() && host.restoreUIState)
            host.restoreUIState (uiState);

        return r;
    }
}

void registerProductionCommands (CommandRegistry& registry, Production& production, ApplicationModel& model,
                                 ThemeManager& themes, AppCommandHost& host)
{
    registry.add (cmd::trackFreeze, { "Freeze Track" }, [&production, &host] (const TrackArgs& a)
    {
        host.report (a.trackId.isEmpty() ? juce::Result::fail ("track.freeze needs a track") : production.freezeTrack (a.trackId));
    });

    registry.add (cmd::trackUnfreeze, { "Unfreeze Track" }, [&production, &host] (const TrackArgs& a)
    {
        host.report (a.trackId.isEmpty() ? juce::Result::fail ("track.unfreeze needs a track") : production.unfreezeTrack (a.trackId));
    });

    registry.add (cmd::trackBounce, { "Bounce Track..." }, [&production, &host] (const BounceArgs& a)
    {
        if (a.trackId.isEmpty())
        {
            host.report (juce::Result::fail ("track.bounce needs a track"));
            return;
        }

        if (a.file.isNotEmpty())
        {
            host.report (production.bounceTrack (a.trackId, a.file));
            return;
        }

        if (host.chooseProjectSaveLocation == nullptr)
        {
            host.report (juce::Result::fail ("track.bounce needs a file"));
            return;
        }

        host.chooseProjectSaveLocation ([&production, &host, id = a.trackId] (const juce::File& chosen)
        {
            host.report (production.bounceTrack (id, chosen));
        });
    });

    registry.add (cmd::fileExportMix, { "Export Mix..." }, [&production, &host] (const FileArgs& a)
    {
        if (a.file.isNotEmpty())
        {
            host.report (production.exportMix (a.file));
            return;
        }

        if (host.chooseProjectSaveLocation == nullptr)
        {
            host.report (juce::Result::fail ("file.exportMix needs a file"));
            return;
        }

        host.chooseProjectSaveLocation ([&production, &host] (const juce::File& chosen)
        {
            host.report (production.exportMix (chosen));
        });
    });

    registry.add (cmd::projectSaveTemplate, { "Save Template" }, [&production, &host] (const FolderArgs& a)
    {
        host.report (a.folder.isEmpty() ? juce::Result::fail ("project.saveTemplate needs a folder") : production.saveTemplate (a.folder));
    });

    registry.add (cmd::projectNewFromTemplate, { "New from Template" }, [&production, &model, &host] (const NewFromTemplateArgs& a)
    {
        if (a.templateFolder.isEmpty() || a.destFolder.isEmpty())
        {
            host.report (juce::Result::fail ("project.newFromTemplate needs template and dest folders"));
            return;
        }

        // Copy first, then open the copy.
        auto r = production.newFromTemplate (a.templateFolder, a.destFolder);
        host.report (r.wasOk() ? openAndRestore (model, host, a.destFolder) : r);
    });

    registry.add (cmd::projectAutosave, { "Autosave" }, [&production, &host]
    {
        host.report (production.autosave (host.captureUIState ? host.captureUIState() : juce::var()));
    });

    registry.add (cmd::projectRecover, { "Recover", [&production] { return production.hasRecovery(); } }, [&production, &model, &host]
    {
        host.report (production.hasRecovery() ? openAndRestore (model, host, production.getRecoveryFolder())
                                              : juce::Result::fail ("No recovery copy"));
    });

    registry.add (cmd::themeUse, { "Use Theme" }, [&themes, &host] (const FileArgs& a)
    {
        host.report (a.file.isEmpty() ? juce::Result::fail ("theme.use needs a file") : themes.useTheme (a.file));
    });
}

} // namespace resamper
