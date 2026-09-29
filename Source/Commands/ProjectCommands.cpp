#include "ProjectCommands.h"
#include "AppCommands.h"

namespace resamper
{

void registerProjectCommands (CommandRegistry& registry, ApplicationModel& model, AppCommandHost& host)
{
    registry.add ({ "project.new", "New Project" }, [&model, &host]
    {
        model.newProject();
        host.restoreUIState ({});
    });

    registry.add ({ "project.open", "Open Project..." }, [&model, &host]
    {
        host.chooseProjectToOpen ([&model, &host] (const juce::File& folder)
        {
            juce::var savedUIState;
            auto r = model.openProject (folder, savedUIState);

            if (r.wasOk())
                host.restoreUIState (savedUIState);

            host.report (r);
        });
    });

    registry.add ({ "project.saveAs", "Save Project As..." }, [&model, &host]
    {
        host.chooseProjectSaveLocation ([&model, &host] (const juce::File& folder)
        {
            host.report (model.saveProjectAs (folder, host.captureUIState()));
        });
    });

    // An untitled Project has nowhere to save yet, so Save asks where, as Save As does.
    registry.add ({ "project.save", "Save Project" }, [&model, &host, &registry]
    {
        if (model.isProjectUntitled())
            registry.invoke ("project.saveAs");
        else
            host.report (model.saveProject (host.captureUIState()));
    });
}

} // namespace resamper
