#include "TestFixture.h"

namespace papercut::test
{

struct TransportCommandTests : juce::UnitTest
{
    TransportCommandTests() : juce::UnitTest ("Transport Commands", "Papercut") {}

    void runTest() override
    {
        beginTest ("transport.play puts the transport into the playing state; transport.stop halts it");
        {
            Fixture f;
            expect (! f.model.isPlaying());

            expect (f.invoke ("transport.play"));
            expect (f.model.isPlaying());

            expect (f.invoke ("transport.stop"));
            expect (! f.model.isPlaying());
        }

        beginTest ("transport.togglePlay alternates play and stop");
        {
            Fixture f;
            f.invoke ("transport.togglePlay");
            expect (f.model.isPlaying());
            f.invoke ("transport.togglePlay");
            expect (! f.model.isPlaying());
        }

        beginTest ("transport.setPosition places the playhead, and transport.play starts there");
        {
            Fixture f;
            expect (f.invoke ("transport.setPosition", transportPositionArgs (3.5)));
            expectWithinAbsoluteError (f.model.getTransportPositionSeconds(), 3.5, 0.001);

            expect (f.invoke ("transport.setPosition", transportPositionArgs (-1.0)));
            expectWithinAbsoluteError (f.model.getTransportPositionSeconds(), 0.0, 0.001);

            expect (f.invoke ("transport.setPosition", transportPositionArgs (3.5)));
            expect (f.invoke ("transport.play"));
            expect (f.model.isPlaying());
            expectGreaterThan (f.model.getTransportPositionSeconds(), 3.0);
        }

        beginTest ("Transport Commands never create undo steps");
        {
            Fixture f;
            f.invoke ("track.add");
            f.invoke ("transport.setPosition", transportPositionArgs (1.0));

            for (auto id : { "transport.play", "transport.stop", "transport.togglePlay",
                             "transport.togglePlay", "transport.returnToStart" })
                f.invoke (id);

            // Exactly one step on the stack: the track.add.
            f.invoke ("edit.undo");
            expectEquals (f.numTracks(), 0);
            expect (! f.model.canUndo());
        }
    }
};

static TransportCommandTests transportCommandTests;

} // namespace papercut::test
