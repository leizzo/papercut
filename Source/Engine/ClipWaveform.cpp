#include "ClipWaveform.h"
#include "ClipWaveformImpl.h"

namespace resamper
{

namespace
{
    /** Still reading peak data. A thumbnail reports itself fully loaded before
        its reader is attached (it has no samples yet), so no channels means
        not started. A missing file never loads. */
    bool isLoading (const tracktion::SmartThumbnail& thumb)
    {
        return thumb.file.getFile().existsAsFile() && (thumb.getNumChannels() == 0 || ! thumb.isFullyLoaded());
    }

    /** Draws [startSeconds, endSeconds] of the thumbnail's file, one lane per channel. */
    void drawChannels (tracktion::SmartThumbnail& thumb, juce::Graphics& g, juce::Rectangle<int> area,
                       double startSeconds, double endSeconds)
    {
        const auto numChannels = thumb.getNumChannels();

        if (numChannels == 0 || area.isEmpty())
            return;

        const tracktion::TimeRange range (tracktion::TimePosition::fromSeconds (startSeconds),
                                          tracktion::TimePosition::fromSeconds (endSeconds));
        const auto laneHeight = area.getHeight() / numChannels;

        for (int channel = 0; channel < numChannels; ++channel)
            thumb.drawChannel (g, area.removeFromTop (laneHeight), range, channel, 1.0f);
    }

    /** A proxy that hasn't been rendered yet, and will be: its source exists. */
    bool isProxyPending (const ClipWaveform::Impl& impl)
    {
        return impl.source != nullptr
            && ! impl.playback->file.getFile().existsAsFile()
            && impl.source->file.getFile().existsAsFile();
    }

    bool isPlaybackComplete (const ClipWaveform::Impl& impl)
    {
        return ! impl.playback->isGeneratingProxy() && ! isProxyPending (impl) && ! isLoading (*impl.playback);
    }

    /** What to draw: the playback file once complete, before that the source if
        it has anything, else whatever the playback file has so far. */
    tracktion::SmartThumbnail* thumbnailToDraw (const ClipWaveform::Impl& impl)
    {
        if (impl.playback == nullptr)
            return nullptr;

        if (impl.source != nullptr && ! isPlaybackComplete (impl) && impl.source->getNumChannels() > 0)
            return impl.source.get();

        return impl.playback->getNumChannels() > 0 ? impl.playback.get() : nullptr;
    }
}

ClipWaveform::ClipWaveform (std::unique_ptr<Impl> i) : impl (std::move (i)) {}
ClipWaveform::~ClipWaveform() = default;

bool ClipWaveform::isGenerating() const
{
    // A recording's thumbnail is fed directly by the audio it records.
    if (impl->playback == nullptr)
        return false;

    return ! isPlaybackComplete (*impl);
}

bool ClipWaveform::hasDrawableAudio() const
{
    return impl->playback == nullptr || thumbnailToDraw (*impl) != nullptr;
}

double ClipWaveform::getProgress() const
{
    if (impl->playback == nullptr)
        return 1.0;

    auto& thumb = *impl->playback;

    if (thumb.isGeneratingProxy())
        return thumb.getProxyProgress();

    if (isProxyPending (*impl))
        return 0.0;

    return isLoading (thumb) ? thumb.getProportionComplete() : 1.0;
}

void ClipWaveform::draw (juce::Graphics& g, juce::Rectangle<int> area,
                         double clipStartSeconds, double clipEndSeconds, double sourceOffsetSeconds) const
{
    if (area.isEmpty() || clipEndSeconds <= clipStartSeconds)
        return;

    if (impl->playback == nullptr)
    {
        auto& thumb = *impl->recordingThumbnail->thumb;
        const auto numChannels = thumb.getNumChannels();

        if (numChannels == 0)
            return;

        const auto laneHeight = area.getHeight() / numChannels;

        for (int channel = 0; channel < numChannels; ++channel)
            thumb.drawChannel (g, area.removeFromTop (laneHeight), clipStartSeconds, clipEndSeconds, channel, 1.0f);

        return;
    }

    auto* thumb = thumbnailToDraw (*impl);

    if (thumb == nullptr)
        return;

    if (thumb == impl->playback.get())
    {
        const auto offset = impl->playbackInClipTime ? 0.0 : sourceOffsetSeconds;
        drawChannels (*thumb, g, area, clipStartSeconds + offset, clipEndSeconds + offset);
        return;
    }

    // The source, each segment where the clip plays it.
    const auto pixelsPerSecond = area.getWidth() / (clipEndSeconds - clipStartSeconds);

    for (auto& segment : impl->segments)
    {
        const auto start = std::max (clipStartSeconds, segment.start);
        const auto end = std::min (clipEndSeconds, segment.start + segment.length);

        if (end <= start || segment.length <= 0)
            continue;

        const auto x1 = area.getX() + juce::roundToInt ((start - clipStartSeconds) * pixelsPerSecond);
        const auto x2 = area.getX() + juce::roundToInt ((end - clipStartSeconds) * pixelsPerSecond);
        const auto toSource = [&segment] (double t)
        {
            return segment.sourceStart + (t - segment.start) / segment.length * segment.sourceLength;
        };

        drawChannels (*thumb, g, area.withLeft (x1).withRight (x2), toSource (start), toSource (end));
    }
}

} // namespace resamper
