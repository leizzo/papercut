#include "TestFixture.h"

#include <tracktion_engine/tracktion_engine.h>

namespace resamper::test
{

struct NoteEditingTests : juce::UnitTest
{
    NoteEditingTests() : juce::UnitTest ("Note Editing", "Resamper") {}

    /** A MIDI track and one empty one-bar clip at the playhead. At the default
        120 bpm a quarter note is half a second and the bar is two seconds. */
    struct Notes : Fixture
    {
        Notes()
        {
            invoke ("track.addMidi");
            model.selectTrack (model.getTracks()[0].id);
            invoke ("clip.addMidi");
        }

        ClipInfo clip() const          { return model.getTracks()[0].clips[0]; }
        juce::String clipId() const    { return clip().id; }

        MidiNoteInfo noteByPitch (int pitch) const
        {
            for (auto& note : clip().notes)
                if (note.pitch == pitch)
                    return note;

            return {};
        }
    };

    void runTest() override
    {
        beginTest ("note.add places a note on the MIDI clip, selects it, and is one undo step");
        {
            Notes f;
            expect (f.invoke ("note.add", noteAddArgs (f.clipId(), 0.0, 0.5, 60)));

            auto note = f.noteByPitch (60);
            expect (note.id.isNotEmpty());
            expectEquals (note.pitch, 60);
            expectEquals (note.velocity, 100);
            expectWithinAbsoluteError (note.startSeconds, 0.0, 1e-6);
            expectWithinAbsoluteError (note.lengthSeconds, 0.5, 1e-3);
            expect (note.selected);
            expect (f.commands.find ("note.delete")->isEnabled());

            f.invoke ("edit.undo");
            expect (f.clip().notes.empty());
            expect (! f.commands.find ("note.delete")->isEnabled());

            f.invoke ("edit.redo");
            expect (f.noteByPitch (60).selected);
            expectWithinAbsoluteError (f.noteByPitch (60).lengthSeconds, 0.5, 1e-3);
        }

        beginTest ("note.add uses clip-local time when the clip is not at the Edit start");
        {
            Notes f;
            f.model.setTransportPosition (1.0);
            f.invoke ("clip.addMidi");
            const auto id = f.model.getTracks()[0].clips[1].id;

            f.invoke ("note.add", noteAddArgs (id, 0.25, 0.5, 67, 80));

            auto note = f.model.getTracks()[0].clips[1].notes[0];
            expectEquals (note.pitch, 67);
            expectEquals (note.velocity, 80);
            expectWithinAbsoluteError (note.startSeconds, 0.25, 1e-3);
            expectWithinAbsoluteError (note.lengthSeconds, 0.5, 1e-3);
        }

        beginTest ("note.add with a bad clip, pitch, or length changes nothing");
        {
            Notes f;
            f.invoke ("note.add", {});
            f.invoke ("note.add", noteAddArgs ("no-such-clip", 0.0, 0.5, 60));
            f.invoke ("note.add", noteAddArgs (f.clipId(), 0.0, 0.0, 60));
            f.invoke ("note.add", noteAddArgs (f.clipId(), 0.0, 0.5, 200));
            f.invoke ("note.add", noteAddArgs (f.clipId(), -0.1, 0.5, 60));
            expect (f.clip().notes.empty());

            f.invoke ("edit.undo");
            expect (f.model.getTracks()[0].clips.empty());
        }

        beginTest ("note.add on an audio clip changes nothing");
        {
            Fixture f;
            f.invoke ("track.add");
            f.audioFileToChoose = writeSineWav (f.scratchDir().getChildFile ("tone.wav"), 1.0);
            f.invoke ("clip.add");
            const auto id = f.model.getTracks()[0].clips[0].id;

            f.invoke ("note.add", noteAddArgs (id, 0.0, 0.5, 60));
            expect (f.model.getTracks()[0].clips[0].notes.empty());

            f.invoke ("edit.undo");
            expect (f.model.getTracks()[0].clips.empty());
        }

        //==============================================================================
        beginTest ("Selecting notes marks them and creates no undo step");
        {
            Notes f;
            f.invoke ("note.add", noteAddArgs (f.clipId(), 0.0, 0.5, 60));
            f.invoke ("note.add", noteAddArgs (f.clipId(), 0.5, 0.5, 64));
            const auto first = f.noteByPitch (60).id;

            f.model.selectNotes ({ first });
            expect (f.noteByPitch (60).selected);
            expect (! f.noteByPitch (64).selected);

            f.invoke ("edit.undo");   // undoes the second note.add, not the selection
            expectEquals ((int) f.clip().notes.size(), 1);
            expect (f.noteByPitch (60).selected);

            f.invoke ("edit.redo");
            expect (f.noteByPitch (60).selected);
            expect (! f.noteByPitch (64).selected);
        }

        beginTest ("note.delete removes the selected notes as one undo step, and does nothing when none are selected");
        {
            Notes f;
            f.invoke ("note.add", noteAddArgs (f.clipId(), 0.0, 0.5, 60));
            f.invoke ("note.add", noteAddArgs (f.clipId(), 0.5, 0.5, 64));
            f.model.selectNotes ({ f.noteByPitch (60).id, f.noteByPitch (64).id });

            expect (f.commands.find ("note.delete")->isEnabled());
            f.invoke ("note.delete");
            expect (f.clip().notes.empty());

            f.invoke ("edit.undo");
            expectEquals ((int) f.clip().notes.size(), 2);
            expect (f.noteByPitch (60).selected);
            expect (f.noteByPitch (64).selected);

            f.model.selectNotes ({});
            expect (! f.commands.find ("note.delete")->isEnabled());
            f.invoke ("note.delete");
            f.invoke ("edit.undo");
            expectEquals ((int) f.clip().notes.size(), 1);
        }

        //==============================================================================
        beginTest ("note.move changes start and pitch together, and a no-op records nothing");
        {
            Notes f;
            f.invoke ("note.add", noteAddArgs (f.clipId(), 0.0, 0.5, 60));
            const auto id = f.noteByPitch (60).id;

            f.invoke ("note.move", noteMoveArgs (f.clipId(), { id }, 0.5, 3));
            expectEquals (f.noteByPitch (63).pitch, 63);
            expectWithinAbsoluteError (f.noteByPitch (63).startSeconds, 0.5, 1e-3);
            expectWithinAbsoluteError (f.noteByPitch (63).lengthSeconds, 0.5, 1e-3);

            f.invoke ("edit.undo");
            expectEquals (f.noteByPitch (60).pitch, 60);
            expectWithinAbsoluteError (f.noteByPitch (60).startSeconds, 0.0, 1e-3);

            f.invoke ("note.move", noteMoveArgs (f.clipId(), { id }, 0.0, 0));
            f.invoke ("note.move", noteMoveArgs (f.clipId(), { "no-such-note" }, 0.5, 1));
            f.invoke ("note.move", {});
            f.invoke ("edit.undo");
            expect (f.clip().notes.empty());
        }

        beginTest ("note.move of several notes is one undo step and stays inside pitch and the clip start");
        {
            Notes f;
            f.invoke ("note.add", noteAddArgs (f.clipId(), 0.0, 0.5, 60));
            f.invoke ("note.add", noteAddArgs (f.clipId(), 0.5, 0.5, 120));
            const auto low = f.noteByPitch (60).id;
            const auto high = f.noteByPitch (120).id;

            f.invoke ("note.move", noteMoveArgs (f.clipId(), { low, high }, 0.0, 20));
            expectEquals (f.noteByPitch (67).pitch, 67);     // 60 + (127 - 120)
            expectEquals (f.noteByPitch (127).pitch, 127);

            f.invoke ("edit.undo");
            expectEquals (f.noteByPitch (60).pitch, 60);
            expectEquals (f.noteByPitch (120).pitch, 120);

            f.invoke ("note.move", noteMoveArgs (f.clipId(), { high }, -5.0, 0));
            expectWithinAbsoluteError (f.noteByPitch (120).startSeconds, 0.0, 1e-3);

            f.invoke ("edit.undo");
            expectWithinAbsoluteError (f.noteByPitch (120).startSeconds, 0.5, 1e-3);
        }

        //==============================================================================
        beginTest ("note.resize sets the note's edges, and an empty range changes nothing");
        {
            Notes f;
            f.invoke ("note.add", noteAddArgs (f.clipId(), 0.0, 0.5, 60));
            const auto id = f.noteByPitch (60).id;

            f.invoke ("note.resize", noteResizeArgs (f.clipId(), id, 0.25, 1.0));
            expectWithinAbsoluteError (f.noteByPitch (60).startSeconds, 0.25, 1e-3);
            expectWithinAbsoluteError (f.noteByPitch (60).lengthSeconds, 0.75, 1e-3);

            f.invoke ("edit.undo");
            expectWithinAbsoluteError (f.noteByPitch (60).startSeconds, 0.0, 1e-3);
            expectWithinAbsoluteError (f.noteByPitch (60).lengthSeconds, 0.5, 1e-3);

            f.invoke ("note.resize", noteResizeArgs (f.clipId(), id, 1.0, 1.0));
            f.invoke ("note.resize", noteResizeArgs (f.clipId(), id, 1.5, 0.5));
            f.invoke ("edit.undo");
            expect (f.clip().notes.empty());
        }

        //==============================================================================
        beginTest ("note.setVelocity sets the selected notes, joins a continued gesture, and ignores the same value");
        {
            Notes f;
            f.invoke ("note.add", noteAddArgs (f.clipId(), 0.0, 0.5, 60));
            f.invoke ("note.add", noteAddArgs (f.clipId(), 0.5, 0.5, 64));
            f.model.selectNotes ({ f.noteByPitch (60).id, f.noteByPitch (64).id });

            f.invoke ("note.setVelocity", noteVelocityArgs (f.clipId(), 80));
            f.invoke ("note.setVelocity", noteVelocityArgs (f.clipId(), 40, true));
            expectEquals (f.noteByPitch (60).velocity, 40);
            expectEquals (f.noteByPitch (64).velocity, 40);

            f.invoke ("edit.undo");
            expectEquals (f.noteByPitch (60).velocity, 100);
            expectEquals (f.noteByPitch (64).velocity, 100);

            f.invoke ("note.setVelocity", noteVelocityArgs (f.clipId(), 100));
            f.model.selectNotes ({});
            f.invoke ("note.setVelocity", noteVelocityArgs (f.clipId(), 20));
            f.invoke ("edit.undo");
            expectEquals ((int) f.clip().notes.size(), 1);
        }

        //==============================================================================
        // 0.35 s is 0.7 quarter-notes at 120 bpm.
        //   1/4  snaps to 1 beat   = 0.5 s
        //   1/8  snaps to 0.5 beat = 0.25 s
        //   1/16 snaps to 0.75 beat = 0.375 s
        beginTest ("note.quantize snaps selected notes to the grid, or every note when none are selected");
        {
            Notes f;
            f.invoke ("note.add", noteAddArgs (f.clipId(), 0.35, 0.5, 60));
            f.invoke ("note.add", noteAddArgs (f.clipId(), 0.35, 0.5, 64));
            f.model.selectNotes ({});

            f.invoke ("note.quantize", noteQuantizeArgs (f.clipId(), "1/4"));
            expectWithinAbsoluteError (f.noteByPitch (60).startSeconds, 0.5, 1e-3);
            expectWithinAbsoluteError (f.noteByPitch (64).startSeconds, 0.5, 1e-3);
            expectWithinAbsoluteError (f.noteByPitch (60).lengthSeconds, 0.5, 1e-3);

            f.invoke ("edit.undo");
            f.model.selectNotes ({ f.noteByPitch (60).id });
            f.invoke ("note.quantize", noteQuantizeArgs (f.clipId(), "1/8"));
            expectWithinAbsoluteError (f.noteByPitch (60).startSeconds, 0.25, 1e-3);
            expectWithinAbsoluteError (f.noteByPitch (64).startSeconds, 0.35, 1e-3);

            f.invoke ("edit.undo");
            f.model.selectNotes ({});
            f.invoke ("note.quantize", noteQuantizeArgs (f.clipId(), "1/16"));
            expectWithinAbsoluteError (f.noteByPitch (60).startSeconds, 0.375, 1e-3);
            expectWithinAbsoluteError (f.noteByPitch (64).startSeconds, 0.375, 1e-3);
        }

        beginTest ("note.quantize of an on-grid note or an unknown grid records nothing");
        {
            Notes f;
            f.invoke ("note.add", noteAddArgs (f.clipId(), 0.0, 0.5, 60));
            f.model.selectNotes ({});

            f.invoke ("note.quantize", noteQuantizeArgs (f.clipId(), "1/4"));
            f.invoke ("note.quantize", noteQuantizeArgs (f.clipId(), "1/32"));
            f.invoke ("note.quantize", {});
            f.invoke ("edit.undo");
            expect (f.clip().notes.empty());
        }

        // The beat grid is Edit time. A clip at 0.1 s (0.2 beats at 120 bpm) makes
        // a note's content beat differ from the line it sits on.
        beginTest ("note.quantize snaps to the Edit beat grid, including a clip that starts off the grid");
        {
            Notes f;
            f.invoke ("clip.move", clipMoveArgs (f.clipId(), 0.1));
            f.invoke ("note.add", noteAddArgs (f.clipId(), 0.4, 0.5, 60));
            f.model.selectNotes ({});

            f.invoke ("note.quantize", noteQuantizeArgs (f.clipId(), "1/4"));
            f.invoke ("edit.undo");
            expect (f.clip().notes.empty(), "a note already on an Edit beat line records nothing");

            f.invoke ("note.add", noteAddArgs (f.clipId(), 0.35, 0.5, 60));
            f.model.selectNotes ({});
            f.invoke ("note.quantize", noteQuantizeArgs (f.clipId(), "1/4"));
            expectWithinAbsoluteError (f.noteByPitch (60).startSeconds, 0.4, 1e-3);
            expectWithinAbsoluteError (f.noteByPitch (60).lengthSeconds, 0.5, 1e-3);
        }

        beginTest ("note.quantize treats a selection of deleted notes as none selected");
        {
            Notes f;
            f.invoke ("note.add", noteAddArgs (f.clipId(), 0.35, 0.5, 60));
            f.invoke ("note.add", noteAddArgs (f.clipId(), 0.35, 0.5, 64));
            f.invoke ("note.delete");

            f.invoke ("note.quantize", noteQuantizeArgs (f.clipId(), "1/4"));
            expectEquals ((int) f.clip().notes.size(), 1);
            expectWithinAbsoluteError (f.noteByPitch (60).startSeconds, 0.5, 1e-3);
        }

        //==============================================================================
        beginTest ("An added note plays back and shows in the clip's arrangement preview");
        {
            Notes f;
            f.invoke ("note.add", noteAddArgs (f.clipId(), 0.0, 0.5, 60));

            auto notes = f.clip().notes;
            expectEquals ((int) notes.size(), 1);
            expectEquals (notes[0].pitch, 60);
            expectWithinAbsoluteError (notes[0].startSeconds, 0.0, 1e-3);
            expectWithinAbsoluteError (notes[0].lengthSeconds, 0.5, 1e-3);

            expectGreaterThan (renderPeak (f), 0.01f);
        }
    }
};

static NoteEditingTests noteEditingTests;

} // namespace resamper::test
