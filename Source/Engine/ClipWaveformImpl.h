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

    // No Edit is passed: it is only used to choose a cache folder, and a
    // waveform must never outlive-reference an Edit that a Project Open replaced.
    static std::unique_ptr<tracktion::SmartThumbnail> makeThumbnail (tracktion::Engine& engine, const juce::File& file,
                                                                      juce::Component& repaintTarget)
    {
        return std::make_unique<tracktion::SmartThumbnail> (engine, tracktion::AudioFile (engine, file), repaintTarget, nullptr);
    }

    Impl (tracktion::Engine& engine, const juce::File& file, juce::Component& repaintTarget)
        : playback (makeThumbnail (engine, file, repaintTarget))
    {
    }

    explicit Impl (tracktion::RecordingThumbnailManager::Thumbnail::Ptr recording)
        : recordingThumbnail (std::move (recording))
    {
    }

    /** The file the clip plays: its source, or a proxy rendered from it. */
    std::unique_ptr<tracktion::SmartThumbnail> playback;

    /** A time-stretched proxy is rendered from the clip's start, so its time is
        the clip's; any other playback file is in source time. */
    bool playbackInClipTime = false;

    /** For a clip that plays a proxy: the file the proxy is rendered from, drawn
        through segments until the proxy is ready. */
    std::unique_ptr<tracktion::SmartThumbnail> source;
    std::vector<Segment> segments;

    tracktion::RecordingThumbnailManager::Thumbnail::Ptr recordingThumbnail;
};

} // namespace resamper
