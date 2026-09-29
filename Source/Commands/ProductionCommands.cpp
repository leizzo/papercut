#include "ProductionCommands.h"

#include "AppCommandHost.h"
#include "ArgKeys.h"
#include "CommandRegistry.h"
#include "Engine/ApplicationModel.h"
#include "Engine/Production.h"
#include "UI/Theme/ThemeManager.h"

namespace resamper
{

namespace
{
    juce::String trackIdFrom (const juce::var& args)    { return args[ArgKeys::trackId].toString(); }
    juce::String fileFrom (const juce::var& args)       { return args[ArgKeys::file].toString(); }

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
    registry.add ({ "track.freeze", "Freeze Track" }, [&production, &host] (const juce::var& args)
    {
        auto id = trackIdFrom (args);
        host.report (id.isEmpty() ? juce::Result::fail ("track.freeze needs a track") : production.freezeTrack (id));
    });

    registry.add ({ "track.unfreeze", "Unfreeze Track" }, [&production, &host] (const juce::var& args)
    {
        auto id = trackIdFrom (args);
        host.report (id.isEmpty() ? juce::Result::fail ("track.unfreeze needs a track") : production.unfreezeTrack (id));
    });

    registry.add ({ "track.bounce", "Bounce Track..." }, [&production, &host] (const juce::var& args)
    {
        auto id = trackIdFrom (args);

        if (id.isEmpty())
        {
            host.report (juce::Result::fail ("track.bounce needs a track"));
            return;
        }

        if (auto file = fileFrom (args); file.isNotEmpty())
        {
            host.report (production.bounceTrack (id, file));
            return;
        }

        if (host.chooseProjectSaveLocation == nullptr)
        {
            host.report (juce::Result::fail ("track.bounce needs a file"));
            return;
        }

        host.chooseProjectSaveLocation ([&production, &host, id] (const juce::File& chosen)
        {
            host.report (production.bounceTrack (id, chosen));
        });
    });

    registry.add ({ "file.exportMix", "Export Mix..." }, [&production, &host] (const juce::var& args)
    {
        if (auto file = fileFrom (args); file.isNotEmpty())
        {
            host.report (production.exportMix (file));
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

    registry.add ({ "project.saveTemplate", "Save Template" }, [&production, &host] (const juce::var& args)
    {
        auto folder = args[ArgKeys::folder].toString();
        host.report (folder.isEmpty() ? juce::Result::fail ("project.saveTemplate needs a folder") : production.saveTemplate (folder));
    });

    registry.add ({ "project.newFromTemplate", "New from Template" }, [&production, &model, &host] (const juce::var& args)
    {
        auto source = args[ArgKeys::templateFolder].toString();
        auto dest = args[ArgKeys::dest].toString();

        if (source.isEmpty() || dest.isEmpty())
        {
            host.report (juce::Result::fail ("project.newFromTemplate needs template and dest folders"));
            return;
        }

        // Copy first, then open the copy.
        auto r = production.newFromTemplate (source, dest);
        host.report (r.wasOk() ? openAndRestore (model, host, dest) : r);
    });

    registry.add ({ "project.autosave", "Autosave" }, [&production, &host]
    {
        host.report (production.autosave (host.captureUIState ? host.captureUIState() : juce::var()));
    });

    registry.add ({ "project.recover", "Recover", [&production] { return production.hasRecovery(); } }, [&production, &model, &host]
    {
        host.report (production.hasRecovery() ? openAndRestore (model, host, production.getRecoveryFolder())
                                              : juce::Result::fail ("No recovery copy"));
    });

    registry.add ({ "theme.use", "Use Theme" }, [&themes, &host] (const juce::var& args)
    {
        auto file = fileFrom (args);
        host.report (file.isEmpty() ? juce::Result::fail ("theme.use needs a file") : themes.useTheme (file));
    });
}

juce::var trackIdArgs (const juce::String& trackId)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::trackId, trackId);
    return args;
}

juce::var bounceArgs (const juce::String& trackId, const juce::String& file)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::trackId, trackId);
    args->setProperty (ArgKeys::file, file);
    return args;
}

juce::var exportMixArgs (const juce::String& file)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::file, file);
    return args;
}

juce::var saveTemplateArgs (const juce::String& folder)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::folder, folder);
    return args;
}

juce::var newFromTemplateArgs (const juce::String& templateFolder, const juce::String& destFolder)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::templateFolder, templateFolder);
    args->setProperty (ArgKeys::dest, destFolder);
    return args;
}

juce::var themeFileArgs (const juce::String& file)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::file, file);
    return args;
}

} // namespace resamper
