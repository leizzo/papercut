#include "TestFixture.h"
#include "Commands/MixerCommands.h"

namespace papercut::test
{

struct MixerTests : juce::UnitTest
{
    MixerTests() : juce::UnitTest ("Mixer", "Papercut") {}

    struct MixerFixture : Fixture
    {
        Mixer mixer { projects };

        MixerFixture()
        {
            registerMixerCommands (commands, mixer, host);
        }

        bool trackIsInABus (const juce::String& trackId) const
        {
            for (auto& bus : mixer.getBuses())
                for (auto& child : bus.childTrackIds)
                    if (child == trackId)
                        return true;

            return false;
        }
    };

    /** An audio track, a return on bus 0, and one send from the audio track. */
    struct SendFixture : MixerFixture
    {
        juce::String sourceId, sendId;

        SendFixture()
        {
            invoke ("track.add");
            invoke ("mixer.addReturn");
            sourceId = model.getTracks()[0].id;
            invoke ("mixer.addSend", sendArgs (sourceId, mixer.getReturns()[0].bus));
            sendId = mixer.getSends (sourceId)[0].id;
        }
    };

    void runTest() override
    {
        beginTest ("addReturn creates a track that getReturns lists on bus 0; undo removes it");
        {
            MixerFixture f;
            f.invoke ("mixer.addReturn");

            auto returns = f.mixer.getReturns();
            expectEquals ((int) returns.size(), 1);
            expectEquals (returns[0].bus, 0);
            expectEquals (returns[0].name, juce::String ("Return"));
            expectEquals (f.numTracks(), 1);
            expectEquals (f.model.getTracks()[0].id, returns[0].trackId);

            f.invoke ("edit.undo");
            expectEquals ((int) f.mixer.getReturns().size(), 0);
            expectEquals (f.numTracks(), 0);
        }

        beginTest ("addSend lists one send; setSendGain is one undo step and a drag is one step");
        {
            SendFixture f;
            expectEquals ((int) f.mixer.getSends (f.sourceId).size(), 1);
            expectEquals (f.mixer.getSends (f.sourceId)[0].bus, 0);

            f.invoke ("mixer.setSendGain", sendGainArgs (f.sourceId, f.sendId, -12.0));
            expectWithinAbsoluteError (f.mixer.getSends (f.sourceId)[0].gainDb, -12.0, 1.0e-2);

            f.invoke ("edit.undo");
            expectWithinAbsoluteError (f.mixer.getSends (f.sourceId)[0].gainDb, 0.0, 1.0e-2);
            expectEquals ((int) f.mixer.getSends (f.sourceId).size(), 1);

            f.invoke ("mixer.setSendGain", sendGainArgs (f.sourceId, f.sendId, -1.0));
            f.invoke ("mixer.setSendGain", sendGainArgs (f.sourceId, f.sendId, -3.0, true));
            f.invoke ("mixer.setSendGain", sendGainArgs (f.sourceId, f.sendId, -9.0, true));
            expectWithinAbsoluteError (f.mixer.getSends (f.sourceId)[0].gainDb, -9.0, 1.0e-2);

            f.invoke ("edit.undo");
            expectWithinAbsoluteError (f.mixer.getSends (f.sourceId)[0].gainDb, 0.0, 1.0e-2);
            expectEquals ((int) f.mixer.getSends (f.sourceId).size(), 1);
        }

        beginTest ("addSend to a bus with no return fails and adds no undo step");
        {
            MixerFixture f;
            f.invoke ("track.add");
            const auto trackId = f.model.getTracks()[0].id;

            f.invoke ("mixer.addSend", sendArgs (trackId, 0));

            expect (! f.errors.isEmpty());
            expect (f.mixer.getSends (trackId).empty());
            expectEquals (f.numTracks(), 1);

            f.invoke ("edit.undo");
            expectEquals (f.numTracks(), 0);
            expect (! f.model.canUndo());
        }

        beginTest ("addBus and moveTrackToBus nest the track; undo restores it");
        {
            MixerFixture f;
            f.invoke ("track.add");
            const auto trackId = f.model.getTracks()[0].id;

            f.invoke ("mixer.addBus", busArgs ("Drums"));
            auto buses = f.mixer.getBuses();
            expectEquals ((int) buses.size(), 1);
            expectEquals (buses[0].name, juce::String ("Drums"));
            expect (buses[0].childTrackIds.empty());

            f.invoke ("mixer.moveToBus", moveToBusArgs (trackId, buses[0].trackId));
            expect (f.trackIsInABus (trackId));
            expectEquals (f.mixer.getBuses()[0].childTrackIds[0], trackId);

            for (int i = 0; i < 4 && f.trackIsInABus (trackId); ++i)
                f.invoke ("edit.undo");

            expect (! f.trackIsInABus (trackId));

            bool stillThere = false;

            for (auto& track : f.model.getTracks())
                stillThere = stillThere || track.id == trackId;

            expect (stillThere);
        }

        beginTest ("setMasterVolume changes the master only; undo restores it");
        {
            MixerFixture f;
            f.invoke ("track.add");
            const auto trackId = f.model.getTracks()[0].id;
            expectWithinAbsoluteError (f.model.getTracks()[0].volumeDb, 0.0, 1.0e-3);

            // The engine's Edit starts the master fader at -3 dB, not 0.
            const auto initialMaster = f.mixer.getMaster().volumeDb;

            f.invoke ("mixer.setMasterVolume", masterVolumeArgs (-6.0));
            f.invoke ("mixer.setMasterVolume", masterVolumeArgs (-9.0, true));
            f.invoke ("mixer.setMasterVolume", masterVolumeArgs (-12.0, true));

            expectWithinAbsoluteError (f.mixer.getMaster().volumeDb, -12.0, 1.0e-2);
            expectWithinAbsoluteError (f.model.getTracks()[0].volumeDb, 0.0, 1.0e-3);

            f.invoke ("edit.undo");
            expectWithinAbsoluteError (f.mixer.getMaster().volumeDb, initialMaster, 1.0e-2);
            expectWithinAbsoluteError (f.model.getTracks()[0].volumeDb, 0.0, 1.0e-3);
            expectEquals (f.model.getTracks()[0].id, trackId);
        }

        beginTest ("getTrackLevel is silence when the track has not played");
        {
            MixerFixture f;
            f.invoke ("track.add");
            const auto level = f.mixer.getTrackLevel (f.model.getTracks()[0].id);
            expectWithinAbsoluteError ((double) level.left, ApplicationModel::minVolumeDb, 1.0e-3);
            expectWithinAbsoluteError ((double) level.right, ApplicationModel::minVolumeDb, 1.0e-3);
            expectWithinAbsoluteError ((double) f.mixer.getMasterLevel().left, ApplicationModel::minVolumeDb, 1.0e-3);
        }

        beginTest ("Send mute is one undo step, because the engine records the gain");
        {
            SendFixture f;
            f.invoke ("mixer.setSendMuted", sendMutedArgs (f.sourceId, f.sendId, true));
            expect (f.mixer.getSends (f.sourceId)[0].muted);

            f.invoke ("edit.undo");
            expect (! f.mixer.getSends (f.sourceId)[0].muted);
            expectWithinAbsoluteError (f.mixer.getSends (f.sourceId)[0].gainDb, 0.0, 1.0e-2);
            expectEquals ((int) f.mixer.getSends (f.sourceId).size(), 1);
        }
    }
};

static MixerTests mixerTests;

} // namespace papercut::test
