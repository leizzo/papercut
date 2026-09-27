#pragma once

#include "ClipWaveform.h"

#include <tracktion_engine/tracktion_engine.h>

namespace papercut
{

struct ClipWaveform::Impl
{
    // No Edit is passed: it is only used to choose a cache folder, and a
    // waveform must never outlive-reference an Edit that a Project Open replaced.
    Impl (tracktion::Engine& engine, const juce::File& file, juce::Component& repaintTarget)
        : thumbnail (engine, tracktion::AudioFile (engine, file), repaintTarget, nullptr)
    {
    }

    tracktion::SmartThumbnail thumbnail;
};

} // namespace papercut
