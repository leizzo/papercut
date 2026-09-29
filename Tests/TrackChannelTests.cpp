#include "TestFixture.h"

namespace resamper::test
{

struct TrackChannelTests : juce::UnitTest
{
    TrackChannelTests() : juce::UnitTest ("Track Channel Controls", "Resamper") {}

    /** Two fresh tracks; the undo history holds their two track.add steps. */
    struct ChannelFixture : Fixture
    {
        ChannelFixture()
        {
            invoke ("track.add");
            invoke ("track.add");
        }

        TrackInfo track (int index = 0) const        { return model.getTracks()[(size_t) index]; }
        juce::String trackId (int index = 0) const   { return track (index).id; }

        /** Undoes one step; returns false if there was none. */
        bool undoOnce()   { const bool could = model.canUndo(); invoke ("edit.undo"); return could; }
    };

    void runTest() override
    {
        beginTest ("A new track sits at 0 dB, centred, unmuted and not soloed");
        {
            ChannelFixture f;
            expectWithinAbsoluteError (f.track().volumeDb, 0.0, 1e-3);
            expectWithinAbsoluteError (f.track().pan, 0.0, 1e-6);
            expect (! f.track().muted);
            expect (! f.track().solo);
        }

        //==============================================================================
        beginTest ("track.setVolume sets one track's volume as one undo step");
        {
            ChannelFixture f;
            f.invoke ("track.setVolume", trackVolumeArgs (f.trackId (1), -12.0));

            expectWithinAbsoluteError (f.track (1).volumeDb, -12.0, 1e-3);
            expectWithinAbsoluteError (f.track (0).volumeDb, 0.0, 1e-3);

            f.invoke ("edit.undo");
            expectWithinAbsoluteError (f.track (1).volumeDb, 0.0, 1e-3);
            expectEquals (f.numTracks(), 2);   // only the volume change was undone

            f.invoke ("edit.redo");
            expectWithinAbsoluteError (f.track (1).volumeDb, -12.0, 1e-3);
        }

        beginTest ("track.setVolume clamps to the fader range; -inf is silence");
        {
            ChannelFixture f;
            f.invoke ("track.setVolume", trackVolumeArgs (f.trackId(), 40.0));
            expectWithinAbsoluteError (f.track().volumeDb, ApplicationModel::maxVolumeDb, 1e-3);

            f.invoke ("track.setVolume", trackVolumeArgs (f.trackId(), -500.0));
            expectWithinAbsoluteError (f.track().volumeDb, ApplicationModel::minVolumeDb, 1e-3);
        }

        beginTest ("A volume change that changes nothing records no undo step");
        {
            ChannelFixture f;
            f.undoOnce();   // back to one track, so the only step left is its track.add
            f.invoke ("track.setVolume", trackVolumeArgs (f.trackId(), 0.0));
            f.invoke ("track.setPan", trackPanArgs (f.trackId(), 0.003));   // the engine snaps it to centre
            f.invoke ("track.setVolume", trackVolumeArgs ("no-such-track", -6.0));
            f.invoke ("track.setVolume");   // no args at all

            expect (f.undoOnce());
            expectEquals (f.numTracks(), 0);
        }

        beginTest ("A fader gesture is one undo step, however many values it sends");
        {
            ChannelFixture f;
            f.invoke ("track.setVolume", trackVolumeArgs (f.trackId(), -1.0));
            f.invoke ("track.setVolume", trackVolumeArgs (f.trackId(), -3.0, true));
            f.invoke ("track.setVolume", trackVolumeArgs (f.trackId(), -9.0, true));
            expectWithinAbsoluteError (f.track().volumeDb, -9.0, 1e-3);

            f.invoke ("edit.undo");
            expectWithinAbsoluteError (f.track().volumeDb, 0.0, 1e-3);
            expectEquals (f.numTracks(), 2);
        }

        beginTest ("Separate gestures are separate undo steps");
        {
            ChannelFixture f;
            f.invoke ("track.setVolume", trackVolumeArgs (f.trackId(), -3.0));
            f.invoke ("track.setVolume", trackVolumeArgs (f.trackId(), -6.0));

            f.invoke ("edit.undo");
            expectWithinAbsoluteError (f.track().volumeDb, -3.0, 1e-3);
        }

        beginTest ("A continuing gesture never merges into an unrelated undo step");
        {
            ChannelFixture f;

            // Continuing after another track's gesture: its own step.
            f.invoke ("track.setVolume", trackVolumeArgs (f.trackId (1), -3.0));
            f.invoke ("track.setVolume", trackVolumeArgs (f.trackId (0), -6.0, true));
            f.invoke ("edit.undo");
            expectWithinAbsoluteError (f.track (0).volumeDb, 0.0, 1e-3);
            expectWithinAbsoluteError (f.track (1).volumeDb, -3.0, 1e-3);

            // Continuing after a different edit: its own step.
            f.invoke ("track.add");
            f.invoke ("track.setVolume", trackVolumeArgs (f.trackId (1), -6.0, true));
            f.invoke ("edit.undo");
            expectEquals (f.numTracks(), 3);

            // Continuing after an undo: its own step.
            f.invoke ("track.setVolume", trackVolumeArgs (f.trackId (1), -6.0));
            f.invoke ("edit.undo");
            f.invoke ("track.setVolume", trackVolumeArgs (f.trackId (1), -9.0, true));
            f.invoke ("edit.undo");
            expectWithinAbsoluteError (f.track (1).volumeDb, -3.0, 1e-3);
        }

        //==============================================================================
        beginTest ("track.setPan pans one track as one undo step, clamped to [-1, 1]");
        {
            ChannelFixture f;
            f.invoke ("track.setPan", trackPanArgs (f.trackId (1), -0.5));
            expectWithinAbsoluteError (f.track (1).pan, -0.5, 1e-6);
            expectWithinAbsoluteError (f.track (0).pan, 0.0, 1e-6);

            f.invoke ("track.setPan", trackPanArgs (f.trackId (1), 3.0));
            expectWithinAbsoluteError (f.track (1).pan, 1.0, 1e-6);

            f.invoke ("edit.undo");
            expectWithinAbsoluteError (f.track (1).pan, -0.5, 1e-6);
        }

        beginTest ("A pan gesture is one undo step, separate from a volume gesture");
        {
            ChannelFixture f;
            f.invoke ("track.setVolume", trackVolumeArgs (f.trackId(), -6.0));
            f.invoke ("track.setPan", trackPanArgs (f.trackId(), 0.2, true));
            f.invoke ("track.setPan", trackPanArgs (f.trackId(), 0.4, true));

            f.invoke ("edit.undo");
            expectWithinAbsoluteError (f.track().pan, 0.0, 1e-6);
            expectWithinAbsoluteError (f.track().volumeDb, -6.0, 1e-3);
        }

        //==============================================================================
        beginTest ("track.toggleMute and track.toggleSolo flip one track and are never undoable");
        {
            ChannelFixture f;
            f.invoke ("track.toggleMute", trackArgs (f.trackId (1)));
            f.invoke ("track.toggleSolo", trackArgs (f.trackId (0)));

            expect (f.track (1).muted);
            expect (! f.track (0).muted);
            expect (f.track (0).solo);
            expect (! f.track (1).solo);

            f.invoke ("edit.undo");   // undoes the second track.add, not mute or solo
            expectEquals (f.numTracks(), 1);
            expect (f.track (0).solo);

            f.invoke ("track.toggleSolo", trackArgs (f.trackId (0)));
            expect (! f.track (0).solo);
        }

        beginTest ("Removing and restoring a track keeps its channel settings");
        {
            ChannelFixture f;
            f.invoke ("track.setVolume", trackVolumeArgs (f.trackId (1), -6.0));
            f.invoke ("track.setPan", trackPanArgs (f.trackId (1), 0.5));
            f.invoke ("track.toggleMute", trackArgs (f.trackId (1)));

            f.invoke ("track.remove");
            f.invoke ("edit.undo");

            expectWithinAbsoluteError (f.track (1).volumeDb, -6.0, 1e-3);
            expectWithinAbsoluteError (f.track (1).pan, 0.5, 1e-6);
            expect (f.track (1).muted);
        }

        beginTest ("Undo never clears the redo history, whatever the channel settings");
        {
            ChannelFixture f;
            f.invoke ("track.remove");   // a track with default settings
            f.invoke ("edit.undo");
            expect (f.model.canRedo());

            f.invoke ("track.setPan", trackPanArgs (f.trackId(), 0.5));   // the first change from a default
            f.invoke ("edit.undo");
            expect (f.model.canRedo());
            f.invoke ("edit.redo");
            expectWithinAbsoluteError (f.track().pan, 0.5, 1e-6);
        }
    }
};

static TrackChannelTests trackChannelTests;

} // namespace resamper::test
