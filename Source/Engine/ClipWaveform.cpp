#include "ClipWaveform.h"
#include "ClipWaveformImpl.h"

namespace te = tracktion;

namespace resamper
{

namespace
{
    // No Edit is passed: it is only used to choose a cache folder, and a
    // waveform must never outlive-reference an Edit that a Project Open replaced.
    std::unique_ptr<te::SmartThumbnail> makeThumbnail (te::Engine& engine, const te::AudioFile& file,
                                                       juce::Component& repaintTarget)
    {
        return std::make_unique<te::SmartThumbnail> (engine, file, repaintTarget, nullptr);
    }

    /** The file is there and has audio a reader can open. The engine caches a
        file's info, so a file deleted since still reads as valid. */
    bool isReadable (const te::AudioFile& file)
    {
        return file.getFile().existsAsFile() && file.isValid();
    }

    /** Still reading peak data. A thumbnail reports itself fully loaded before
        its reader is attached (it has no samples yet), so no channels means
        not started. A missing or unreadable file never loads. */
    bool isLoading (const te::SmartThumbnail& thumb)
    {
        return isReadable (thumb.file) && (thumb.getNumChannels() == 0 || ! thumb.isFullyLoaded());
    }

    /** Some peaks are read: there is a waveform to see, if only its start. */
    bool hasPeaks (const te::SmartThumbnail& thumb)
    {
        return thumb.getNumChannels() > 0 && thumb.getNumSamplesFinished() > 0;
    }

    /** Draws [startSeconds, endSeconds] of the thumbnail's file, one lane per channel. */
    void drawChannels (juce::AudioThumbnailBase& thumb, juce::Graphics& g, juce::Rectangle<int> area,
                       double startSeconds, double endSeconds)
    {
        const auto numChannels = thumb.getNumChannels();

        if (numChannels == 0 || area.isEmpty())
            return;

        const auto laneHeight = area.getHeight() / numChannels;

        for (int channel = 0; channel < numChannels; ++channel)
            thumb.drawChannel (g, area.removeFromTop (laneHeight), startSeconds, endSeconds, channel, 1.0f);
    }
}

ClipWaveform::Impl::Impl (te::WaveAudioClip& clip, juce::Component& repaintTarget)
{
    auto& engine = clip.edit.engine;
    const auto playbackFile = clip.getPlaybackFile();
    const auto sourceFile = clip.getAudioFile();
    playbackThumbnail = makeThumbnail (engine, playbackFile, repaintTarget);

    // A proxy can take seconds to render: draw its source meanwhile, where the clip plays it.
    if (playbackFile == sourceFile)
        return;

    playbackInClipTime = clip.usesTimeStretchedProxy();
    sourceThumbnail = makeThumbnail (engine, sourceFile, repaintTarget);

    const auto sampleRate = sourceFile.getSampleRate();
    const auto list = te::AudioSegmentList::create (clip, true, false);

    if (sampleRate > 0)
        for (auto& segment : list->getSegments())
            segments.push_back ({ segment.start.inSeconds(), segment.length.inSeconds(),
                                  (double) segment.startSample / sampleRate,
                                  (double) segment.lengthSample / sampleRate });
}

bool ClipWaveform::Impl::isProxyPending() const
{
    if (sourceThumbnail == nullptr || playbackThumbnail->file.getFile().existsAsFile())
        return false;

    if (playbackThumbnail->isGeneratingProxy())
        proxyRenderStarted = true;
    else if (proxyRenderStarted)
        return false;

    return isReadable (sourceThumbnail->file);
}

bool ClipWaveform::Impl::isPlaybackComplete() const
{
    return ! playbackThumbnail->isGeneratingProxy() && ! isProxyPending() && ! isLoading (*playbackThumbnail);
}

te::SmartThumbnail* ClipWaveform::Impl::thumbnailToDraw() const
{
    if (playbackThumbnail == nullptr)
        return nullptr;

    const auto playbackReady = isPlaybackComplete() && hasPeaks (*playbackThumbnail);

    if (sourceThumbnail != nullptr && ! playbackReady && hasPeaks (*sourceThumbnail))
        return sourceThumbnail.get();

    return hasPeaks (*playbackThumbnail) ? playbackThumbnail.get() : nullptr;
}

ClipWaveform::ClipWaveform (std::unique_ptr<Impl> i) : impl (std::move (i)) {}
ClipWaveform::~ClipWaveform() = default;

bool ClipWaveform::isGenerating() const
{
    // A recording's thumbnail is fed directly by the audio it records.
    if (impl->playbackThumbnail == nullptr)
        return false;

    return ! impl->isPlaybackComplete();
}

bool ClipWaveform::hasDrawableAudio() const
{
    return impl->playbackThumbnail == nullptr || impl->thumbnailToDraw() != nullptr;
}

double ClipWaveform::getProgress() const
{
    if (impl->playbackThumbnail == nullptr)
        return 1.0;

    auto& thumb = *impl->playbackThumbnail;

    if (thumb.isGeneratingProxy())
        return thumb.getProxyProgress();

    if (impl->isProxyPending())
        return 0.0;

    return isLoading (thumb) ? thumb.getProportionComplete() : 1.0;
}

void ClipWaveform::draw (juce::Graphics& g, juce::Rectangle<int> area,
                         double clipStartSeconds, double clipEndSeconds, double sourceOffsetSeconds) const
{
    if (area.isEmpty() || clipEndSeconds <= clipStartSeconds)
        return;

    if (impl->playbackThumbnail == nullptr)
    {
        drawChannels (*impl->recordingThumbnail->thumb, g, area, clipStartSeconds, clipEndSeconds);
        return;
    }

    auto* thumb = impl->thumbnailToDraw();

    if (thumb == nullptr)
        return;

    if (thumb == impl->playbackThumbnail.get())
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
