#include "TestFixture.h"

#include <juce_audio_formats/juce_audio_formats.h>

namespace papercut::test
{

juce::File writeSineWav (const juce::File& file, double seconds, int numChannels)
{
    constexpr double sampleRate = 44100.0;
    const auto numSamples = (int) (seconds * sampleRate);

    juce::AudioBuffer<float> buffer (numChannels, numSamples);

    for (int ch = 0; ch < numChannels; ++ch)
        for (int i = 0; i < numSamples; ++i)
            buffer.setSample (ch, i, 0.5f * (float) std::sin (juce::MathConstants<double>::twoPi * 440.0 * i / sampleRate));

    file.getParentDirectory().createDirectory();
    file.deleteFile();

    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> out (file.createOutputStream().release());
    auto writer = wav.createWriterFor (out,
                                       juce::AudioFormatWriterOptions().withSampleRate (sampleRate)
                                                                       .withNumChannels (numChannels)
                                                                       .withBitsPerSample (16));
    jassert (writer != nullptr);
    writer->writeFromAudioSampleBuffer (buffer, 0, numSamples);
    return file;
}

Fixture::Fixture()
{
    scratchDir().createDirectory();

    auto pick = [] (juce::File& chosen)
    {
        return [&chosen] (AppCommandHost::FileCallback cb)
        {
            if (chosen != juce::File())
                cb (chosen);
        };
    };

    host.chooseAudioFile = pick (audioFileToChoose);
    host.chooseProjectToOpen = pick (projectToOpen);
    host.chooseProjectSaveLocation = pick (projectSaveLocation);
    host.captureUIState = [this] { return uiState; };
    host.restoreUIState = [this] (const juce::var& v) { uiState = v; };
    host.reportError = [this] (const juce::String& e) { errors.add (e); };

    registerAppCommands (commands, model, host);
}

Fixture::~Fixture()
{
    scratchDir().deleteRecursively();
}

} // namespace papercut::test
