#include "TestFixture.h"

#include <tracktion_engine/tracktion_engine.h>

namespace papercut::test
{

/** End-to-end audio proof without a device: Commands build the Edit, the
    engine renders it offline, and the result must be audible. */
struct RenderTests : juce::UnitTest
{
    RenderTests() : juce::UnitTest ("Offline Render", "Papercut") {}

    void runTest() override
    {
        beginTest ("An Edit with a WAV clip on an audio track renders non-silent audio");
        {
            Fixture f;
            f.invoke ("track.add");
            f.audioFileToChoose = writeSineWav (f.scratchDir().getChildFile ("tone.wav"), 1.0);
            f.invoke ("clip.add");

            auto rendered = f.scratchDir().getChildFile ("render.wav");
            expect (tracktion::Renderer::renderToFile (f.projects.getEdit(), rendered, false));

            juce::AudioFormatManager formats;
            formats.registerBasicFormats();
            std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (rendered));
            expect (reader != nullptr);

            if (reader != nullptr)
            {
                juce::AudioBuffer<float> buffer ((int) reader->numChannels, (int) reader->lengthInSamples);
                reader->read (&buffer, 0, buffer.getNumSamples(), 0, true, true);

                expectGreaterThan (buffer.getNumSamples(), (int) (0.9 * reader->sampleRate));
                expectGreaterThan (buffer.getMagnitude (0, buffer.getNumSamples()), 0.1f);
            }
        }
    }
};

static RenderTests renderTests;

} // namespace papercut::test
