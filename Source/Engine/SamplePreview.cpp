#include "SamplePreview.h"
#include "EngineManager.h"

#include <tracktion_engine/tracktion_engine.h>

namespace papercut
{

struct SamplePreview::Impl
{
    explicit Impl (EngineManager& em) : devices (em.getEngine().getDeviceManager().deviceManager)
    {
        formats.registerBasicFormats();
        readAhead.startThread();
        player.setSource (&transport);
        devices.addAudioCallback (&player);
    }

    ~Impl()
    {
        devices.removeAudioCallback (&player);
        player.setSource (nullptr);
        transport.setSource (nullptr);
        readAhead.stopThread (1000);
    }

    juce::AudioDeviceManager& devices;
    juce::AudioFormatManager formats;
    juce::TimeSliceThread readAhead { "Sample Preview" };
    juce::AudioTransportSource transport;
    juce::AudioSourcePlayer player;
    std::unique_ptr<juce::AudioFormatReaderSource> source;
    juce::File file;
};

SamplePreview::SamplePreview (EngineManager& em) : impl (std::make_unique<Impl> (em)) {}
SamplePreview::~SamplePreview() = default;

bool SamplePreview::play (const juce::File& file)
{
    stop();

    auto* reader = impl->formats.createReaderFor (file);

    if (reader == nullptr)
        return false;

    const auto sampleRate = reader->sampleRate;
    auto source = std::make_unique<juce::AudioFormatReaderSource> (reader, true);
    impl->transport.setSource (source.get(), 32768, &impl->readAhead, sampleRate);
    impl->source = std::move (source);
    impl->file = file;
    impl->transport.setPosition (0);
    impl->transport.start();
    return true;
}

void SamplePreview::stop()
{
    impl->transport.stop();
    impl->transport.setSource (nullptr);
    impl->source.reset();
    impl->file = juce::File();
}

bool SamplePreview::isPlaying() const   { return impl->transport.isPlaying(); }
juce::File SamplePreview::getFile() const  { return impl->file; }

} // namespace papercut
