#include "ClipWaveform.h"
#include "ClipWaveformImpl.h"

namespace resamper
{

ClipWaveform::ClipWaveform (std::unique_ptr<Impl> i) : impl (std::move (i)) {}
ClipWaveform::~ClipWaveform() = default;

bool ClipWaveform::isGenerating() const
{
    // A recording's thumbnail is fed directly by the audio it records.
    if (impl->fileThumbnail == nullptr)
        return false;

    // A time-stretched clip's audio doesn't exist until its proxy is rendered.
    auto& thumb = *impl->fileThumbnail;
    return thumb.isGeneratingProxy() || ! thumb.isFullyLoaded();
}

double ClipWaveform::getProgress() const
{
    if (impl->fileThumbnail == nullptr)
        return 1.0;

    auto& thumb = *impl->fileThumbnail;
    return thumb.isGeneratingProxy() ? thumb.getProxyProgress() : thumb.getProportionComplete();
}

void ClipWaveform::draw (juce::Graphics& g, juce::Rectangle<int> area,
                         double sourceStartSeconds, double sourceEndSeconds) const
{
    const auto numChannels = impl->fileThumbnail != nullptr ? impl->fileThumbnail->getNumChannels()
                                                            : impl->recordingThumbnail->thumb->getNumChannels();

    if (numChannels == 0 || area.isEmpty())
        return;

    const tracktion::TimeRange range (tracktion::TimePosition::fromSeconds (sourceStartSeconds),
                                      tracktion::TimePosition::fromSeconds (sourceEndSeconds));
    const auto laneHeight = area.getHeight() / numChannels;

    for (int channel = 0; channel < numChannels; ++channel)
    {
        const auto lane = area.removeFromTop (laneHeight);

        if (impl->fileThumbnail != nullptr)
            impl->fileThumbnail->drawChannel (g, lane, range, channel, 1.0f);
        else
            impl->recordingThumbnail->thumb->drawChannel (g, lane, sourceStartSeconds, sourceEndSeconds, channel, 1.0f);
    }
}

} // namespace resamper
