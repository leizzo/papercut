#include "TestFixture.h"

#include <tracktion_engine/tracktion_engine.h>

namespace resamper::test
{

/** What an Audio Clip plays when the user adds a file (#111): WAV and AIFF in
    place, a compressed file decoded into the Project. */
struct AudioImportTests : juce::UnitTest
{
    AudioImportTests() : juce::UnitTest ("Audio Import", "Resamper") {}

    void runTest() override
    {
        beginTest ("A FLAC becomes a 32-bit float WAV in the Project's Audio folder, at its own sample rate");
        {
            Fixture f;
            f.invoke (cmd::trackAdd);
            f.audioFileToChoose = writeSineFlac (f.scratchDir().getChildFile ("library/loop.flac"), 1.5);
            f.invoke (cmd::clipAdd);
            expect (f.errors.isEmpty(), f.errors.joinIntoString ("; "));

            const auto clip = f.model.getTracks()[0].clips[0];
            const auto audioFolder = ProjectManager::getAudioFolder (f.projects.getProjectFolder());
            expect (clip.playbackFile.getParentDirectory() == audioFolder, clip.playbackFile.getFullPathName());
            expect (clip.playbackFile.hasFileExtension ("wav"));
            expectWithinAbsoluteError (clip.lengthSeconds, 1.5, 1e-3);

            juce::WavAudioFormat wav;
            std::unique_ptr<juce::AudioFormatReader> reader (wav.createReaderFor (clip.playbackFile.createInputStream().release(), true));
            expect (reader != nullptr);

            if (reader != nullptr)
            {
                expectEquals ((int) reader->bitsPerSample, 32);
                expect (reader->usesFloatingPointData);
                expectEquals (reader->sampleRate, 44100.0);
                expectEquals ((int) reader->numChannels, 2);
                expectEquals ((int) reader->lengthInSamples, (int) (1.5 * 44100.0));
            }
        }

        beginTest ("Adding the same FLAC twice decodes it twice, to two files");
        {
            Fixture f;
            f.invoke (cmd::trackAdd);
            f.audioFileToChoose = writeSineFlac (f.scratchDir().getChildFile ("loop.flac"), 0.5);
            f.invoke (cmd::clipAdd);
            f.invoke (cmd::clipAdd);
            expect (f.errors.isEmpty(), f.errors.joinIntoString ("; "));

            const auto clips = f.model.getTracks()[0].clips;
            expectEquals ((int) clips.size(), 2);

            if (clips.size() == 2)
            {
                expect (clips[0].playbackFile != clips[1].playbackFile);
                expect (clips[0].playbackFile.existsAsFile() && clips[1].playbackFile.existsAsFile());
            }
        }

        beginTest ("A WAV is played where it is");
        {
            Fixture f;
            f.invoke (cmd::trackAdd);
            f.audioFileToChoose = writeSineWav (f.scratchDir().getChildFile ("library/tone.wav"), 0.5);
            f.invoke (cmd::clipAdd);

            expect (f.model.getTracks()[0].clips[0].playbackFile == f.audioFileToChoose);
            expect (! ProjectManager::getAudioFolder (f.projects.getProjectFolder()).getChildFile ("tone.wav").exists());
        }

        beginTest ("A slot clip from a FLAC plays a WAV in the Project's Audio folder");
        {
            Fixture f;
            f.invoke (cmd::trackAdd);
            expect (f.session.setSceneCount (1).wasOk());

            const auto id = f.model.getTracks()[0].id;
            expect (f.session.addSlotClip (id, 0, writeSineFlac (f.scratchDir().getChildFile ("loop.flac"), 0.5)).wasOk());

            auto* slot = tracktion::getAudioTracks (f.projects.getEdit())[0]->getClipSlotList().getClipSlots()[0];
            auto* clip = dynamic_cast<tracktion::WaveAudioClip*> (slot->getClip());
            expect (clip != nullptr);

            if (clip != nullptr)
                expect (clip->getPlaybackFile().getFile().getParentDirectory()
                            == ProjectManager::getAudioFolder (f.projects.getProjectFolder()));
        }

        beginTest ("A decoded FLAC goes with an untitled Project through Save As and Open");
        {
            Fixture f;
            f.invoke (cmd::trackAdd);
            f.audioFileToChoose = writeSineFlac (f.scratchDir().getChildFile ("loop.flac"), 0.5);
            f.invoke (cmd::clipAdd);

            f.projectSaveLocation = f.scratchDir().getChildFile ("Saved");
            f.invoke (cmd::projectSaveAs);
            expect (f.errors.isEmpty(), f.errors.joinIntoString ("; "));

            Fixture reopened;
            reopened.projectToOpen = f.projectSaveLocation;
            reopened.invoke (cmd::projectOpen);
            expect (reopened.errors.isEmpty(), reopened.errors.joinIntoString ("; "));

            const auto playback = reopened.model.getTracks()[0].clips[0].playbackFile;
            expect (playback.isAChildOf (f.projectSaveLocation), playback.getFullPathName());
            expect (playback.existsAsFile());
        }
    }
};

static AudioImportTests audioImportTests;

} // namespace resamper::test
