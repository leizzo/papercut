#include "ClipWaveform.h"
#include "ClipWaveformImpl.h"

namespace papercut
{

ClipWaveform::ClipWaveform (std::unique_ptr<Impl> i) : impl (std::move (i)) {}
ClipWaveform::~ClipWaveform() = default;

bool ClipWaveform::isGenerating() const
{
    // A time-stretched clip's audio doesn't exist until its proxy is rendered.
    return impl->thumbnail.isGeneratingProxy() || ! impl->thumbnail.isFullyLoaded();
}

double ClipWaveform::getProgress() const
{
    return impl->thumbnail.isGeneratingProxy() ? impl->thumbnail.getProxyProgress()
                                               : impl->thumbnail.getProportionComplete();
}

void ClipWaveform::draw (juce::Graphics& g, juce::Rectangle<int> area,
                         double sourceStartSeconds, double sourceEndSeconds) const
{
    auto& thumb = impl->thumbnail;
    const auto numChannels = thumb.getNumChannels();

    if (numChannels == 0 || area.isEmpty())
        return;

    const tracktion::TimeRange range (tracktion::TimePosition::fromSeconds (sourceStartSeconds),
                                      tracktion::TimePosition::fromSeconds (sourceEndSeconds));
    const auto laneHeight = area.getHeight() / numChannels;

    for (int channel = 0; channel < numChannels; ++channel)
        thumb.drawChannel (g, area.removeFromTop (laneHeight), range, channel, 1.0f);
}

} // namespace papercut
