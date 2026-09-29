#include "TestFixture.h"
#include "Commands/TapTempo.h"

namespace resamper::test
{

struct TransportCommandTests : juce::UnitTest
{
    TransportCommandTests() : juce::UnitTest ("Transport Commands", "Resamper") {}

    void runTest() override
    {
        beginTest ("transport.play puts the transport into the playing state; transport.stop halts it");
        {
            Fixture f;
            expect (! f.model.isPlaying());

            expect (f.invoke (cmd::transportPlay));
            expect (f.model.isPlaying());

            expect (f.invoke (cmd::transportStop));
            expect (! f.model.isPlaying());
        }

        beginTest ("transport.togglePlay alternates play and stop");
        {
            Fixture f;
            f.invoke (cmd::transportTogglePlay);
            expect (f.model.isPlaying());
            f.invoke (cmd::transportTogglePlay);
            expect (! f.model.isPlaying());
        }

        beginTest ("transport.setPosition places the playhead, and transport.play starts there");
        {
            Fixture f;
            expect (f.invoke (cmd::transportSetPosition, 3.5));
            expectWithinAbsoluteError (f.model.getTransportPositionSeconds(), 3.5, 0.001);

            expect (f.invoke (cmd::transportSetPosition, -1.0));
            expectWithinAbsoluteError (f.model.getTransportPositionSeconds(), 0.0, 0.001);

            expect (f.invoke (cmd::transportSetPosition, 3.5));
            expect (f.invoke (cmd::transportPlay));
            expect (f.model.isPlaying());
            expectGreaterThan (f.model.getTransportPositionSeconds(), 3.0);
        }

        beginTest ("Transport Commands never create undo steps");
        {
            Fixture f;
            f.invoke (cmd::trackAdd);
            f.invoke (cmd::transportSetPosition, 1.0);

            for (auto command : { cmd::transportPlay, cmd::transportStop, cmd::transportTogglePlay,
                                  cmd::transportTogglePlay, cmd::transportReturnToStart })
                f.invoke (command);

            // Exactly one step on the stack: the track.add.
            f.invoke (cmd::editUndo);
            expectEquals (f.numTracks(), 0);
            expect (! f.model.canUndo());
        }

        beginTest ("Play while stopped starts from the insert marker, not where Stop left the playhead (§6.1)");
        {
            Fixture f;
            f.invoke (cmd::transportSetPosition, 2.0);
            f.invoke (cmd::transportPlay);
            juce::Thread::sleep (50);
            f.invoke (cmd::transportStop);
            f.invoke (cmd::transportPlay);
            expectWithinAbsoluteError (f.model.getTransportPositionSeconds(), 2.0, 0.05);
            f.invoke (cmd::transportStop);
        }

        beginTest ("Play while stopped with the loop on starts from the loop start");
        {
            Fixture f;
            f.invoke (cmd::transportSetLoopRange, { 4.0, 8.0 });
            f.invoke (cmd::transportSetPosition, 1.0);
            f.invoke (cmd::transportPlay);
            expectWithinAbsoluteError (f.model.getTransportPositionSeconds(), 4.0, 0.05);
            f.invoke (cmd::transportStop);
        }

        beginTest ("Play while playing restarts from the insert marker");
        {
            Fixture f;
            f.invoke (cmd::transportSetPosition, 1.0);
            f.invoke (cmd::transportPlay);
            juce::Thread::sleep (100);
            f.invoke (cmd::transportPlay);
            expect (f.model.isPlaying());
            expectWithinAbsoluteError (f.model.getTransportPositionSeconds(), 1.0, 0.05);
            f.invoke (cmd::transportStop);
        }

        beginTest ("Stop once keeps the position; Stop again returns to the start");
        {
            Fixture f;
            f.invoke (cmd::transportSetPosition, 3.0);
            f.invoke (cmd::transportPlay);
            f.invoke (cmd::transportStop);
            expectGreaterOrEqual (f.model.getTransportPositionSeconds(), 3.0);

            f.invoke (cmd::transportStop);
            expectWithinAbsoluteError (f.model.getTransportPositionSeconds(), 0.0, 0.001);
            expectWithinAbsoluteError (f.model.getInsertMarkerSeconds(), 0.0, 0.001);
        }

        beginTest ("transport.setTempo is one undo step; a drag's later values join it");
        {
            Fixture f;
            const auto before = f.model.getTempo();
            expect (f.invoke (cmd::transportSetTempo, { 124.0 }));
            expectWithinAbsoluteError (f.model.getTempo(), 124.0, 1.0e-6);

            f.invoke (cmd::transportSetTempo, { 125.0 });
            f.invoke (cmd::transportSetTempo, { 126.0, true });
            f.invoke (cmd::transportSetTempo, { 127.5, true });
            expectWithinAbsoluteError (f.model.getTempo(), 127.5, 1.0e-6);

            f.invoke (cmd::editUndo);
            expectWithinAbsoluteError (f.model.getTempo(), 124.0, 1.0e-6);
            f.invoke (cmd::editUndo);
            expectWithinAbsoluteError (f.model.getTempo(), before, 1.0e-6);
        }

        beginTest ("Tempo is clamped to the engine's range");
        {
            Fixture f;
            f.invoke (cmd::transportSetTempo, { 5000.0 });
            expectWithinAbsoluteError (f.model.getTempo(), ApplicationModel::maxTempo, 1.0e-6);
            f.invoke (cmd::transportSetTempo, { 1.0 });
            expectWithinAbsoluteError (f.model.getTempo(), ApplicationModel::minTempo, 1.0e-6);
        }

        beginTest ("transport.setTimeSignature changes the bar and is one undo step");
        {
            Fixture f;
            expectEquals (f.model.getTimeSignature().numerator, 4);
            f.invoke (cmd::transportSetTimeSignature, { 6, 8 });
            expectEquals (f.model.getTimeSignature().numerator, 6);
            expectEquals (f.model.getTimeSignature().denominator, 8);

            f.invoke (cmd::transportSetTimeSignature, { 0, 3 });   // refused
            expectEquals (f.model.getTimeSignature().numerator, 6);

            f.invoke (cmd::editUndo);
            expectEquals (f.model.getTimeSignature().numerator, 4);
            expectEquals (f.model.getTimeSignature().denominator, 4);
        }

        beginTest ("Positions read as bars.beats.sixteenths from 1.1.1");
        {
            Fixture f;
            f.invoke (cmd::transportSetTempo, { 120.0 });
            auto p = f.model.toBarsBeats (0.0);
            expectEquals (p.bar, 1);
            expectEquals (p.beat, 1);
            expectEquals (p.sixteenth, 1);

            // 120 BPM in 4/4: a bar is 2 s, a beat 0.5 s, a sixteenth 0.125 s.
            p = f.model.toBarsBeats (2.0 + 0.5 + 0.125);
            expectEquals (p.bar, 2);
            expectEquals (p.beat, 2);
            expectEquals (p.sixteenth, 2);
        }

        beginTest ("Before the start (a count-in), bars count back from 1: 0, -1, ...");
        {
            Fixture f;
            f.invoke (cmd::transportSetTempo, { 120.0 });

            auto p = f.model.toBarsBeats (-0.5);   // the last beat before bar 1
            expectEquals (p.bar, 0);
            expectEquals (p.beat, 4);
            expectEquals (p.sixteenth, 1);

            p = f.model.toBarsBeats (-4.0);        // two bars before
            expectEquals (p.bar, -1);
            expectEquals (p.beat, 1);
            expectEquals (p.sixteenth, 1);
        }

        beginTest ("transport.toggleMetronome switches the click, outside undo");
        {
            Fixture f;
            const auto was = f.model.isMetronomeOn();
            f.invoke (cmd::transportToggleMetronome);
            expect (f.model.isMetronomeOn() != was);
            expect (! f.model.canUndo());
        }

        beginTest ("Tap tempo averages the last taps and starts over after a pause");
        {
            TapTempo tap;
            expectEquals (tap.tap (10.0), 0.0);
            expectWithinAbsoluteError (tap.tap (10.5), 120.0, 1.0e-6);
            expectWithinAbsoluteError (tap.tap (11.0), 120.0, 1.0e-6);
            expectWithinAbsoluteError (tap.tap (11.6), 60.0 / ((0.5 + 0.5 + 0.6) / 3.0), 1.0e-6);

            // A long pause starts a new count.
            expectEquals (tap.tap (20.0), 0.0);
            expectWithinAbsoluteError (tap.tap (20.4), 150.0, 1.0e-6);
        }
    }
};

static TransportCommandTests transportCommandTests;

} // namespace resamper::test
