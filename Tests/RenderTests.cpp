#include "TestFixture.h"

#include <tracktion_engine/tracktion_engine.h>

namespace papercut::test
{

/** End-to-end audio proof without a device: Commands build the Edit, the
    engine renders it offline, and the result must be audible. */
struct RenderTests : juce::UnitTest
{
    RenderTests() : juce::UnitTest ("Offline Render", "Papercut") {}

    /** Renders the whole Edit offline and returns the peak level (0 if nothing rendered). */
    float renderPeak (Fixture& f)
    {
        auto rendered = f.scratchDir().getChildFile ("render.wav");
        rendered.deleteFile();
        expect (tracktion::Renderer::renderToFile (f.projects.getEdit(), rendered, false));

        juce::AudioFormatManager formats;
        formats.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (rendered));

        if (reader == nullptr || reader->lengthInSamples == 0)
            return 0.0f;

        juce::AudioBuffer<float> buffer ((int) reader->numChannels, (int) reader->lengthInSamples);
        reader->read (&buffer, 0, buffer.getNumSamples(), 0, true, true);
        return buffer.getMagnitude (0, buffer.getNumSamples());
    }

    void runTest() override
    {
        beginTest ("An Edit with a WAV clip on an audio track renders non-silent audio");
        {
            Fixture f;
            f.invoke ("track.add");
            f.audioFileToChoose = writeSineWav (f.scratchDir().getChildFile ("tone.wav"), 1.0);
            f.invoke ("clip.add");
            expectGreaterThan (renderPeak (f), 0.1f);
        }

        beginTest ("A tempo-tagged (ACID) loop gets its time-stretched audio and renders non-silent");
        {
            // Such loops play from a time-stretched proxy. Without a time-stretcher
            // compiled in, or if nothing starts rendering it, the clip is silent and
            // has no waveform (and an offline render waits for it forever).
            Fixture f;
            f.invoke ("track.add");
            f.audioFileToChoose = writeSineWav (f.scratchDir().getChildFile ("loop.wav"), 2.0, 2, 172.0);
            f.invoke ("clip.add");
            expect (f.errors.isEmpty(), f.errors.joinIntoString ("; "));

            auto* clip = dynamic_cast<tracktion::WaveAudioClip*> (tracktion::getAudioTracks (f.projects.getEdit())[0]->getClips()[0]);
            expect (clip != nullptr && clip->usesTimeStretchedProxy());

            juce::Component repaintTarget;
            auto waveform = f.model.createWaveform (f.model.getTracks()[0].clips[0].id, repaintTarget);

            for (int i = 0; i < 300 && (waveform->isGenerating() || ! clip->getPlaybackFile().getFile().existsAsFile()); ++i)
                juce::MessageManager::getInstance()->runDispatchLoopUntil (100);

            const auto proxy = clip->getPlaybackFile().getFile();
            expect (proxy.getSize() > 44, "time-stretched audio was never produced: " + proxy.getFullPathName());

            if (proxy.getSize() > 44)   // an empty proxy would make the render below never finish
                expectGreaterThan (renderPeak (f), 0.1f);
        }
    }
};

static RenderTests renderTests;

} // namespace papercut::test
