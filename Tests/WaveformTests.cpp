#include "TestFixture.h"

#include <tracktion_engine/tracktion_engine.h>

namespace resamper::test
{

namespace
{
    /** Dispatches messages until done() or about a minute has passed; returns done(). */
    template <typename Predicate>
    bool dispatchUntil (Predicate done)
    {
        for (int i = 0; i < 6000 && ! done(); ++i)
            juce::MessageManager::getInstance()->runDispatchLoopUntil (10);

        return done();
    }

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
    bool usesTimeStretchedProxy (Fixture& f)
    {
        auto* clip = dynamic_cast<tracktion::WaveAudioClip*> (tracktion::getAudioTracks (f.projects.getEdit())[0]->getClips()[0]);
        return clip != nullptr && clip->usesTimeStretchedProxy();
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
            expect (! usesTimeStretchedProxy (f));
            expect (clip.playbackFile == f.audioFileToChoose);

            juce::Component repaintTarget;
            auto waveform = f.model.createWaveform (clip.id, repaintTarget);
            expect (waveform->isGenerating(), "reported ready before reading anything");

            expect (dispatchUntil ([&] { return ! waveform->isGenerating(); }));
            expect (waveform->hasDrawableAudio());
            expectEquals (waveform->getProgress(), 1.0);
            expectGreaterThan (inkedPixels (*waveform, 0.0, clip.lengthSeconds, clip.sourceOffsetSeconds), toneInk);
        }

        beginTest ("A time-stretched clip draws its source while its proxy renders");
        {
            Fixture f;
            f.invoke (cmd::trackAdd);
            f.audioFileToChoose = writeSineWav (f.scratchDir().getChildFile ("loop.wav"), 30.0, 2, 100.0);
            f.invoke (cmd::clipAdd);

            expect (usesTimeStretchedProxy (f));

            const auto clip = f.model.getTracks()[0].clips[0];
            const auto proxy = clip.playbackFile;
            expect (proxy != f.audioFileToChoose);

            // Nothing has dispatched since the clip was added, so its proxy job hasn't started.
            auto& engine = f.projects.getEdit().engine;
            expect (! engine.getAudioFileManager().proxyGenerator.isProxyBeingGenerated (tracktion::AudioFile (engine, proxy)));

            juce::Component repaintTarget;
            auto waveform = f.model.createWaveform (clip.id, repaintTarget);
            expect (waveform->isGenerating(), "reported ready before the proxy job started");

            // The source's peaks take a fraction of the proxy's render; they load
            // from the start, so that's where to look.
            expect (dispatchUntil ([&] { return waveform->hasDrawableAudio(); }));
            expect (! proxy.existsAsFile(), "the proxy was ready before the source was drawn");
            expect (waveform->isGenerating());
            expectGreaterThan (inkedPixels (*waveform, 0.0, 0.5, clip.sourceOffsetSeconds), toneInk);

            expect (dispatchUntil ([&] { return ! waveform->isGenerating(); }));
            expect (proxy.existsAsFile());
            expectGreaterThan (inkedPixels (*waveform, 0.0, clip.lengthSeconds, clip.sourceOffsetSeconds), toneInk);
        }

        beginTest ("A time-stretched clip trimmed at its start draws the audio it plays, from its source and its proxy");
        {
            // A tone, then silence: the clip's first moments are tone only if the
            // trim is honoured once, not twice (the proxy starts at the clip's start).
            Fixture f;
            f.invoke (cmd::trackAdd);
            f.audioFileToChoose = writeSineWav (f.scratchDir().getChildFile ("loop.wav"), 30.0, 2, 100.0, 1.0);
            f.invoke (cmd::clipAdd);

            const auto added = f.model.getTracks()[0].clips[0];
            f.invoke (cmd::clipResize, { added.id, added.startSeconds + 0.5, added.startSeconds + added.lengthSeconds });

            const auto clip = f.model.getTracks()[0].clips[0];
            expectGreaterThan (clip.sourceOffsetSeconds, 0.0);
            expect (usesTimeStretchedProxy (f));

            juce::Component repaintTarget;
            auto waveform = f.model.createWaveform (clip.id, repaintTarget);

            auto expectToneThenSilence = [&] (const juce::String& from)
            {
                expectGreaterThan (inkedPixels (*waveform, 0.0, 0.2, clip.sourceOffsetSeconds), toneInk, from + ": tone at the clip's start");
                expectLessThan (inkedPixels (*waveform, 2.0, 2.2, clip.sourceOffsetSeconds), silenceInk, from + ": silence later");
            };

            // As above, the source's first peaks come long before the proxy.
            expect (dispatchUntil ([&] { return waveform->hasDrawableAudio(); }));
            expect (! clip.playbackFile.existsAsFile(), "the proxy was ready before the source was drawn");
            expectToneThenSilence ("source");

            expect (dispatchUntil ([&] { return ! waveform->isGenerating(); }));
            expectToneThenSilence ("proxy");
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
