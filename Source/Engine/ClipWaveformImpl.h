#pragma once

#include "ClipWaveform.h"

#include <tracktion_engine/tracktion_engine.h>

namespace papercut
{

/** Either a clip's file thumbnail or a recording's growing one. */
struct ClipWaveform::Impl
{
    // No Edit is passed: it is only used to choose a cache folder, and a
    // waveform must never outlive-reference an Edit that a Project Open replaced.
    Impl (tracktion::Engine& engine, const juce::File& file, juce::Component& repaintTarget)
        : fileThumbnail (std::make_unique<tracktion::SmartThumbnail> (engine, tracktion::AudioFile (engine, file), repaintTarget, nullptr))
    {
    }

    explicit Impl (tracktion::RecordingThumbnailManager::Thumbnail::Ptr recording)
        : recordingThumbnail (std::move (recording))
    {
    }

    std::unique_ptr<tracktion::SmartThumbnail> fileThumbnail;
    tracktion::RecordingThumbnailManager::Thumbnail::Ptr recordingThumbnail;
};

} // namespace papercut
