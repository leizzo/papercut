#include "ProductionCommands.h"

#include "AppCommands.h"
#include "CommandRegistry.h"
#include "Engine/ApplicationModel.h"
#include "Engine/Production.h"
#include "UI/Theme/ThemeManager.h"

namespace papercut
{

namespace
{
    namespace ArgKeys
    {
        const juce::Identifier trackId ("trackId"), file ("file"), folder ("folder"),
                               templateFolder ("template"), dest ("dest");
    }

    class ProductionCommand : public Command
    {
    public:
        ProductionCommand (juce::String commandId, juce::String name, Production& p, AppCommandHost& h)
            : Command (std::move (commandId), std::move (name)), production (p), host (h) {}

    protected:
        Production& production;
        AppCommandHost& host;

        void report (const juce::Result& r) const
        {
            if (r.failed() && host.reportError)
                host.reportError (r.getErrorMessage());
        }
    };

    juce::String trackIdFrom (const juce::var& args)    { return args[ArgKeys::trackId].toString(); }
    juce::String fileFrom (const juce::var& args)       { return args[ArgKeys::file].toString(); }

    struct FreezeTrackCommand : ProductionCommand
    {
        FreezeTrackCommand (Production& p, AppCommandHost& h) : ProductionCommand ("track.freeze", "Freeze Track", p, h) {}

        void execute (const juce::var& args) override
        {
            auto id = trackIdFrom (args);

            if (id.isEmpty())
                report (juce::Result::fail ("track.freeze needs a track"));
            else
                report (production.freezeTrack (id));
        }
    };

    struct UnfreezeTrackCommand : ProductionCommand
    {
        UnfreezeTrackCommand (Production& p, AppCommandHost& h) : ProductionCommand ("track.unfreeze", "Unfreeze Track", p, h) {}

        void execute (const juce::var& args) override
        {
            auto id = trackIdFrom (args);

            if (id.isEmpty())
                report (juce::Result::fail ("track.unfreeze needs a track"));
            else
                report (production.unfreezeTrack (id));
        }
    };

    struct BounceTrackCommand : ProductionCommand
    {
        BounceTrackCommand (Production& p, AppCommandHost& h) : ProductionCommand ("track.bounce", "Bounce Track...", p, h) {}

        void execute (const juce::var& args) override
        {
            auto id = trackIdFrom (args);

            if (id.isEmpty())
            {
                report (juce::Result::fail ("track.bounce needs a track"));
                return;
            }

            if (auto file = fileFrom (args); file.isNotEmpty())
            {
                report (production.bounceTrack (id, file));
                return;
            }

            if (host.chooseProjectSaveLocation == nullptr)
            {
                report (juce::Result::fail ("track.bounce needs a file"));
                return;
            }

            host.chooseProjectSaveLocation ([this, id] (const juce::File& chosen)
            {
                report (production.bounceTrack (id, chosen));
            });
        }
    };

    struct ExportMixCommand : ProductionCommand
    {
        ExportMixCommand (Production& p, AppCommandHost& h) : ProductionCommand ("file.exportMix", "Export Mix...", p, h) {}

        void execute (const juce::var& args) override
        {
            if (auto file = fileFrom (args); file.isNotEmpty())
            {
                report (production.exportMix (file));
                return;
            }

            if (host.chooseProjectSaveLocation == nullptr)
            {
                report (juce::Result::fail ("file.exportMix needs a file"));
                return;
            }

            host.chooseProjectSaveLocation ([this] (const juce::File& chosen)
            {
                report (production.exportMix (chosen));
            });
        }
    };

    struct SaveTemplateCommand : ProductionCommand
    {
        SaveTemplateCommand (Production& p, AppCommandHost& h) : ProductionCommand ("project.saveTemplate", "Save Template", p, h) {}

        void execute (const juce::var& args) override
        {
            auto folder = args[ArgKeys::folder].toString();

            if (folder.isEmpty())
                report (juce::Result::fail ("project.saveTemplate needs a folder"));
            else
                report (production.saveTemplate (folder));
        }
    };

    struct NewFromTemplateCommand : ProductionCommand
    {
        NewFromTemplateCommand (Production& p, ApplicationModel& m, AppCommandHost& h)
            : ProductionCommand ("project.newFromTemplate", "New from Template", p, h), model (m) {}

        void execute (const juce::var& args) override
        {
            auto source = args[ArgKeys::templateFolder].toString();
            auto dest = args[ArgKeys::dest].toString();

            if (source.isEmpty() || dest.isEmpty())
            {
                report (juce::Result::fail ("project.newFromTemplate needs template and dest folders"));
                return;
            }

            // Copy first, then open through the model so it drops the old Edit's listener.
            auto r = production.newFromTemplate (source, dest);
            juce::var uiState;

            if (r.wasOk())
                r = model.openProject (dest, uiState);

            if (r.wasOk() && host.restoreUIState)
                host.restoreUIState (uiState);

            report (r);
        }

        ApplicationModel& model;
    };

    struct AutosaveCommand : ProductionCommand
    {
        AutosaveCommand (Production& p, AppCommandHost& h) : ProductionCommand ("project.autosave", "Autosave", p, h) {}

        void execute (const juce::var&) override
        {
            report (production.autosave (host.captureUIState ? host.captureUIState() : juce::var()));
        }
    };

    struct RecoverCommand : ProductionCommand
    {
        RecoverCommand (Production& p, ApplicationModel& m, AppCommandHost& h)
            : ProductionCommand ("project.recover", "Recover", p, h), model (m) {}

        bool isEnabled() const override    { return production.hasRecovery(); }

        void execute (const juce::var&) override
        {
            if (! production.hasRecovery())
            {
                report (juce::Result::fail ("No recovery copy"));
                return;
            }

            juce::var uiState;
            auto r = model.openProject (production.getRecoveryFolder(), uiState);

            if (r.wasOk() && host.restoreUIState)
                host.restoreUIState (uiState);

            report (r);
        }

        ApplicationModel& model;
    };

    struct UseThemeCommand : Command
    {
        UseThemeCommand (ThemeManager& tm, AppCommandHost& h)
            : Command ("theme.use", "Use Theme"), themes (tm), host (h) {}

        void execute (const juce::var& args) override
        {
            auto file = fileFrom (args);

            auto r = file.isEmpty() ? juce::Result::fail ("theme.use needs a file")
                                    : themes.useTheme (file);

            if (r.failed() && host.reportError)
                host.reportError (r.getErrorMessage());
        }

        ThemeManager& themes;
        AppCommandHost& host;
    };
}

void registerProductionCommands (CommandRegistry& registry, Production& production, ApplicationModel& model,
                                 ThemeManager& themes, AppCommandHost& host)
{
    registry.add (std::make_unique<FreezeTrackCommand> (production, host));
    registry.add (std::make_unique<UnfreezeTrackCommand> (production, host));
    registry.add (std::make_unique<BounceTrackCommand> (production, host));
    registry.add (std::make_unique<ExportMixCommand> (production, host));
    registry.add (std::make_unique<SaveTemplateCommand> (production, host));
    registry.add (std::make_unique<NewFromTemplateCommand> (production, model, host));
    registry.add (std::make_unique<AutosaveCommand> (production, host));
    registry.add (std::make_unique<RecoverCommand> (production, model, host));
    registry.add (std::make_unique<UseThemeCommand> (themes, host));
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

} // namespace papercut
