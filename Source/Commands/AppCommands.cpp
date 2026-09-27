#include "AppCommands.h"

#include "Engine/ApplicationModel.h"

namespace papercut
{

namespace
{
    /** Base for Commands that act on the Application Model. */
    class ModelCommand : public Command
    {
    public:
        ModelCommand (juce::String id, juce::String name, ApplicationModel& m, AppCommandHost& h)
            : Command (std::move (id), std::move (name)), model (m), host (h) {}

    protected:
        ApplicationModel& model;
        AppCommandHost& host;

        void report (const juce::Result& r) const
        {
            if (r.failed() && host.reportError)
                host.reportError (r.getErrorMessage());
        }
    };

    //==============================================================================
    struct NewProjectCommand : ModelCommand
    {
        NewProjectCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("project.new", "New Project", m, h) {}

        void execute() override
        {
            model.newProject();
            host.restoreUIState ({});
        }
    };

    struct OpenProjectCommand : ModelCommand
    {
        OpenProjectCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("project.open", "Open Project...", m, h) {}

        void execute() override
        {
            host.chooseProjectToOpen ([this] (const juce::File& folder)
            {
                juce::var savedUIState;
                auto r = model.openProject (folder, savedUIState);

                if (r.wasOk())
                    host.restoreUIState (savedUIState);

                report (r);
            });
        }
    };

    struct SaveProjectAsCommand : ModelCommand
    {
        SaveProjectAsCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("project.saveAs", "Save Project As...", m, h) {}

        void execute() override
        {
            host.chooseProjectSaveLocation ([this] (const juce::File& folder)
            {
                report (model.saveProjectAs (folder, host.captureUIState()));
            });
        }
    };

    struct SaveProjectCommand : ModelCommand
    {
        SaveProjectCommand (ApplicationModel& m, AppCommandHost& h, SaveProjectAsCommand& sa)
            : ModelCommand ("project.save", "Save Project", m, h), saveAs (sa) {}

        void execute() override
        {
            if (model.isProjectUntitled())
                saveAs.execute();
            else
                report (model.saveProject (host.captureUIState()));
        }

        SaveProjectAsCommand& saveAs;
    };

    //==============================================================================
    struct AddTrackCommand : ModelCommand
    {
        AddTrackCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("track.add", "Add Audio Track", m, h) {}
        void execute() override   { model.addAudioTrack(); }
    };

    struct RemoveTrackCommand : ModelCommand
    {
        RemoveTrackCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("track.remove", "Remove Track", m, h) {}
        void execute() override   { model.removeTrack(); }
    };

    struct AddClipCommand : ModelCommand
    {
        AddClipCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("clip.add", "Add Audio Clip...", m, h) {}

        void execute() override
        {
            host.chooseAudioFile ([this] (const juce::File& f) { report (model.insertAudioClip (f)); });
        }
    };

    //==============================================================================
    struct UndoCommand : ModelCommand
    {
        UndoCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("edit.undo", "Undo", m, h) {}
        void execute() override           { model.undo(); }
        bool isEnabled() const override   { return model.canUndo(); }
    };

    struct RedoCommand : ModelCommand
    {
        RedoCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("edit.redo", "Redo", m, h) {}
        void execute() override           { model.redo(); }
        bool isEnabled() const override   { return model.canRedo(); }
    };

    //==============================================================================
    struct PlayCommand : ModelCommand
    {
        PlayCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("transport.play", "Play", m, h) {}
        void execute() override   { model.play(); }
    };

    struct StopCommand : ModelCommand
    {
        StopCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("transport.stop", "Stop", m, h) {}
        void execute() override   { model.stop(); }
    };

    /** The spacebar: dispatches to transport.play or transport.stop, so both
        surfaces run the very same Commands. */
    struct TogglePlayCommand : ModelCommand
    {
        TogglePlayCommand (ApplicationModel& m, AppCommandHost& h, CommandRegistry& r)
            : ModelCommand ("transport.togglePlay", "Play/Stop", m, h), registry (r) {}

        void execute() override   { registry.invoke (model.isPlaying() ? "transport.stop" : "transport.play"); }

        CommandRegistry& registry;
    };

    struct ReturnToStartCommand : ModelCommand
    {
        ReturnToStartCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("transport.returnToStart", "Return to Start", m, h) {}
        void execute() override   { model.returnToStart(); }
    };
}

void registerAppCommands (CommandRegistry& registry, ApplicationModel& model, AppCommandHost& host)
{
    auto saveAs = std::make_unique<SaveProjectAsCommand> (model, host);
    auto save = std::make_unique<SaveProjectCommand> (model, host, *saveAs);

    registry.add (std::make_unique<NewProjectCommand> (model, host));
    registry.add (std::make_unique<OpenProjectCommand> (model, host));
    registry.add (std::move (save));
    registry.add (std::move (saveAs));

    registry.add (std::make_unique<AddTrackCommand> (model, host));
    registry.add (std::make_unique<RemoveTrackCommand> (model, host));
    registry.add (std::make_unique<AddClipCommand> (model, host));

    registry.add (std::make_unique<UndoCommand> (model, host));
    registry.add (std::make_unique<RedoCommand> (model, host));

    registry.add (std::make_unique<PlayCommand> (model, host));
    registry.add (std::make_unique<StopCommand> (model, host));
    registry.add (std::make_unique<TogglePlayCommand> (model, host, registry));
    registry.add (std::make_unique<ReturnToStartCommand> (model, host));
}

} // namespace papercut
