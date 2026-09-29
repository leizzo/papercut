#include "TestFixture.h"
#include "Commands/MixerCommands.h"

#include <tracktion_engine/tracktion_engine.h>

namespace resamper::test
{

struct MixerTests : juce::UnitTest
{
    MixerTests() : juce::UnitTest ("Mixer", "Resamper") {}

    struct MixerFixture : Fixture
    {
        bool trackIsInABus (const juce::String& trackId) const
        {
            for (auto& bus : mixer.getBuses())
                for (auto& child : bus.childTrackIds)
                    if (child == trackId)
                        return true;

            return false;
        }

        std::optional<Strip> strip (const juce::String& id) const
        {
            for (auto& s : mixer.getStrips())
                if (s.id == id)
                    return s;

            return std::nullopt;
        }

        juce::String busId (const juce::String& name) const
        {
            for (auto& bus : mixer.getBuses())
                if (bus.name == name)
                    return bus.trackId;

            return {};
        }

        /** Nests any track in a folder through the engine: the Mixer only moves audio tracks. */
        void nest (const juce::String& trackId, const juce::String& folderId)
        {
            auto& edit = projects.getEdit();
            auto* track = tracktion::findTrackForID (edit, tracktion::EditItemID::fromString (trackId));
            auto* folder = tracktion::findTrackForID (edit, tracktion::EditItemID::fromString (folderId));
            auto children = folder->getAllSubTracks (false);
            edit.moveTrack (track, tracktion::TrackInsertPoint (folder, children.isEmpty() ? nullptr : children.getLast()));
        }
    };

    /** An audio track, a return on bus 0, and one send from the audio track. */
    struct SendFixture : MixerFixture
    {
        juce::String sourceId, sendId;

        SendFixture()
        {
            invoke (cmd::trackAdd);
            invoke (cmd::mixerAddReturn);
            sourceId = model.getTracks()[0].id;
            invoke (cmd::mixerAddSend, { sourceId, mixer.getReturns()[0].bus });
            sendId = mixer.getSends (sourceId)[0].id;
        }
    };

    void runTest() override
    {
        beginTest ("addReturn creates a track that getReturns lists on bus 0 and TrackInfo marks as a return; undo removes it");
        {
            MixerFixture f;
            f.invoke (cmd::mixerAddReturn);

            auto returns = f.mixer.getReturns();
            expectEquals ((int) returns.size(), 1);
            expectEquals (returns[0].bus, 0);
            expectEquals (returns[0].name, juce::String ("Return"));
            expectEquals (f.numTracks(), 1);
            expectEquals (f.model.getTracks()[0].id, returns[0].trackId);
            expect (f.model.getTracks()[0].isReturn);

            f.invoke (cmd::editUndo);
            expectEquals ((int) f.mixer.getReturns().size(), 0);
            expectEquals (f.numTracks(), 0);
        }

        beginTest ("addSend lists one send; setSendGain is one undo step and a drag is one step");
        {
            SendFixture f;
            expectEquals ((int) f.mixer.getSends (f.sourceId).size(), 1);
            expectEquals (f.mixer.getSends (f.sourceId)[0].bus, 0);

            f.invoke (cmd::mixerSetSendGain, { f.sourceId, f.sendId, -12.0 });
            expectWithinAbsoluteError (f.mixer.getSends (f.sourceId)[0].gainDb, -12.0, 1.0e-2);

            f.invoke (cmd::editUndo);
            expectWithinAbsoluteError (f.mixer.getSends (f.sourceId)[0].gainDb, 0.0, 1.0e-2);
            expectEquals ((int) f.mixer.getSends (f.sourceId).size(), 1);

            f.invoke (cmd::mixerSetSendGain, { f.sourceId, f.sendId, -1.0 });
            f.invoke (cmd::mixerSetSendGain, { f.sourceId, f.sendId, -3.0, true });
            f.invoke (cmd::mixerSetSendGain, { f.sourceId, f.sendId, -9.0, true });
            expectWithinAbsoluteError (f.mixer.getSends (f.sourceId)[0].gainDb, -9.0, 1.0e-2);

            f.invoke (cmd::editUndo);
            expectWithinAbsoluteError (f.mixer.getSends (f.sourceId)[0].gainDb, 0.0, 1.0e-2);
            expectEquals ((int) f.mixer.getSends (f.sourceId).size(), 1);
        }

        beginTest ("addSend to a bus with no return fails and adds no undo step");
        {
            MixerFixture f;
            f.invoke (cmd::trackAdd);
            const auto trackId = f.model.getTracks()[0].id;
            expect (! f.model.getTracks()[0].isReturn);

            f.invoke (cmd::mixerAddSend, { trackId, 0 });

            expect (! f.errors.isEmpty());
            expect (f.mixer.getSends (trackId).empty());
            expectEquals (f.numTracks(), 1);

            f.invoke (cmd::editUndo);
            expectEquals (f.numTracks(), 0);
            expect (! f.model.canUndo());
        }

        beginTest ("addBus and moveTrackToBus nest the track; undo restores it");
        {
            MixerFixture f;
            f.invoke (cmd::trackAdd);
            const auto trackId = f.model.getTracks()[0].id;

            f.invoke (cmd::mixerAddBus, { "Drums" });
            auto buses = f.mixer.getBuses();
            expectEquals ((int) buses.size(), 1);
            expectEquals (buses[0].name, juce::String ("Drums"));
            expect (buses[0].childTrackIds.empty());

            f.invoke (cmd::mixerMoveToBus, { trackId, buses[0].trackId });
            expect (f.trackIsInABus (trackId));
            expectEquals (f.mixer.getBuses()[0].childTrackIds[0], trackId);

            for (int i = 0; i < 4 && f.trackIsInABus (trackId); ++i)
                f.invoke (cmd::editUndo);

            expect (! f.trackIsInABus (trackId));

            bool stillThere = false;

            for (auto& track : f.model.getTracks())
                stillThere = stillThere || track.id == trackId;

            expect (stillThere);
        }

        beginTest ("getStrips: tracks in Edit order, each Bus after its last child, then Returns; no Strip for a Folder-only Folder");
        {
            MixerFixture f;
            f.invoke (cmd::mixerAddReturn);   // first in the Edit, still last among the Strips

            for (int i = 0; i < 4; ++i)
                f.invoke (cmd::trackAdd);

            const auto tracks = f.model.getTracks();
            const auto returnId = tracks[0].id, t1 = tracks[1].id, t2 = tracks[2].id, t3 = tracks[3].id, t4 = tracks[4].id;

            f.invoke (cmd::mixerAddBus, { "Drums" });
            f.invoke (cmd::mixerAddBus, { "Kick" });
            const auto drums = f.busId ("Drums"), kick = f.busId ("Kick");

            // Return, t3, Drums { t1, Kick { t2 } }, Folder { t4 }
            f.invoke (cmd::mixerMoveToBus, { t1, drums });
            f.nest (kick, drums);
            f.invoke (cmd::mixerMoveToBus, { t2, kick });

            auto& edit = f.projects.getEdit();
            auto folder = edit.insertNewFolderTrack (tracktion::TrackInsertPoint::getEndOfTracks (edit), nullptr, false);
            f.nest (t4, folder->itemID.toString());

            juce::StringArray order, outputs;

            for (auto& s : f.mixer.getStrips())
            {
                order.add (s.name);
                outputs.add (s.output);
            }

            const auto name = [&] (const juce::String& id) { return f.strip (id)->name; };
            expectEquals (order.joinIntoString (","),
                          juce::StringArray { name (t3), name (t1), name (t2), "Kick", "Drums", name (t4), name (returnId) }.joinIntoString (","));
            expectEquals (outputs.joinIntoString (","), juce::String ("Master,Drums,Kick,Drums,Master,Master,Master"));

            expect (f.strip (drums)->role == StripRole::bus);
            expect (f.strip (returnId)->role == StripRole::returnTrack);
            expectEquals (f.strip (returnId)->returnLetter, juce::String ("A"));
            expectEquals (f.strip (returnId)->number, 0);
            expectEquals (f.strip (t3)->number, 1);
            expectEquals (f.strip (t2)->number, 3);
            expectEquals (f.strip (t4)->number, 4);
            expect (! f.strip (folder->itemID.toString()).has_value());
        }

        beginTest ("A Strip's Output follows moveTrackToBus, and undo puts it back");
        {
            MixerFixture f;
            f.invoke (cmd::trackAdd);
            const auto trackId = f.model.getTracks()[0].id;
            f.invoke (cmd::mixerAddBus, { "Drums" });

            expectEquals (f.strip (trackId)->output, juce::String ("Master"));
            f.invoke (cmd::mixerMoveToBus, { trackId, f.busId ("Drums") });
            expectEquals (f.strip (trackId)->output, juce::String ("Drums"));

            f.invoke (cmd::editUndo);
            expectEquals (f.strip (trackId)->output, juce::String ("Master"));
        }

        beginTest ("A Strip carries its fader, sends and inserts");
        {
            SendFixture f;
            f.invoke (cmd::trackSetVolume, { f.sourceId, -6.0, false });

            const auto strip = f.strip (f.sourceId);
            expect (strip.has_value());
            expectWithinAbsoluteError (strip->volumeDb, -6.0, 1.0e-2);
            expectEquals ((int) strip->sends.size(), 1);
            expectEquals (strip->sends[0].id, f.sendId);
            expect (strip->inserts.empty());
        }

        beginTest ("setMasterVolume changes the master only; undo restores it");
        {
            MixerFixture f;
            f.invoke (cmd::trackAdd);
            const auto trackId = f.model.getTracks()[0].id;
            expectWithinAbsoluteError (f.model.getTracks()[0].volumeDb, 0.0, 1.0e-3);

            // The engine's Edit starts the master fader at -3 dB, not 0.
            const auto initialMaster = f.mixer.getMaster().volumeDb;

            f.invoke (cmd::mixerSetMasterVolume, { -6.0 });
            f.invoke (cmd::mixerSetMasterVolume, { -9.0, true });
            f.invoke (cmd::mixerSetMasterVolume, { -12.0, true });

            expectWithinAbsoluteError (f.mixer.getMaster().volumeDb, -12.0, 1.0e-2);
            expectWithinAbsoluteError (f.model.getTracks()[0].volumeDb, 0.0, 1.0e-3);

            f.invoke (cmd::editUndo);
            expectWithinAbsoluteError (f.mixer.getMaster().volumeDb, initialMaster, 1.0e-2);
            expectWithinAbsoluteError (f.model.getTracks()[0].volumeDb, 0.0, 1.0e-3);
            expectEquals (f.model.getTracks()[0].id, trackId);
        }

        beginTest ("getTrackLevel is silence when the track has not played");
        {
            MixerFixture f;
            f.invoke (cmd::trackAdd);
            const auto level = f.mixer.getTrackLevel (f.model.getTracks()[0].id);
            expectWithinAbsoluteError ((double) level.left, ApplicationModel::minVolumeDb, 1.0e-3);
            expectWithinAbsoluteError ((double) level.right, ApplicationModel::minVolumeDb, 1.0e-3);
            expectWithinAbsoluteError ((double) f.mixer.getMasterLevel().left, ApplicationModel::minVolumeDb, 1.0e-3);
        }

        beginTest ("Send mute is one undo step, because the engine records the gain");
        {
            SendFixture f;
            f.invoke (cmd::mixerSetSendMuted, { f.sourceId, f.sendId, true });
            expect (f.mixer.getSends (f.sourceId)[0].muted);

            f.invoke (cmd::editUndo);
            expect (! f.mixer.getSends (f.sourceId)[0].muted);
            expectWithinAbsoluteError (f.mixer.getSends (f.sourceId)[0].gainDb, 0.0, 1.0e-2);
            expectEquals ((int) f.mixer.getSends (f.sourceId).size(), 1);
        }
    }
};

static MixerTests mixerTests;

} // namespace resamper::test
