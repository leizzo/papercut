#include "TestFixture.h"

namespace papercut::test
{

struct TrackCommandTests : juce::UnitTest
{
    TrackCommandTests() : juce::UnitTest ("Track Commands", "Papercut") {}

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
    }
};

static TrackCommandTests trackCommandTests;

} // namespace papercut::test
