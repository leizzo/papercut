#include "TestFixture.h"
#include "Commands/SessionCommands.h"

#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion;

namespace resamper::test
{

/** launchSlot only queues. The audio thread is what marks a slot playing;
    this steps the LaunchHandle the same way, so a headless test can require it. */
static void markSlotPlaying (te::Edit& edit, const juce::String& clipId)
{
    for (auto* track : te::getAudioTracks (edit))
    {
        if (! track->state.getChildWithName (te::IDs::CLIPSLOTS).isValid())
            continue;

        for (auto* slot : track->getClipSlotList().getClipSlots())
        {
            auto* clip = slot != nullptr ? slot->getClip() : nullptr;

            if (clip == nullptr || clip->itemID.toString() != clipId)
                continue;

            auto handle = clip->getLaunchHandle();

            if (handle == nullptr)
                return;

            te::SyncRange sync;
            auto end = sync.end;
            const auto step = te::BeatDuration::fromBeats (0.5);
            end.monotonicBeat.v = end.monotonicBeat.v + step;
            end.beat = end.beat + step;
            handle->advance ({ sync.end, end });
            return;
        }
    }
}

struct SessionTests : juce::UnitTest
{
    SessionTests() : juce::UnitTest ("Session", "Resamper") {}

    void runTest() override
    {
        beginTest ("setSceneCount adds scenes and undo removes them");
        {
            Fixture f;
            auto& session = f.session;

            for (const char* id : { "session.setSceneCount", "session.renameScene", "session.addSlotClip",
                                    "session.addMidiSlotClip", "session.clearSlot", "session.launchSlot",
                                    "session.launchScene", "session.stopAll", "session.recordToArrangement" })
                expect (f.commands.contains (id));

            // The Project already has an empty SCENES node, created outside undo.
            // The scenes added here are the undoable part.
            expect (f.invoke (cmd::sessionSetSceneCount, 2));
            expectEquals ((int) session.getScenes().size(), 2);

            expect (f.invoke (cmd::sessionRenameScene, { 0, "Verse" }));
            expectEquals (session.getScenes()[0].name, juce::String ("Verse"));

            expect (f.invoke (cmd::sessionSetSceneCount, 2));   // already there: no new step
            f.invoke (cmd::editUndo);
            expect (session.getScenes()[0].name.isEmpty());

            f.invoke (cmd::editUndo);
            expectEquals ((int) session.getScenes().size(), 0);
        }

        beginTest ("addSlotClip stays out of the Arrangement and undo clears the slot");
        {
            Fixture f;
            auto& session = f.session;

            f.invoke (cmd::trackAdd);
            expect (session.setSceneCount (1).wasOk());

            const auto id = f.model.getTracks()[0].id;
            const auto wav = writeSineWav (f.scratchDir().getChildFile ("tone.wav"), 2.0);
            expect (session.addSlotClip (id, 0, wav).wasOk());

            auto slots = session.getSlots (id);
            expectEquals ((int) slots.size(), 1);
            expect (slots[0].hasClip);
            expect (slots[0].clipId.isNotEmpty());
            expect (f.model.getTracks()[0].clips.empty());
            expectEquals (session.getScenes()[0].occupiedSlots, 1);

            f.invoke (cmd::editUndo);
            expect (! session.getSlots (id)[0].hasClip);
            expect (f.model.getTracks()[0].clips.empty());
        }

        beginTest ("addSlotClip on a MIDI track fails and adds no undo step");
        {
            Fixture f;
            auto& session = f.session;

            f.invoke (cmd::trackAddMidi);
            const auto id = f.model.getTracks()[0].id;
            const auto wav = writeSineWav (f.scratchDir().getChildFile ("tone.wav"), 2.0);

            expect (f.model.getTracks()[0].kind == TrackKind::midi);
            expect (session.addSlotClip (id, 0, wav).failed());
            expect (session.getScenes().empty());

            f.invoke (cmd::editUndo);
            expectEquals (f.numTracks(), 0);
        }

        beginTest ("launchSlot queues one slot; launchScene queues only that row");
        {
            Fixture f;
            auto& session = f.session;

            f.invoke (cmd::trackAdd);
            f.invoke (cmd::trackAdd);
            expect (session.setSceneCount (2).wasOk());

            const auto wav = writeSineWav (f.scratchDir().getChildFile ("tone.wav"), 2.0);
            const auto a = f.model.getTracks()[0].id;
            const auto b = f.model.getTracks()[1].id;

            expect (session.addSlotClip (a, 0, wav).wasOk());
            expect (session.addSlotClip (b, 0, wav).wasOk());
            expect (session.addSlotClip (a, 1, wav).wasOk());

            expect (! session.launchScene (9));
            f.invoke (cmd::editUndo);
            expect (! session.getSlots (a)[1].hasClip);   // the failed launch recorded nothing

            expect (session.addSlotClip (a, 1, wav).wasOk());

            expect (session.launchSlot (a, 0));
            {
                const auto slot = session.getSlots (a)[0];
                expect (slot.queued || slot.playing);
            }

            expect (session.stopSlot (a, 0));
            {
                const auto slot = session.getSlots (a)[0];
                expect (! slot.queued);
                expect (! slot.playing);
            }

            expect (session.launchScene (0));
            const auto rowA = session.getSlots (a);
            const auto rowB = session.getSlots (b);
            expect (rowA[0].queued || rowA[0].playing);
            expect (rowB[0].queued || rowB[0].playing);
            expect (! (rowA[1].queued || rowA[1].playing));
            expect (! (rowB[1].queued || rowB[1].playing));

            expect (session.setSceneCount (3).wasOk());
            expect (! session.launchScene (2));
            f.invoke (cmd::editUndo);
            expectEquals ((int) session.getScenes().size(), 2);
        }

        beginTest ("addMidiSlotClip stays off the Arrangement; a WAV on a MIDI track still fails");
        {
            Fixture f;
            auto& session = f.session;

            f.invoke (cmd::trackAddMidi);
            expect (session.setSceneCount (1).wasOk());
            const auto id = f.model.getTracks()[0].id;

            expect (session.addMidiSlotClip (id, 0).wasOk());
            expect (session.getSlots (id)[0].hasClip);
            expect (f.model.getTracks()[0].clips.empty());

            f.invoke (cmd::editUndo);
            expect (! session.getSlots (id)[0].hasClip);

            f.invoke (cmd::trackAdd);
            const auto audioId = f.model.getTracks()[1].id;
            expect (session.addMidiSlotClip (audioId, 0).failed());
        }

        beginTest ("recordIntoArrangement places a playing slot at the playhead and skips a queued one");
        {
            Fixture f;
            auto& session = f.session;

            f.invoke (cmd::trackAdd);
            expect (session.setSceneCount (1).wasOk());

            const auto id = f.model.getTracks()[0].id;
            const auto wav = writeSineWav (f.scratchDir().getChildFile ("tone.wav"), 2.0);
            expect (session.addSlotClip (id, 0, wav).wasOk());

            expect (session.launchSlot (id, 0));
            expect (session.getSlots (id)[0].queued);
            expect (session.recordIntoArrangement().failed());
            f.invoke (cmd::editUndo);
            expect (! session.getSlots (id)[0].hasClip);   // the failed record recorded nothing
            expect (session.addSlotClip (id, 0, wav).wasOk());

            expect (f.model.setTransportPosition (1.25));
            expect (session.launchSlot (id, 0));
            markSlotPlaying (f.projects.getEdit(), session.getSlots (id)[0].clipId);
            expect (session.getSlots (id)[0].playing);
            expect (session.recordIntoArrangement().wasOk());

            auto clips = f.model.getTracks()[0].clips;
            expectEquals ((int) clips.size(), 1);
            expectWithinAbsoluteError (clips[0].startSeconds, 1.25, 1e-6);
            expectWithinAbsoluteError (clips[0].lengthSeconds, 2.0, 1e-3);
            expect (clips[0].file == wav);
            expect (session.getSlots (id)[0].hasClip);

            f.invoke (cmd::editUndo);
            expect (f.model.getTracks()[0].clips.empty());
            expect (session.getSlots (id)[0].hasClip);
        }

        beginTest ("recordIntoArrangement places a playing MIDI slot on the Arrangement");
        {
            Fixture f;
            auto& session = f.session;

            f.invoke (cmd::trackAddMidi);
            expect (session.setSceneCount (1).wasOk());
            const auto id = f.model.getTracks()[0].id;
            expect (session.addMidiSlotClip (id, 0).wasOk());
            expect (session.launchSlot (id, 0));
            markSlotPlaying (f.projects.getEdit(), session.getSlots (id)[0].clipId);
            expect (session.getSlots (id)[0].playing);

            expect (f.model.setTransportPosition (0.5));
            expect (session.recordIntoArrangement().wasOk());

            auto clips = f.model.getTracks()[0].clips;
            expectEquals ((int) clips.size(), 1);
            expect (clips[0].kind == TrackKind::midi);
            expectWithinAbsoluteError (clips[0].startSeconds, 0.5, 1.0e-4);
            expect (session.getSlots (id)[0].hasClip);
        }
    }
};

static SessionTests sessionTests;

} // namespace resamper::test
