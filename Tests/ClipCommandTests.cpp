#include "TestFixture.h"

namespace resamper::test
{

struct ClipCommandTests : juce::UnitTest
{
    ClipCommandTests() : juce::UnitTest ("Clip Commands", "Resamper") {}

    void runTest() override
    {
        beginTest ("clip.add inserts a clip referencing the chosen file, spanning its length");
        {
            Fixture f;
            f.invoke (cmd::trackAdd);
            f.audioFileToChoose = writeSineWav (f.scratchDir().getChildFile ("tone.wav"), 1.5);

            expect (f.invoke (cmd::clipAdd));

            auto tracks = f.model.getTracks();
            expectEquals ((int) tracks[0].clips.size(), 1);

            auto clip = tracks[0].clips[0];
            expect (clip.file == f.audioFileToChoose);
            expectWithinAbsoluteError (clip.startSeconds, 0.0, 1e-6);
            expectWithinAbsoluteError (clip.lengthSeconds, 1.5, 1e-3);
            expect (f.errors.isEmpty());
        }

        beginTest ("edit.undo removes the inserted clip and keeps the track");
        {
            Fixture f;
            f.invoke (cmd::trackAdd);
            f.audioFileToChoose = writeSineWav (f.scratchDir().getChildFile ("tone.wav"), 1.0);
            f.invoke (cmd::clipAdd);

            f.invoke (cmd::editUndo);

            expectEquals (f.numTracks(), 1);
            expect (f.model.getTracks()[0].clips.empty());

            f.invoke (cmd::editRedo);
            expectEquals ((int) f.model.getTracks()[0].clips.size(), 1);
        }

        beginTest ("A second clip.add appends after the first clip");
        {
            Fixture f;
            f.invoke (cmd::trackAdd);
            f.audioFileToChoose = writeSineWav (f.scratchDir().getChildFile ("tone.wav"), 1.0);
            f.invoke (cmd::clipAdd);
            f.invoke (cmd::clipAdd);

            auto clips = f.model.getTracks()[0].clips;
            expectEquals ((int) clips.size(), 2);
            expectWithinAbsoluteError (clips[1].startSeconds, clips[0].startSeconds + clips[0].lengthSeconds, 1e-3);
        }

        beginTest ("clip.add targets the selected track");
        {
            Fixture f;
            f.invoke (cmd::trackAdd);
            f.invoke (cmd::trackAdd);
            f.model.selectTrack (f.model.getTracks()[1].id);

            f.audioFileToChoose = writeSineWav (f.scratchDir().getChildFile ("tone.wav"), 1.0);
            f.invoke (cmd::clipAdd);

            auto tracks = f.model.getTracks();
            expect (tracks[0].clips.empty());
            expectEquals ((int) tracks[1].clips.size(), 1);
        }

        beginTest ("clip.add on an empty Edit creates a track; one undo removes both");
        {
            Fixture f;
            f.audioFileToChoose = writeSineWav (f.scratchDir().getChildFile ("tone.wav"), 1.0);
            f.invoke (cmd::clipAdd);

            expectEquals (f.numTracks(), 1);
            expectEquals ((int) f.model.getTracks()[0].clips.size(), 1);

            f.invoke (cmd::editUndo);
            expectEquals (f.numTracks(), 0);
            expect (! f.model.canUndo());
        }

        beginTest ("clip.add with a file that is not audio reports an error and changes nothing");
        {
            Fixture f;
            f.invoke (cmd::trackAdd);
            f.audioFileToChoose = f.scratchDir().getChildFile ("notes.txt");
            f.audioFileToChoose.replaceWithText ("not audio");

            f.invoke (cmd::clipAdd);

            expectEquals (f.errors.size(), 1);
            expect (f.model.getTracks()[0].clips.empty());
            f.invoke (cmd::editUndo);
            expectEquals (f.numTracks(), 0);
        }

        beginTest ("Cancelling the file chooser changes nothing");
        {
            Fixture f;
            f.invoke (cmd::clipAdd);
            expectEquals (f.numTracks(), 0);
            expect (! f.model.canUndo());
        }
    }
};

static ClipCommandTests clipCommandTests;

} // namespace resamper::test
