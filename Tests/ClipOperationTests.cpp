#include "TestFixture.h"

namespace resamper::test
{

/** Arrangement clip operations (PRD §8.2) at the Command seam. */
struct ClipOperationTests : juce::UnitTest
{
    ClipOperationTests() : juce::UnitTest ("Clip Operations", "Resamper") {}

    /** An audio track with a 1 s tone at 0, and a MIDI track with a one-bar clip holding 3 notes. */
    struct Clips : Fixture
    {
        Clips()
        {
            invoke ("track.add");
            audioFileToChoose = writeSineWav (scratchDir().getChildFile ("tone.wav"), 1.0);
            invoke ("clip.add");
            invoke ("track.addMidi");
            model.selectTrack (track (1).id);
            invoke ("clip.addMidi");

            for (int i = 0; i < 3; ++i)
                invoke ("note.add", noteAddArgs (midiClip().id, 0.25 * i, 0.2, 60 + i));

            model.selectClip ({});
        }

        TrackInfo track (int i) const       { return model.getTracks()[(size_t) i]; }
        ClipInfo audioClip (int i = 0) const { return track (0).clips[(size_t) i]; }
        ClipInfo midiClip (int i = 0) const  { return track (1).clips[(size_t) i]; }
    };

    void runTest() override
    {
        beginTest ("Mod+D duplicates each selected clip right after itself, as one undo step");
        {
            Clips f;
            const auto original = f.midiClip();
            f.model.selectClip (original.id);
            f.model.selectClip (f.audioClip().id, ApplicationModel::SelectionMode::add);

            expect (f.invoke ("clip.duplicate"));
            expectEquals ((int) f.track (0).clips.size(), 2);
            expectEquals ((int) f.track (1).clips.size(), 2);

            const auto copy = f.midiClip (1);
            expectWithinAbsoluteError (copy.startSeconds, original.startSeconds + original.lengthSeconds, 1.0e-6);
            expectEquals ((int) copy.notes.size(), 3);
            expect (copy.notes[0].id != original.notes[0].id, "a copied note gets its own id");
            expectWithinAbsoluteError (f.audioClip (1).startSeconds, 1.0, 1.0e-6);

            // The copies are selected, ready for another Mod+D.
            expect (copy.selected && ! f.midiClip().selected);

            f.invoke ("edit.undo");
            expectEquals ((int) f.track (0).clips.size() + (int) f.track (1).clips.size(), 2);
        }

        beginTest ("Alt-drag copies a clip to a position on a track of its kind; the original stays");
        {
            Clips f;
            f.invoke ("track.add");
            const auto target = f.track (2).id;

            f.invoke ("clip.copy", clipMoveArgs (f.audioClip().id, 4.0, target));
            expectEquals ((int) f.track (0).clips.size(), 1);
            expectEquals ((int) f.track (2).clips.size(), 1);
            expectWithinAbsoluteError (f.track (2).clips[0].startSeconds, 4.0, 1.0e-6);

            f.invoke ("clip.copy", clipMoveArgs (f.midiClip().id, 4.0, target));   // MIDI onto audio: refused
            expectEquals ((int) f.track (2).clips.size(), 1);

            f.invoke ("edit.undo");
            expect (f.track (2).clips.empty());
        }

        beginTest ("Loop-extend repeats a MIDI clip's content to the new end, one undo step");
        {
            Clips f;
            const auto clip = f.midiClip();
            const auto end = clip.startSeconds + 3.0 * clip.lengthSeconds;

            expect (f.invoke ("clip.loopExtend", clipResizeArgs (clip.id, clip.startSeconds, end)));
            auto extended = f.midiClip();
            expect (extended.looping);
            expectWithinAbsoluteError (extended.lengthSeconds, 3.0 * clip.lengthSeconds, 1.0e-6);
            expectWithinAbsoluteError (extended.loopLengthSeconds, clip.lengthSeconds, 1.0e-6);

            f.invoke ("edit.undo");
            expect (! f.midiClip().looping);
            expectWithinAbsoluteError (f.midiClip().lengthSeconds, clip.lengthSeconds, 1.0e-6);
        }

        beginTest ("Loop-extend repeats an audio clip past the end of its file, and it plays there");
        {
            Clips f;
            f.invoke ("track.toggleMute", trackArgs (f.track (1).id));
            const auto clip = f.audioClip();
            expect (f.invoke ("clip.loopExtend", clipResizeArgs (clip.id, 0.0, 3.0)));
            auto extended = f.audioClip();
            expect (extended.looping);
            expectWithinAbsoluteError (extended.lengthSeconds, 3.0, 1.0e-3);
            expectGreaterThan (renderPeak (f), 0.1f);

            f.invoke ("edit.undo");
            expectWithinAbsoluteError (f.audioClip().lengthSeconds, 1.0, 1.0e-3);
        }

        beginTest ("Consolidating MIDI clips merges their notes into one clip spanning them");
        {
            Clips f;
            const auto first = f.midiClip();
            f.model.selectClip (first.id);
            f.invoke ("clip.duplicate");
            f.model.selectClip (f.midiClip (0).id);
            f.model.selectClip (f.midiClip (1).id, ApplicationModel::SelectionMode::add);

            expect (f.invoke ("clip.consolidate"));
            expectEquals ((int) f.track (1).clips.size(), 1);
            const auto merged = f.midiClip();
            expectWithinAbsoluteError (merged.startSeconds, first.startSeconds, 1.0e-6);
            expectWithinAbsoluteError (merged.lengthSeconds, 2.0 * first.lengthSeconds, 1.0e-6);
            expectEquals ((int) merged.notes.size(), 6);
            expectWithinAbsoluteError (merged.notes[3].startSeconds, first.lengthSeconds, 1.0e-6);

            f.invoke ("edit.undo");
            expectEquals ((int) f.track (1).clips.size(), 2);
        }

        beginTest ("Consolidating audio clips renders them into one new audio clip");
        {
            Clips f;
            f.invoke ("track.toggleMute", trackArgs (f.track (1).id));
            f.invoke ("clip.copy", clipMoveArgs (f.audioClip().id, 1.5, f.track (0).id));
            f.model.selectClip (f.audioClip (0).id);
            f.model.selectClip (f.audioClip (1).id, ApplicationModel::SelectionMode::add);

            expect (f.invoke ("clip.consolidate"));
            expectEquals (f.errors.joinIntoString ("; "), juce::String());
            expectEquals ((int) f.track (0).clips.size(), 1);
            const auto merged = f.audioClip();
            expectWithinAbsoluteError (merged.startSeconds, 0.0, 1.0e-3);
            expectWithinAbsoluteError (merged.lengthSeconds, 2.5, 0.01);
            expect (merged.file.existsAsFile());
            expectGreaterThan (renderPeak (f), 0.1f);

            f.invoke ("edit.undo");
            expectEquals ((int) f.track (0).clips.size(), 2);
        }

        beginTest ("Shift/Mod selection: add and toggle clips; a plain click replaces");
        {
            Clips f;
            using Mode = ApplicationModel::SelectionMode;
            f.model.selectClip (f.audioClip().id);
            f.model.selectClip (f.midiClip().id, Mode::add);
            expectEquals (f.model.getSelectedClipIds().size(), 2);
            f.model.selectClip (f.audioClip().id, Mode::toggle);
            expectEquals (f.model.getSelectedClipIds().joinIntoString (","), f.midiClip().id);
            f.model.selectClip (f.audioClip().id);
            expectEquals (f.model.getSelectedClipIds().joinIntoString (","), f.audioClip().id);
        }

        beginTest ("Delete, rename, reverse and colour a clip; each is one undo step");
        {
            Clips f;
            const auto id = f.audioClip().id;

            f.invoke ("clip.rename", clipRenameArgs (id, "Hook"));
            expectEquals (f.audioClip().name, juce::String ("Hook"));

            f.invoke ("clip.setColour", clipColourArgs (id, 4));
            expectEquals (f.audioClip().colourIndex, 4);
            f.invoke ("clip.setColour", clipArgs (id));   // no value: not the first colour
            expectEquals (f.audioClip().colourIndex, 4);

            f.invoke ("clip.reverse", clipArgs (id));
            expect (f.audioClip().reversed);
            f.invoke ("clip.reverse", clipArgs (f.midiClip().id));   // MIDI can't reverse
            expect (! f.midiClip().reversed);

            f.model.selectClip (id);
            f.invoke ("clip.delete");
            expect (f.track (0).clips.empty());

            f.invoke ("edit.undo");
            expect (f.audioClip().reversed);
            f.invoke ("edit.undo");
            expect (! f.audioClip().reversed);
            f.invoke ("edit.undo");
            expectEquals (f.audioClip().colourIndex, -1);
            f.invoke ("edit.undo");
            expectEquals (f.audioClip().name, juce::String ("tone"));
        }

        beginTest ("Tracks select the same way: Shift adds, Mod toggles; the first is the selected track");
        {
            Clips f;
            using Mode = ApplicationModel::SelectionMode;
            f.model.selectTrack (f.track (0).id);
            f.model.selectTrack (f.track (1).id, Mode::add);
            expect (f.track (0).selected && f.track (1).selected);
            f.model.selectTrack (f.track (0).id, Mode::toggle);
            expect (! f.track (0).selected && f.track (1).selected);
            expectEquals (f.model.getSelectedTrackId(), f.track (1).id);
        }

        beginTest ("edit.deselectAll clears clips, tracks and notes, outside undo");
        {
            Clips f;
            f.model.selectClip (f.audioClip().id);
            f.model.selectNotes ({ f.midiClip().notes[0].id });
            const auto couldUndo = f.model.canUndo();

            f.invoke ("edit.deselectAll");
            expect (f.model.getSelectedClipIds().isEmpty());
            expect (f.model.getSelectedTrackId().isEmpty());
            expect (! f.model.hasSelectedNotes());
            expect (f.model.canUndo() == couldUndo);
        }
    }
};

static ClipOperationTests clipOperationTests;

} // namespace resamper::test
