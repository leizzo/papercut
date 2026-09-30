#pragma once

#include "ClipWaveform.h"

#include <tracktion_engine/tracktion_engine.h>

namespace resamper
{

/** Either a clip's file thumbnails or a recording's growing one. */
struct ClipWaveform::Impl
{
    /** A stretch of the clip, in seconds from its start, and the source audio it plays. */
    struct Segment
    {
        double start = 0, length = 0, sourceStart = 0, sourceLength = 0;
    };

    /** The clip's playback file and, if that is a proxy, its source mapped
        through the clip's segments. */
    Impl (tracktion::WaveAudioClip&, juce::Component& repaintTarget);

    explicit Impl (tracktion::RecordingThumbnailManager::Thumbnail::Ptr recording)
        : recordingThumbnail (std::move (recording))
    {
    }

    /** A proxy that is yet to be rendered: not there, not failed, and its source is readable. */
    bool isProxyPending() const;

    /** The playback file is there and all its peaks are read. */
    bool isPlaybackComplete() const;

    /** What to draw: the playback file once complete; before that (or if its
        proxy failed) the source, if it has peaks; else whatever the playback
        file has so far. */
    tracktion::SmartThumbnail* thumbnailToDraw() const;

    /** The file the clip plays: its source, or a proxy rendered from it. */
    std::unique_ptr<tracktion::SmartThumbnail> playbackThumbnail;

    /** A time-stretched proxy is rendered from the clip's start, so its time is
        the clip's; any other playback file is in source time. */
    bool playbackInClipTime = false;

    /** For a clip that plays a proxy: the file the proxy is rendered from, drawn
        through segments until the proxy is ready. */
    std::unique_ptr<tracktion::SmartThumbnail> sourceThumbnail;
    std::vector<Segment> segments;

    /** Set once the proxy was seen rendering, so a render that ends without a
        file counts as failed rather than still to come. */
    mutable bool proxyRenderStarted = false;

    tracktion::RecordingThumbnailManager::Thumbnail::Ptr recordingThumbnail;
};

} // namespace resamper
