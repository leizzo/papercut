#include "AppCommands.h"

#include "Engine/ApplicationModel.h"

namespace papercut
{

namespace
{
    /** Keys of the args built by the *Args functions below. */
    namespace ArgKeys
    {
        const juce::Identifier clipId ("clipId"), start ("start"), end ("end"), trackId ("trackId"),
                               value ("value"), continuesGesture ("continuesGesture");
    }

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

        void execute (const juce::var&) override
        {
            model.newProject();
            host.restoreUIState ({});
        }
    };

    struct OpenProjectCommand : ModelCommand
    {
        OpenProjectCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("project.open", "Open Project...", m, h) {}

        void execute (const juce::var&) override
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

        void execute (const juce::var&) override
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

        void execute (const juce::var&) override
        {
            if (model.isProjectUntitled())
                saveAs.execute ({});
            else
                report (model.saveProject (host.captureUIState()));
        }

        SaveProjectAsCommand& saveAs;
    };

    //==============================================================================
    struct AddTrackCommand : ModelCommand
    {
        AddTrackCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("track.add", "Add Audio Track", m, h) {}
        void execute (const juce::var&) override   { model.addAudioTrack(); }
    };

    struct RemoveTrackCommand : ModelCommand
    {
        RemoveTrackCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("track.remove", "Remove Track", m, h) {}
        void execute (const juce::var&) override   { model.removeTrack(); }
    };

    /** A track header fader: one undo step per drag (see trackVolumeArgs). */
    struct SetTrackVolumeCommand : ModelCommand
    {
        SetTrackVolumeCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("track.setVolume", "Set Track Volume", m, h) {}

        void execute (const juce::var& args) override
        {
            if (args[ArgKeys::value].isDouble())
                model.setTrackVolume (args[ArgKeys::trackId], args[ArgKeys::value], args[ArgKeys::continuesGesture]);
        }
    };

    struct SetTrackPanCommand : ModelCommand
    {
        SetTrackPanCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("track.setPan", "Set Track Pan", m, h) {}

        void execute (const juce::var& args) override
        {
            if (args[ArgKeys::value].isDouble())
                model.setTrackPan (args[ArgKeys::trackId], args[ArgKeys::value], args[ArgKeys::continuesGesture]);
        }
    };

    /** Flips mute or solo on the track in trackArgs. */
    struct ToggleTrackFlagCommand : ModelCommand
    {
        using Getter = bool (*) (const TrackInfo&);
        using Setter = bool (ApplicationModel::*) (const juce::String&, bool);

        ToggleTrackFlagCommand (juce::String id, juce::String name, ApplicationModel& m, AppCommandHost& h, Getter g, Setter s)
            : ModelCommand (std::move (id), std::move (name), m, h), get (g), set (s) {}

        void execute (const juce::var& args) override
        {
            const auto trackId = args[ArgKeys::trackId].toString();

            for (auto& track : model.getTracks())
                if (track.id == trackId)
                    (model.*set) (trackId, ! get (track));
        }

        Getter get;
        Setter set;
    };

    struct AddClipCommand : ModelCommand
    {
        AddClipCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("clip.add", "Add Audio Clip...", m, h) {}

        void execute (const juce::var&) override
        {
            host.chooseAudioFile ([this] (const juce::File& f) { report (model.insertAudioClip (f)); });
        }
    };

    /** A drag in the Arrangement. */
    struct MoveClipCommand : ModelCommand
    {
        MoveClipCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("clip.move", "Move Clip", m, h) {}

        void execute (const juce::var& args) override
        {
            if (args[ArgKeys::start].isDouble())
                model.moveClip (args[ArgKeys::clipId], args[ArgKeys::start], args[ArgKeys::trackId]);
        }
    };

    /** A drag on a clip's edge in the Arrangement. */
    struct ResizeClipCommand : ModelCommand
    {
        ResizeClipCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("clip.resize", "Resize Clip", m, h) {}

        void execute (const juce::var& args) override
        {
            if (args[ArgKeys::start].isDouble() && args[ArgKeys::end].isDouble())
                model.resizeClip (args[ArgKeys::clipId], args[ArgKeys::start], args[ArgKeys::end]);
        }
    };

    /** Cuts the selected clip at the playhead. */
    struct SplitClipCommand : ModelCommand
    {
        SplitClipCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("clip.split", "Split Clip at Playhead", m, h) {}

        void execute (const juce::var&) override
        {
            model.splitClip (model.getSelectedClipId(), model.getTransportPositionSeconds());
        }

        bool isEnabled() const override
        {
            return model.canSplitClip (model.getSelectedClipId(), model.getTransportPositionSeconds());
        }
    };

    //==============================================================================
    struct UndoCommand : ModelCommand
    {
        UndoCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("edit.undo", "Undo", m, h) {}
        void execute (const juce::var&) override           { model.undo(); }
        bool isEnabled() const override   { return model.canUndo(); }
    };

    struct RedoCommand : ModelCommand
    {
        RedoCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("edit.redo", "Redo", m, h) {}
        void execute (const juce::var&) override           { model.redo(); }
        bool isEnabled() const override   { return model.canRedo(); }
    };

    //==============================================================================
    struct PlayCommand : ModelCommand
    {
        PlayCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("transport.play", "Play", m, h) {}
        void execute (const juce::var&) override   { model.play(); }
    };

    struct StopCommand : ModelCommand
    {
        StopCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("transport.stop", "Stop", m, h) {}
        void execute (const juce::var&) override   { model.stop(); }
    };

    /** The spacebar: dispatches to transport.play or transport.stop, so both
        surfaces run the very same Commands. */
    struct TogglePlayCommand : ModelCommand
    {
        TogglePlayCommand (ApplicationModel& m, AppCommandHost& h, CommandRegistry& r)
            : ModelCommand ("transport.togglePlay", "Play/Stop", m, h), registry (r) {}

        void execute (const juce::var&) override   { registry.invoke (model.isPlaying() ? "transport.stop" : "transport.play"); }

        CommandRegistry& registry;
    };

    struct ReturnToStartCommand : ModelCommand
    {
        ReturnToStartCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("transport.returnToStart", "Return to Start", m, h) {}
        void execute (const juce::var&) override   { model.returnToStart(); }
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
    registry.add (std::make_unique<SetTrackVolumeCommand> (model, host));
    registry.add (std::make_unique<SetTrackPanCommand> (model, host));
    registry.add (std::make_unique<ToggleTrackFlagCommand> ("track.toggleMute", "Mute Track", model, host,
                                                            [] (const TrackInfo& t) { return t.muted; }, &ApplicationModel::setTrackMuted));
    registry.add (std::make_unique<ToggleTrackFlagCommand> ("track.toggleSolo", "Solo Track", model, host,
                                                            [] (const TrackInfo& t) { return t.solo; }, &ApplicationModel::setTrackSolo));
    registry.add (std::make_unique<AddClipCommand> (model, host));
    registry.add (std::make_unique<MoveClipCommand> (model, host));
    registry.add (std::make_unique<ResizeClipCommand> (model, host));
    registry.add (std::make_unique<SplitClipCommand> (model, host));

    registry.add (std::make_unique<UndoCommand> (model, host));
    registry.add (std::make_unique<RedoCommand> (model, host));

    registry.add (std::make_unique<PlayCommand> (model, host));
    registry.add (std::make_unique<StopCommand> (model, host));
    registry.add (std::make_unique<TogglePlayCommand> (model, host, registry));
    registry.add (std::make_unique<ReturnToStartCommand> (model, host));
}

juce::var trackArgs (const juce::String& trackId)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::trackId, trackId);
    return args;
}

juce::var trackVolumeArgs (const juce::String& trackId, double db, bool continuesGesture)
{
    auto args = trackArgs (trackId);
    args.getDynamicObject()->setProperty (ArgKeys::value, db);
    args.getDynamicObject()->setProperty (ArgKeys::continuesGesture, continuesGesture);
    return args;
}

juce::var trackPanArgs (const juce::String& trackId, double pan, bool continuesGesture)
{
    return trackVolumeArgs (trackId, pan, continuesGesture);   // same shape
}

juce::var clipMoveArgs (const juce::String& clipId, double startSeconds, const juce::String& trackId)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::clipId, clipId);
    args->setProperty (ArgKeys::start, startSeconds);
    args->setProperty (ArgKeys::trackId, trackId);
    return args;
}

juce::var clipResizeArgs (const juce::String& clipId, double startSeconds, double endSeconds)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::clipId, clipId);
    args->setProperty (ArgKeys::start, startSeconds);
    args->setProperty (ArgKeys::end, endSeconds);
    return args;
}

} // namespace papercut
