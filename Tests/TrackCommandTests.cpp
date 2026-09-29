#include "TestFixture.h"

namespace resamper::test
{

struct TrackCommandTests : juce::UnitTest
{
    TrackCommandTests() : juce::UnitTest ("Track Commands", "Resamper") {}

    void runTest() override
    {
        beginTest ("A New Project holds an empty Edit with no undo history");
        {
            Fixture f;
            expectEquals (f.numTracks(), 0);
            expect (! f.model.canUndo());
        }

        beginTest ("track.add and track.remove mutate the Edit's track list");
        {
            Fixture f;
            expect (f.invoke ("track.add"));
            expect (f.invoke ("track.add"));
            expectEquals (f.numTracks(), 2);

            expect (f.invoke ("track.remove"));
            expectEquals (f.numTracks(), 1);
        }

        beginTest ("track.remove removes the selected track");
        {
            Fixture f;
            f.invoke ("track.add");
            f.invoke ("track.add");
            auto first = f.model.getTracks()[0].id;
            auto second = f.model.getTracks()[1].id;

            f.model.selectTrack (first);
            f.invoke ("track.remove");

            expectEquals (f.numTracks(), 1);
            expectEquals (f.model.getTracks()[0].id, second);
        }

        beginTest ("track.remove on an empty Edit changes nothing");
        {
            Fixture f;
            f.invoke ("track.remove");
            expectEquals (f.numTracks(), 0);
            expect (! f.model.canUndo());
        }

        beginTest ("edit.undo / edit.redo reverse and replay track.add, one step per Command");
        {
            Fixture f;
            f.invoke ("track.add");
            f.invoke ("track.add");

            f.invoke ("edit.undo");
            expectEquals (f.numTracks(), 1);
            f.invoke ("edit.undo");
            expectEquals (f.numTracks(), 0);
            expect (! f.model.canUndo());

            f.invoke ("edit.redo");
            expectEquals (f.numTracks(), 1);
            f.invoke ("edit.redo");
            expectEquals (f.numTracks(), 2);
            expect (! f.model.canRedo());
        }

        beginTest ("edit.undo restores a removed track with its identity");
        {
            Fixture f;
            f.invoke ("track.add");
            auto before = f.model.getTracks()[0];

            f.invoke ("track.remove");
            f.invoke ("edit.undo");

            expectEquals (f.numTracks(), 1);
            expectEquals (f.model.getTracks()[0].id, before.id);
            expectEquals (f.model.getTracks()[0].name, before.name);
        }

        beginTest ("Selection never creates undo steps");
        {
            Fixture f;
            f.invoke ("track.add");
            auto id = f.model.getTracks()[0].id;

            f.model.selectTrack (id);
            expect (f.model.getTracks()[0].selected);

            // The only undo step is the track.add itself.
            f.invoke ("edit.undo");
            expectEquals (f.numTracks(), 0);
            expect (! f.model.canUndo());
        }

        beginTest ("New tracks take the next colour of the track palette; track.setColour changes it, one undo step");
        {
            Fixture f;
            f.invoke ("track.add");
            f.invoke ("track.addMidi");
            f.invoke ("track.add");
            auto tracks = f.model.getTracks();
            expectEquals (tracks[0].colourIndex, 0);
            expectEquals (tracks[1].colourIndex, 1);
            expectEquals (tracks[2].colourIndex, 2);

            f.invoke ("track.setColour", trackColourArgs (tracks[1].id, 5));
            expectEquals (f.model.getTracks()[1].colourIndex, 5);

            f.invoke ("track.setColour", trackColourArgs (tracks[1].id, 99));   // out of the palette: refused
            expectEquals (f.model.getTracks()[1].colourIndex, 5);

            f.invoke ("edit.undo");
            expectEquals (f.model.getTracks()[1].colourIndex, 1);
        }
    }
};

static TrackCommandTests trackCommandTests;

} // namespace resamper::test
