#include "TestFixture.h"

#include <tracktion_engine/tracktion_engine.h>

namespace papercut::test
{

struct ClipEditingTests : juce::UnitTest
{
    ClipEditingTests() : juce::UnitTest ("Clip Editing", "Papercut") {}

    /** One track holding one 2 s clip at 0 s. */
    struct ClipFixture : Fixture
    {
        ClipFixture()
        {
            invoke ("track.add");
            audioFileToChoose = writeSineWav (scratchDir().getChildFile ("tone.wav"), 2.0);
            invoke ("clip.add");
        }

        ClipInfo clip (int track = 0, int index = 0) const   { return model.getTracks()[(size_t) track].clips[(size_t) index]; }
        juce::String clipId() const                          { return clip().id; }
        juce::String trackId (int track) const               { return model.getTracks()[(size_t) track].id; }

        void setPlayhead (double seconds)
        {
            projects.getEdit().getTransport().setPosition (tracktion::TimePosition::fromSeconds (seconds));
        }
    };

    void runTest() override
    {
        //==============================================================================
        beginTest ("Selecting a clip marks it, and creates no undo step");
        {
            ClipFixture f;
            f.invoke ("track.add");

            expect (! f.clip().selected);
            f.model.selectClip (f.clipId());

            expect (f.clip().selected);

            f.invoke ("edit.undo");   // undoes track.add, not the selection
            expectEquals (f.numTracks(), 1);
            expect (f.clip().selected);
        }

        beginTest ("clip.add goes to the selected clip's track; track.remove never takes it");
        {
            ClipFixture f;
            f.invoke ("track.add");
            f.model.selectClip (f.clipId());

            f.invoke ("clip.add");
            expectEquals ((int) f.model.getTracks()[0].clips.size(), 2);

            f.invoke ("track.remove");   // no track is selected: the last one goes
            expectEquals (f.numTracks(), 1);
            expectEquals ((int) f.model.getTracks()[0].clips.size(), 2);
        }

        beginTest ("A selected clip stays selected through a move, a split, and their undo");
        {
            ClipFixture f;
            f.invoke ("track.add");
            f.model.selectClip (f.clipId());

            f.invoke ("clip.move", clipMoveArgs (f.clipId(), 1.0, f.trackId (1)));
            expect (f.clip (1).selected);
            f.invoke ("edit.undo");
            expect (f.clip (0).selected);

            f.setPlayhead (1.0);
            f.invoke ("clip.split");
            expect (f.clip (0, 0).selected);
            f.invoke ("edit.undo");
            expect (f.clip (0, 0).selected);
        }

        //==============================================================================
        beginTest ("clip.move changes the start, keeps the length, and is one undo step");
        {
            ClipFixture f;
            f.invoke ("clip.move", clipMoveArgs (f.clipId(), 3.0));

            expectWithinAbsoluteError (f.clip().startSeconds, 3.0, 1e-6);
            expectWithinAbsoluteError (f.clip().lengthSeconds, 2.0, 1e-3);

            f.invoke ("edit.undo");
            expectWithinAbsoluteError (f.clip().startSeconds, 0.0, 1e-6);
            f.invoke ("edit.undo");
            expect (f.model.getTracks()[0].clips.empty());   // the next step back is clip.add
        }

        beginTest ("clip.move to another track moves the clip there; undo moves it back");
        {
            ClipFixture f;
            f.invoke ("track.add");
            const auto id = f.clipId();

            f.invoke ("clip.move", clipMoveArgs (id, 1.0, f.trackId (1)));

            auto tracks = f.model.getTracks();
            expect (tracks[0].clips.empty());
            expectEquals ((int) tracks[1].clips.size(), 1);
            expectEquals (tracks[1].clips[0].id, id);
            expectWithinAbsoluteError (tracks[1].clips[0].startSeconds, 1.0, 1e-6);

            f.invoke ("edit.undo");
            tracks = f.model.getTracks();
            expectEquals ((int) tracks[0].clips.size(), 1);
            expect (tracks[1].clips.empty());
            expectWithinAbsoluteError (tracks[0].clips[0].startSeconds, 0.0, 1e-6);
        }

        beginTest ("clip.move clamps to the Edit start");
        {
            ClipFixture f;
            f.invoke ("clip.move", clipMoveArgs (f.clipId(), 2.0));
            f.invoke ("clip.move", clipMoveArgs (f.clipId(), -5.0));
            expectWithinAbsoluteError (f.clip().startSeconds, 0.0, 1e-6);
        }

        beginTest ("clip.move with an unknown clip or no change creates no undo step");
        {
            ClipFixture f;
            f.invoke ("clip.move", clipMoveArgs ("no-such-clip", 1.0));
            f.invoke ("clip.move", clipMoveArgs (f.clipId(), 0.0));
            f.invoke ("clip.move", {});

            f.invoke ("edit.undo");
            expect (f.model.getTracks()[0].clips.empty());   // straight back past clip.add
        }

        //==============================================================================
        beginTest ("clip.resize from the end shortens the clip; undo restores it");
        {
            ClipFixture f;
            f.invoke ("clip.resize", clipResizeArgs (f.clipId(), 0.0, 1.25));

            expectWithinAbsoluteError (f.clip().startSeconds, 0.0, 1e-6);
            expectWithinAbsoluteError (f.clip().lengthSeconds, 1.25, 1e-6);
            expectWithinAbsoluteError (f.clip().sourceOffsetSeconds, 0.0, 1e-6);

            f.invoke ("edit.undo");
            expectWithinAbsoluteError (f.clip().lengthSeconds, 2.0, 1e-3);
        }

        beginTest ("clip.resize from the start trims the front and keeps the audio in place");
        {
            ClipFixture f;
            f.invoke ("clip.resize", clipResizeArgs (f.clipId(), 0.5, 2.0));

            expectWithinAbsoluteError (f.clip().startSeconds, 0.5, 1e-6);
            expectWithinAbsoluteError (f.clip().lengthSeconds, 1.5, 1e-3);
            expectWithinAbsoluteError (f.clip().sourceOffsetSeconds, 0.5, 1e-6);
        }

        beginTest ("clip.resize cannot reach past either end of the source audio");
        {
            ClipFixture f;
            f.invoke ("clip.move", clipMoveArgs (f.clipId(), 1.0));
            f.invoke ("clip.resize", clipResizeArgs (f.clipId(), 1.5, 2.5));   // trimmed to source 0.5..1.5

            f.invoke ("clip.resize", clipResizeArgs (f.clipId(), 0.0, 9.0));

            expectWithinAbsoluteError (f.clip().startSeconds, 1.0, 1e-6);          // source start
            expectWithinAbsoluteError (f.clip().sourceOffsetSeconds, 0.0, 1e-6);
            expectWithinAbsoluteError (f.clip().lengthSeconds, 2.0, 1e-3);         // source end
        }

        beginTest ("clip.resize to an empty or inverted range changes nothing");
        {
            ClipFixture f;
            f.invoke ("clip.resize", clipResizeArgs (f.clipId(), 1.0, 1.0));
            f.invoke ("clip.resize", clipResizeArgs (f.clipId(), 1.5, 0.5));

            expectWithinAbsoluteError (f.clip().lengthSeconds, 2.0, 1e-3);
            f.invoke ("edit.undo");
            expect (f.model.getTracks()[0].clips.empty());
        }

        //==============================================================================
        beginTest ("clip.split cuts the selected clip at the playhead into two contiguous clips");
        {
            ClipFixture f;
            f.model.selectClip (f.clipId());
            f.setPlayhead (0.75);

            expect (f.commands.find ("clip.split")->isEnabled());
            f.invoke ("clip.split");

            auto clips = f.model.getTracks()[0].clips;
            expectEquals ((int) clips.size(), 2);
            expectWithinAbsoluteError (clips[0].startSeconds, 0.0, 1e-6);
            expectWithinAbsoluteError (clips[0].lengthSeconds, 0.75, 1e-6);
            expectWithinAbsoluteError (clips[1].startSeconds, 0.75, 1e-6);
            expectWithinAbsoluteError (clips[1].lengthSeconds, 1.25, 1e-3);
            expectWithinAbsoluteError (clips[1].sourceOffsetSeconds, 0.75, 1e-6);
            expect (clips[1].file == clips[0].file);

            f.invoke ("edit.undo");
            clips = f.model.getTracks()[0].clips;
            expectEquals ((int) clips.size(), 1);
            expectWithinAbsoluteError (clips[0].lengthSeconds, 2.0, 1e-3);
        }

        beginTest ("clip.split is disabled and does nothing without a selected clip under the playhead");
        {
            ClipFixture f;
            f.setPlayhead (0.75);
            expect (! f.commands.find ("clip.split")->isEnabled());
            f.invoke ("clip.split");

            f.model.selectClip (f.clipId());
            f.setPlayhead (5.0);
            expect (! f.commands.find ("clip.split")->isEnabled());
            f.invoke ("clip.split");

            f.setPlayhead (0.0005);   // within a millisecond of the edge: the engine won't cut
            expect (! f.commands.find ("clip.split")->isEnabled());
            f.invoke ("clip.split");

            expectEquals ((int) f.model.getTracks()[0].clips.size(), 1);
            f.invoke ("edit.undo");
            expect (f.model.getTracks()[0].clips.empty());
        }
    }
};

static ClipEditingTests clipEditingTests;

} // namespace papercut::test
