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
        auto thumb = std::make_unique<te::SmartThumbnail> (engine, file, repaintTarget, nullptr);

        // Left to its timer, a thumbnail reads nothing until the first tick, so a
        // waveform made for a split clip paints empty once even when the cache
        // holds its file's peaks (#89).
        thumb->audioFileChanged();
        return thumb;
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

    /** The clip plays its file at other than its recorded speed, so a stretch of
        the clip is not the same stretch of its file. */
    bool isWarped (te::WaveAudioClip& clip)
    {
        return clip.getAutoTempo() || std::abs (clip.getSpeedRatio() - 1.0) > 1.0e-5;
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

ClipWaveform::Impl::Impl (te::WaveAudioClip& c, juce::Component& repaintTarget)
    : thumbnail (makeThumbnail (c.edit.engine, c.getPlaybackFile(), repaintTarget)), clip (&c)
{
}

ClipWaveform::ClipWaveform (std::unique_ptr<Impl> i) : impl (std::move (i)) {}
ClipWaveform::~ClipWaveform() = default;

bool ClipWaveform::isGenerating() const
{
    // A recording's thumbnail is fed directly by the audio it records.
    return impl->thumbnail != nullptr && isLoading (*impl->thumbnail);
}

bool ClipWaveform::hasDrawableAudio() const
{
    return impl->thumbnail == nullptr || hasPeaks (*impl->thumbnail);
}

double ClipWaveform::getProgress() const
{
    if (impl->thumbnail == nullptr || ! isLoading (*impl->thumbnail))
        return 1.0;

    return impl->thumbnail->getProportionComplete();
}

void ClipWaveform::draw (juce::Graphics& g, juce::Rectangle<int> area,
                         double clipStartSeconds, double clipEndSeconds, double sourceOffsetSeconds) const
{
    if (area.isEmpty() || clipEndSeconds <= clipStartSeconds)
        return;

    if (impl->thumbnail == nullptr)
    {
        drawChannels (*impl->recordingThumbnail->thumb, g, area, clipStartSeconds, clipEndSeconds);
        return;
    }

    auto& thumb = *impl->thumbnail;

    if (! hasPeaks (thumb))
        return;

    auto* clip = impl->clip.get();

    if (clip == nullptr || ! isWarped (*clip))
    {
        drawChannels (thumb, g, area, clipStartSeconds + sourceOffsetSeconds, clipEndSeconds + sourceOffsetSeconds);
        return;
    }

    // Each segment of the file where the clip plays it. The engine keeps the
    // list until the clip or the tempo changes.
    const auto sampleRate = clip->getAudioFile().getSampleRate();

    if (sampleRate <= 0)
        return;

    const auto clipStart = clip->getPosition().getStart();
    const auto pixelsPerSecond = area.getWidth() / (clipEndSeconds - clipStartSeconds);

    for (auto& segment : clip->getAudioSegmentList().getSegments())
    {
        const auto segmentStart = (segment.start - clipStart).inSeconds();
        const auto segmentLength = segment.length.inSeconds();
        const auto start = std::max (clipStartSeconds, segmentStart);
        const auto end = std::min (clipEndSeconds, segmentStart + segmentLength);

        if (end <= start || segmentLength <= 0)
            continue;

        const auto x1 = area.getX() + juce::roundToInt ((start - clipStartSeconds) * pixelsPerSecond);
        const auto x2 = area.getX() + juce::roundToInt ((end - clipStartSeconds) * pixelsPerSecond);
        const auto toSource = [&] (double t)
        {
            return ((double) segment.startSample + (t - segmentStart) / segmentLength * (double) segment.lengthSample) / sampleRate;
        };

        drawChannels (thumb, g, area.withLeft (x1).withRight (x2), toSource (start), toSource (end));
    }
}

} // namespace resamper
