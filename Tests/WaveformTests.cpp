#include "TestFixture.h"

#include <tracktion_engine/tracktion_engine.h>

namespace resamper::test
{

namespace
{
    /** How many pixels the waveform inks for a clip time range: a tone fills a
        band, silence a hairline. */
    int inkedPixels (const ClipWaveform& waveform, double clipStart, double clipEnd, double sourceOffset)
    {
        juce::Image image (juce::Image::ARGB, 200, 40, true);
        {
            juce::Graphics g (image);
            g.setColour (juce::Colours::white);
            waveform.draw (g, image.getBounds(), clipStart, clipEnd, sourceOffset);
        }

        int inked = 0;

        for (int y = 0; y < image.getHeight(); ++y)
            for (int x = 0; x < image.getWidth(); ++x)
                if (image.getPixelAt (x, y).getAlpha() > 0)
                    ++inked;

        return inked;
    }

    constexpr int toneInk = 1500, silenceInk = 1000;

    /** The first track's first clip, which each test adds from an audio file. */
    tracktion::WaveAudioClip* firstClip (Fixture& f)
    {
        auto* track = tracktion::getAudioTracks (f.projects.getEdit())[0];
        return track != nullptr ? dynamic_cast<tracktion::WaveAudioClip*> (track->getClips()[0]) : nullptr;
    }
}

/** A clip's waveform, as the Arrangement draws it (#87). */
struct WaveformTests : juce::UnitTest
{
    WaveformTests() : juce::UnitTest ("Clip Waveform", "Resamper") {}

    void runTest() override
    {
        beginTest ("An unwarped clip's waveform reads its file, with no proxy, and is not ready before it starts");
        {
            Fixture f;
            f.invoke (cmd::trackAdd);
            f.audioFileToChoose = writeSineWav (f.scratchDir().getChildFile ("tone.wav"), 10.0);
            f.invoke (cmd::clipAdd);

            const auto clip = f.model.getTracks()[0].clips[0];
            auto* waveClip = firstClip (f);
            expect (waveClip != nullptr && ! waveClip->getAutoTempo());
            expect (clip.playbackFile == f.audioFileToChoose);

            juce::Component repaintTarget;
            auto waveform = f.model.createWaveform (clip.id, repaintTarget);
            expect (waveform->isGenerating(), "reported ready before reading anything");

            expect (dispatchUntil ([&] { return ! waveform->isGenerating(); }));
            expect (waveform->hasDrawableAudio());
            expectEquals (waveform->getProgress(), 1.0);
            expectGreaterThan (inkedPixels (*waveform, 0.0, clip.lengthSeconds, clip.sourceOffsetSeconds), toneInk);
        }

        beginTest ("A warped clip plays its own file, with no proxy, and draws it stretched over the clip (#111)");
        {
            // A 30 s loop at 100 BPM, in a 120 BPM Edit: it plays in 25 s.
            Fixture f;
            f.invoke (cmd::trackAdd);
            f.audioFileToChoose = writeSineWav (f.scratchDir().getChildFile ("loop.wav"), 30.0, 2, 100.0);
            f.invoke (cmd::clipAdd);

            auto* waveClip = firstClip (f);
            expect (waveClip != nullptr && waveClip->getAutoTempo() && ! waveClip->canUseProxy());

            const auto clip = f.model.getTracks()[0].clips[0];
            expect (clip.playbackFile == f.audioFileToChoose);
            expectWithinAbsoluteError (clip.lengthSeconds, 25.0, 0.01);

            juce::Component repaintTarget;
            auto waveform = f.model.createWaveform (clip.id, repaintTarget);
            expect (dispatchUntil ([&] { return ! waveform->isGenerating(); }));
            expectGreaterThan (inkedPixels (*waveform, 0.0, clip.lengthSeconds, clip.sourceOffsetSeconds), toneInk);
            expectGreaterThan (inkedPixels (*waveform, 24.5, 25.0, clip.sourceOffsetSeconds), toneInk, "the clip's end");
        }

        beginTest ("A warped clip's waveform follows a trim and a tempo change, without a new waveform (#111)");
        {
            // The loop's first 1 s is a tone: 1.667 beats at its 100 BPM. Trimmed by
            // 0.5 s (1 beat) at 120 BPM, the clip opens with 0.667 beats of tone:
            // 0.333 s at 120 BPM, 0.667 s at 60 BPM.
            Fixture f;
            f.invoke (cmd::trackAdd);
            f.audioFileToChoose = writeSineWav (f.scratchDir().getChildFile ("loop.wav"), 30.0, 2, 100.0, 1.0);
            f.invoke (cmd::clipAdd);

            const auto added = f.model.getTracks()[0].clips[0];
            f.invoke (cmd::clipResize, { added.id, added.startSeconds + 0.5, added.startSeconds + added.lengthSeconds });

            const auto clip = f.model.getTracks()[0].clips[0];
            expect (clip.playbackFile == f.audioFileToChoose);

            juce::Component repaintTarget;
            auto waveform = f.model.createWaveform (clip.id, repaintTarget);
            expect (dispatchUntil ([&] { return ! waveform->isGenerating(); }));

            expectGreaterThan (inkedPixels (*waveform, 0.0, 0.2, clip.sourceOffsetSeconds), toneInk, "tone at the clip's start");
            expectLessThan (inkedPixels (*waveform, 0.45, 0.6, clip.sourceOffsetSeconds), silenceInk, "silence at 120 BPM");

            expect (f.invoke (cmd::transportSetTempo, { 60.0 }));
            const auto slower = f.model.getTracks()[0].clips[0];
            expect (slower.playbackFile == clip.playbackFile);
            expectGreaterThan (inkedPixels (*waveform, 0.45, 0.6, slower.sourceOffsetSeconds), toneInk, "tone at 60 BPM");
        }

        beginTest ("A clip whose file has gone is not left generating");
        {
            Fixture f;
            f.invoke (cmd::trackAdd);
            f.audioFileToChoose = writeSineWav (f.scratchDir().getChildFile ("gone.wav"), 1.0);
            f.invoke (cmd::clipAdd);
            expect (f.audioFileToChoose.deleteFile());

            juce::Component repaintTarget;
            auto waveform = f.model.createWaveform (f.model.getTracks()[0].clips[0].id, repaintTarget);

            expect (dispatchUntil ([&] { return ! waveform->isGenerating(); }));
            expect (! waveform->hasDrawableAudio());
        }
    }
};

static WaveformTests waveformTests;

} // namespace resamper::test
