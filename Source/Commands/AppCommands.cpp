#include "AppCommands.h"
#include "TapTempo.h"

#include <optional>

#include "Engine/ApplicationModel.h"

namespace resamper
{

namespace
{
    /** Keys of the args built by the *Args functions below. */
    namespace ArgKeys
    {
        const juce::Identifier clipId ("clipId"), start ("start"), end ("end"), trackId ("trackId"),
                               value ("value"), continuesGesture ("continuesGesture"), input ("input"),
                               take ("take"), position ("position"),
                               pitch ("pitch"), length ("length"), velocity ("velocity"), grid ("grid"),
                               noteId ("noteId"), noteIds ("noteIds"),
                               deltaSeconds ("deltaSeconds"), deltaPitch ("deltaPitch"),
                               bpm ("bpm"), file ("file"), name ("name"), argument ("argument"), numerator ("numerator"), denominator ("denominator"),
                               countIn ("countIn");
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

    struct AddMidiTrackCommand : ModelCommand
    {
        AddMidiTrackCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("track.addMidi", "Add MIDI Track", m, h) {}
        void execute (const juce::var&) override   { model.addMidiTrack(); }
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

    /** A track header's input menu. */
    /** F1-F8: the mute of the track at args["argument"] (0-based). */
    struct ToggleMuteAtCommand : ModelCommand
    {
        ToggleMuteAtCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("track.toggleMuteAt", "Mute Track", m, h) {}

        void execute (const juce::var& args) override
        {
            const auto tracks = model.getTracks();
            const auto index = (int) args[ArgKeys::argument];

            if (juce::isPositiveAndBelow (index, (int) tracks.size()))
                model.setTrackMuted (tracks[(size_t) index].id, ! tracks[(size_t) index].muted);
        }
    };

    /** S: solo the selected track. */
    struct ToggleSoloSelectedCommand : ModelCommand
    {
        ToggleSoloSelectedCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("track.toggleSoloSelected", "Solo", m, h) {}

        void execute (const juce::var&) override
        {
            for (auto& track : model.getTracks())
                if (track.id == model.getSelectedTrackId())
                    model.setTrackSolo (track.id, ! track.solo);
        }

        bool isEnabled() const override   { return model.getSelectedTrackId().isNotEmpty(); }
    };

    /** The span of the selected clips, if any. */
    std::optional<TimeRangeSeconds> selectionSpan (const ApplicationModel& model)
    {
        std::optional<TimeRangeSeconds> span;

        for (auto& track : model.getTracks())
            for (auto& clip : track.clips)
                if (clip.selected)
                {
                    const auto end = clip.startSeconds + clip.lengthSeconds;
                    span = span ? TimeRangeSeconds { std::min (span->start, clip.startSeconds), std::max (span->end, end) }
                                : TimeRangeSeconds { clip.startSeconds, end };
                }

        return span;
    }

    /** Mod+L: loop the selection, or with nothing selected, toggle the loop. */
    struct LoopSelectionCommand : ModelCommand
    {
        LoopSelectionCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("transport.loopSelection", "Loop Selection", m, h) {}

        void execute (const juce::var&) override
        {
            if (auto span = selectionSpan (model); span && model.setLoopRange (span->start, span->end))
                model.setLooping (true);
            else
                model.setLooping (! model.isLooping());
        }
    };

    /** Shift+Space: play from the start of the selection (else as Play). */
    struct PlayFromSelectionCommand : ModelCommand
    {
        PlayFromSelectionCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("transport.playFromSelection", "Play from Selection", m, h) {}

        void execute (const juce::var&) override
        {
            if (auto span = selectionSpan (model))
                model.setTransportPosition (span->start);

            model.play();
        }
    };

    /** Up / Down (Shift: octave) in the Piano Roll. */
    struct TransposeSelectedNotesCommand : ModelCommand
    {
        TransposeSelectedNotesCommand (ApplicationModel& m, AppCommandHost& h)
            : ModelCommand ("note.transposeSelected", "Transpose", m, h) {}

        void execute (const juce::var& args) override
        {
            const auto clipId = args[ArgKeys::clipId].toString();
            juce::StringArray selected;

            for (auto& track : model.getTracks())
                for (auto& clip : track.clips)
                    if (clip.id == clipId)
                        for (auto& note : clip.notes)
                            if (note.selected)
                                selected.add (note.id);

            if (! selected.isEmpty())
                model.moveNotes (clipId, selected, 0.0, (int) args[ArgKeys::argument]);
        }
    };

    /** Mod+A in the Piano Roll. Never undoable. */
    struct SelectAllNotesCommand : ModelCommand
    {
        SelectAllNotesCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("note.selectAll", "Select All Notes", m, h) {}

        void execute (const juce::var& args) override
        {
            juce::StringArray ids;

            for (auto& track : model.getTracks())
                for (auto& clip : track.clips)
                    if (clip.id == args[ArgKeys::clipId].toString())
                        for (auto& note : clip.notes)
                            ids.add (note.id);

            model.selectNotes (ids);
        }
    };

    struct DeselectAllCommand : ModelCommand
    {
        DeselectAllCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("edit.deselectAll", "Deselect All", m, h) {}
        void execute (const juce::var&) override   { model.deselectAll(); }
    };

    /** Selects a track in every view (a mixer strip click). Never undoable. */
    struct SelectTrackCommand : ModelCommand
    {
        SelectTrackCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("track.select", "Select Track", m, h) {}
        void execute (const juce::var& args) override   { model.selectTrack (args[ArgKeys::trackId].toString()); }
    };

    struct SetTrackColourCommand : ModelCommand
    {
        SetTrackColourCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("track.setColour", "Set Track Colour", m, h) {}

        void execute (const juce::var& args) override
        {
            model.setTrackColour (args[ArgKeys::trackId].toString(), (int) args[ArgKeys::value]);
        }
    };

    struct SetTrackInputCommand : ModelCommand
    {
        SetTrackInputCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("track.setInput", "Set Track Input", m, h) {}

        void execute (const juce::var& args) override
        {
            model.setTrackInput (args[ArgKeys::trackId], args[ArgKeys::input]);
        }
    };

    /** Flips mute, solo or arm on the track in trackArgs. */
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

    /** A sample dropped from the Browser onto a lane. */
    struct InsertClipAtCommand : ModelCommand
    {
        InsertClipAtCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("clip.insertAt", "Insert Audio Clip", m, h) {}

        void execute (const juce::var& args) override
        {
            report (model.insertAudioClipAt (juce::File (args[ArgKeys::file].toString()), args[ArgKeys::trackId].toString(),
                                             (double) args[ArgKeys::start]));
        }
    };

    struct AddMidiClipCommand : ModelCommand
    {
        AddMidiClipCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("clip.addMidi", "Add MIDI Clip", m, h) {}
        void execute (const juce::var&) override   { report (model.insertMidiClip()); }
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

    /** A clip's take menu. */
    /** Alt-drag: a copy at the drop position. */
    struct CopyClipCommand : ModelCommand
    {
        CopyClipCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("clip.copy", "Copy Clip", m, h) {}

        void execute (const juce::var& args) override
        {
            report (model.copyClip (args[ArgKeys::clipId].toString(), (double) args[ArgKeys::start], args[ArgKeys::trackId].toString()));
        }
    };

    /** The top-right corner drag: the clip repeats up to the new end. */
    struct LoopExtendClipCommand : ModelCommand
    {
        LoopExtendClipCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("clip.loopExtend", "Loop Clip", m, h) {}

        void execute (const juce::var& args) override
        {
            model.loopExtendClip (args[ArgKeys::clipId].toString(), (double) args[ArgKeys::end]);
        }
    };

    struct SelectionCommand : ModelCommand
    {
        SelectionCommand (const char* id, const char* name, ApplicationModel& m, AppCommandHost& h,
                          std::function<void (ApplicationModel&, AppCommandHost&)> fn)
            : ModelCommand (id, name, m, h), action (std::move (fn)) {}

        void execute (const juce::var&) override   { action (model, host); }
        bool isEnabled() const override             { return ! model.getSelectedClipIds().isEmpty(); }

        std::function<void (ApplicationModel&, AppCommandHost&)> action;
    };

    /** Delete / Backspace: the selected notes (Piano Roll), else the selected clips. */
    struct DeleteCommand : ModelCommand
    {
        DeleteCommand (ApplicationModel& m, AppCommandHost& h, CommandRegistry& r)
            : ModelCommand ("edit.delete", "Delete", m, h), registry (r) {}

        void execute (const juce::var&) override
        {
            registry.invoke (model.hasSelectedNotes() ? "note.delete" : "clip.delete");
        }

        bool isEnabled() const override   { return model.hasSelectedNotes() || ! model.getSelectedClipIds().isEmpty(); }

        CommandRegistry& registry;
    };

    struct RenameClipCommand : ModelCommand
    {
        RenameClipCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("clip.rename", "Rename Clip", m, h) {}

        void execute (const juce::var& args) override
        {
            model.renameClip (args[ArgKeys::clipId].toString(), args[ArgKeys::name].toString());
        }
    };

    struct ReverseClipCommand : ModelCommand
    {
        ReverseClipCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("clip.reverse", "Reverse", m, h) {}

        void execute (const juce::var& args) override
        {
            auto id = args[ArgKeys::clipId].toString();
            model.reverseClip (id.isNotEmpty() ? id : model.getSelectedClipId());
        }
    };

    struct SetClipColourCommand : ModelCommand
    {
        SetClipColourCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("clip.setColour", "Clip Colour", m, h) {}

        void execute (const juce::var& args) override
        {
            const auto value = args[ArgKeys::value];

            // A missing value would otherwise read as 0, the first palette colour.
            if (value.isInt() || value.isInt64() || value.isDouble())
                model.setClipColour (args[ArgKeys::clipId].toString(), (int) value);
        }
    };

    struct SetClipTakeCommand : ModelCommand
    {
        SetClipTakeCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("clip.setTake", "Switch Take", m, h) {}

        void execute (const juce::var& args) override
        {
            if (args[ArgKeys::take].isInt())
                model.setClipTake (args[ArgKeys::clipId], args[ArgKeys::take]);
        }
    };

    //==============================================================================
    struct AddNoteCommand : ModelCommand
    {
        AddNoteCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("note.add", "Add Note", m, h) {}

        void execute (const juce::var& args) override
        {
            auto pitch = args[ArgKeys::pitch];
            auto velocity = args[ArgKeys::velocity];

            if (! args[ArgKeys::start].isDouble() || ! args[ArgKeys::length].isDouble()
                || ! (pitch.isInt() || pitch.isDouble())
                || ! (velocity.isVoid() || velocity.isInt() || velocity.isDouble()))
                return;

            const int vel = velocity.isVoid() ? ApplicationModel::defaultNoteVelocity : (int) velocity;
            model.addNote (args[ArgKeys::clipId].toString(), args[ArgKeys::start], args[ArgKeys::length], (int) pitch, vel);
        }
    };

    struct DeleteNotesCommand : ModelCommand
    {
        DeleteNotesCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("note.delete", "Delete Notes", m, h) {}
        void execute (const juce::var&) override   { model.deleteSelectedNotes(); }
        bool isEnabled() const override            { return model.hasSelectedNotes(); }
    };

    /** A drag of one or more notes in the Piano Roll. */
    struct MoveNotesCommand : ModelCommand
    {
        MoveNotesCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("note.move", "Move Notes", m, h) {}

        void execute (const juce::var& args) override
        {
            if (! args[ArgKeys::deltaSeconds].isDouble()
                || ! (args[ArgKeys::deltaPitch].isInt() || args[ArgKeys::deltaPitch].isDouble()))
                return;

            juce::StringArray ids;

            if (auto* list = args[ArgKeys::noteIds].getArray())
                for (auto& id : *list)
                    ids.add (id.toString());

            model.moveNotes (args[ArgKeys::clipId].toString(), ids, args[ArgKeys::deltaSeconds], (int) args[ArgKeys::deltaPitch]);
        }
    };

    /** A drag on a note's edge in the Piano Roll. */
    struct ResizeNoteCommand : ModelCommand
    {
        ResizeNoteCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("note.resize", "Resize Note", m, h) {}

        void execute (const juce::var& args) override
        {
            if (args[ArgKeys::start].isDouble() && args[ArgKeys::end].isDouble())
                model.resizeNote (args[ArgKeys::clipId].toString(), args[ArgKeys::noteId].toString(),
                                  args[ArgKeys::start], args[ArgKeys::end]);
        }
    };

    /** A drag in the velocity lane. */
    struct SetNoteVelocityCommand : ModelCommand
    {
        SetNoteVelocityCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("note.setVelocity", "Set Note Velocity", m, h) {}

        void execute (const juce::var& args) override
        {
            if (args[ArgKeys::velocity].isInt() || args[ArgKeys::velocity].isDouble())
                model.setNoteVelocity (args[ArgKeys::clipId].toString(), (int) args[ArgKeys::velocity],
                                       args[ArgKeys::continuesGesture]);
        }
    };

    struct QuantizeNotesCommand : ModelCommand
    {
        QuantizeNotesCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("note.quantize", "Quantize Notes", m, h) {}

        void execute (const juce::var& args) override
        {
            auto grid = args[ArgKeys::grid].toString();

            if (grid.isNotEmpty())
                model.quantizeNotes (args[ArgKeys::clipId].toString(), grid);
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

    struct RecordCommand : ModelCommand
    {
        RecordCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("transport.record", "Record", m, h) {}

        /** Counts in when the count-in is on, unless args say countIn: false (Shift-click Rec). */
        void execute (const juce::var& args) override
        {
            const auto* obj = args.getDynamicObject();
            report (model.record (obj == nullptr || ! obj->hasProperty (ArgKeys::countIn) || (bool) args[ArgKeys::countIn]));
        }
    };

    struct ToggleLoopCommand : ModelCommand
    {
        ToggleLoopCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("transport.toggleLoop", "Loop", m, h) {}
        void execute (const juce::var&) override   { model.setLooping (! model.isLooping()); }
    };

    /** A drag along the timeline ruler. */
    struct SetLoopRangeCommand : ModelCommand
    {
        SetLoopRangeCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("transport.setLoopRange", "Set Loop", m, h) {}

        void execute (const juce::var& args) override
        {
            if (args[ArgKeys::start].isDouble() && args[ArgKeys::end].isDouble()
                 && model.setLoopRange (args[ArgKeys::start], args[ArgKeys::end]))
                model.setLooping (true);
        }
    };

    struct ReturnToStartCommand : ModelCommand
    {
        ReturnToStartCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("transport.returnToStart", "Return to Start", m, h) {}
        void execute (const juce::var&) override   { model.returnToStart(); }
    };

    struct SetTempoCommand : ModelCommand
    {
        SetTempoCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("transport.setTempo", "Set Tempo", m, h) {}

        void execute (const juce::var& args) override
        {
            if (auto bpm = args[ArgKeys::bpm]; bpm.isDouble() || bpm.isInt())
                model.setTempo (bpm, (bool) args[ArgKeys::continuesGesture]);
        }
    };

    /** `T`: sets the tempo from the last few taps. */
    struct TapTempoCommand : ModelCommand
    {
        TapTempoCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("transport.tapTempo", "Tap Tempo", m, h) {}

        void execute (const juce::var&) override
        {
            if (auto bpm = taps.tap (juce::Time::getMillisecondCounterHiRes() / 1000.0); bpm > 0)
                model.setTempo (bpm);
        }

        TapTempo taps;
    };

    struct SetTimeSignatureCommand : ModelCommand
    {
        SetTimeSignatureCommand (ApplicationModel& m, AppCommandHost& h)
            : ModelCommand ("transport.setTimeSignature", "Set Time Signature", m, h) {}

        void execute (const juce::var& args) override
        {
            model.setTimeSignature ((int) args[ArgKeys::numerator], (int) args[ArgKeys::denominator]);
        }
    };

    struct ToggleMetronomeCommand : ModelCommand
    {
        ToggleMetronomeCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("transport.toggleMetronome", "Metronome", m, h) {}
        void execute (const juce::var&) override   { model.setMetronomeOn (! model.isMetronomeOn()); }
        bool isTicked() const override              { return model.isMetronomeOn(); }
    };

    struct ToggleCountInCommand : ModelCommand
    {
        ToggleCountInCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("transport.toggleCountIn", "Count-in (2 Bars)", m, h) {}
        void execute (const juce::var&) override   { model.setCountInOn (! model.isCountInOn()); }
        bool isTicked() const override              { return model.isCountInOn(); }
    };

    /** A click on the timeline ruler. */
    struct SetPositionCommand : ModelCommand
    {
        SetPositionCommand (ApplicationModel& m, AppCommandHost& h) : ModelCommand ("transport.setPosition", "Set Playhead", m, h) {}

        void execute (const juce::var& args) override
        {
            if (args[ArgKeys::position].isDouble())
                model.setTransportPosition (args[ArgKeys::position]);
        }
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
    registry.add (std::make_unique<AddMidiTrackCommand> (model, host));
    registry.add (std::make_unique<RemoveTrackCommand> (model, host));
    registry.add (std::make_unique<SetTrackVolumeCommand> (model, host));
    registry.add (std::make_unique<SetTrackPanCommand> (model, host));
    registry.add (std::make_unique<ToggleTrackFlagCommand> ("track.toggleMute", "Mute Track", model, host,
                                                            [] (const TrackInfo& t) { return t.muted; }, &ApplicationModel::setTrackMuted));
    registry.add (std::make_unique<ToggleTrackFlagCommand> ("track.toggleSolo", "Solo Track", model, host,
                                                            [] (const TrackInfo& t) { return t.solo; }, &ApplicationModel::setTrackSolo));
    registry.add (std::make_unique<ToggleTrackFlagCommand> ("track.toggleArm", "Arm Track for Recording", model, host,
                                                            [] (const TrackInfo& t) { return t.armed; }, &ApplicationModel::setTrackArmed));
    registry.add (std::make_unique<SetTrackInputCommand> (model, host));
    registry.add (std::make_unique<SetTrackColourCommand> (model, host));
    registry.add (std::make_unique<SelectTrackCommand> (model, host));
    registry.add (std::make_unique<DeselectAllCommand> (model, host));
    registry.add (std::make_unique<ToggleMuteAtCommand> (model, host));
    registry.add (std::make_unique<ToggleSoloSelectedCommand> (model, host));
    registry.add (std::make_unique<LoopSelectionCommand> (model, host));
    registry.add (std::make_unique<PlayFromSelectionCommand> (model, host));
    registry.add (std::make_unique<TransposeSelectedNotesCommand> (model, host));
    registry.add (std::make_unique<SelectAllNotesCommand> (model, host));
    registry.add (std::make_unique<AddClipCommand> (model, host));
    registry.add (std::make_unique<InsertClipAtCommand> (model, host));
    registry.add (std::make_unique<AddMidiClipCommand> (model, host));
    registry.add (std::make_unique<MoveClipCommand> (model, host));
    registry.add (std::make_unique<ResizeClipCommand> (model, host));
    registry.add (std::make_unique<SplitClipCommand> (model, host));
    registry.add (std::make_unique<SetClipTakeCommand> (model, host));
    registry.add (std::make_unique<CopyClipCommand> (model, host));
    registry.add (std::make_unique<LoopExtendClipCommand> (model, host));
    registry.add (std::make_unique<RenameClipCommand> (model, host));
    registry.add (std::make_unique<ReverseClipCommand> (model, host));
    registry.add (std::make_unique<SetClipColourCommand> (model, host));
    registry.add (std::make_unique<SelectionCommand> ("clip.duplicate", "Duplicate", model, host,
                                                      [] (ApplicationModel& m, AppCommandHost&) { m.duplicateSelectedClips(); }));
    registry.add (std::make_unique<SelectionCommand> ("clip.consolidate", "Consolidate", model, host,
                                                      [] (ApplicationModel& m, AppCommandHost& h)
                                                      {
                                                          const auto count = m.getSelectedClipIds().size();

                                                          if (auto r = m.consolidateSelectedClips(); r.failed())
                                                          {
                                                              if (h.reportError)
                                                                  h.reportError (r.getErrorMessage());
                                                          }
                                                          else if (h.notify)
                                                          {
                                                              h.notify ("Consolidated " + juce::String (count) + " clips into one", true);
                                                          }
                                                      }));
    registry.add (std::make_unique<DeleteCommand> (model, host, registry));
    registry.add (std::make_unique<SelectionCommand> ("clip.delete", "Delete", model, host,
                                                      [] (ApplicationModel& m, AppCommandHost&) { m.deleteSelectedClips(); }));
    registry.add (std::make_unique<AddNoteCommand> (model, host));
    registry.add (std::make_unique<DeleteNotesCommand> (model, host));
    registry.add (std::make_unique<MoveNotesCommand> (model, host));
    registry.add (std::make_unique<ResizeNoteCommand> (model, host));
    registry.add (std::make_unique<SetNoteVelocityCommand> (model, host));
    registry.add (std::make_unique<QuantizeNotesCommand> (model, host));

    registry.add (std::make_unique<UndoCommand> (model, host));
    registry.add (std::make_unique<RedoCommand> (model, host));

    registry.add (std::make_unique<PlayCommand> (model, host));
    registry.add (std::make_unique<StopCommand> (model, host));
    registry.add (std::make_unique<TogglePlayCommand> (model, host, registry));
    registry.add (std::make_unique<SetTempoCommand> (model, host));
    registry.add (std::make_unique<TapTempoCommand> (model, host));
    registry.add (std::make_unique<SetTimeSignatureCommand> (model, host));
    registry.add (std::make_unique<ToggleMetronomeCommand> (model, host));
    registry.add (std::make_unique<ToggleCountInCommand> (model, host));
    registry.add (std::make_unique<ReturnToStartCommand> (model, host));
    registry.add (std::make_unique<SetPositionCommand> (model, host));
    registry.add (std::make_unique<RecordCommand> (model, host));
    registry.add (std::make_unique<ToggleLoopCommand> (model, host));
    registry.add (std::make_unique<SetLoopRangeCommand> (model, host));
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

juce::var trackInputArgs (const juce::String& trackId, const juce::String& inputName)
{
    auto args = trackArgs (trackId);
    args.getDynamicObject()->setProperty (ArgKeys::input, inputName);
    return args;
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

juce::var clipTakeArgs (const juce::String& clipId, int takeIndex)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::clipId, clipId);
    args->setProperty (ArgKeys::take, takeIndex);
    return args;
}

juce::var recordArgs (bool withCountIn)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::countIn, withCountIn);
    return args;
}

juce::var loopRangeArgs (double startSeconds, double endSeconds)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::start, startSeconds);
    args->setProperty (ArgKeys::end, endSeconds);
    return args;
}

juce::var transportPositionArgs (double seconds)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::position, seconds);
    return args;
}

juce::var noteAddArgs (const juce::String& clipId, double startSeconds, double lengthSeconds, int pitch, int velocity)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::clipId, clipId);
    args->setProperty (ArgKeys::start, startSeconds);
    args->setProperty (ArgKeys::length, lengthSeconds);
    args->setProperty (ArgKeys::pitch, pitch);
    args->setProperty (ArgKeys::velocity, velocity);
    return args;
}

juce::var noteMoveArgs (const juce::String& clipId, const juce::StringArray& noteIds, double deltaSeconds, int deltaPitch)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::clipId, clipId);
    juce::Array<juce::var> ids;

    for (auto& id : noteIds)
        ids.add (id);

    args->setProperty (ArgKeys::noteIds, juce::var (ids));
    args->setProperty (ArgKeys::deltaSeconds, deltaSeconds);
    args->setProperty (ArgKeys::deltaPitch, deltaPitch);
    return args;
}

juce::var noteResizeArgs (const juce::String& clipId, const juce::String& noteId, double startSeconds, double endSeconds)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::clipId, clipId);
    args->setProperty (ArgKeys::noteId, noteId);
    args->setProperty (ArgKeys::start, startSeconds);
    args->setProperty (ArgKeys::end, endSeconds);
    return args;
}

juce::var noteVelocityArgs (const juce::String& clipId, int velocity, bool continuesGesture)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::clipId, clipId);
    args->setProperty (ArgKeys::velocity, velocity);
    args->setProperty (ArgKeys::continuesGesture, continuesGesture);
    return args;
}

juce::var noteQuantizeArgs (const juce::String& clipId, const juce::String& grid)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::clipId, clipId);
    args->setProperty (ArgKeys::grid, grid);
    return args;
}

juce::var tempoArgs (double bpm, bool continuesGesture)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::bpm, bpm);
    args->setProperty (ArgKeys::continuesGesture, continuesGesture);
    return args;
}

juce::var timeSignatureArgs (int numerator, int denominator)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::numerator, numerator);
    args->setProperty (ArgKeys::denominator, denominator);
    return args;
}

juce::var clipInsertAtArgs (const juce::File& file, const juce::String& trackId, double startSeconds)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::file, file.getFullPathName());
    args->setProperty (ArgKeys::trackId, trackId);
    args->setProperty (ArgKeys::start, startSeconds);
    return args;
}

juce::var trackColourArgs (const juce::String& trackId, int colourIndex)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::trackId, trackId);
    args->setProperty (ArgKeys::value, colourIndex);
    return args;
}

juce::var clipArgs (const juce::String& clipId)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::clipId, clipId);
    return args;
}

juce::var clipRenameArgs (const juce::String& clipId, const juce::String& name)
{
    auto args = clipArgs (clipId);
    args.getDynamicObject()->setProperty (ArgKeys::name, name);
    return args;
}

juce::var clipColourArgs (const juce::String& clipId, int colourIndex)
{
    auto args = clipArgs (clipId);
    args.getDynamicObject()->setProperty (ArgKeys::value, colourIndex);
    return args;
}

} // namespace resamper
