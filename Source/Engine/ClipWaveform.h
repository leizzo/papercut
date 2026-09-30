#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

namespace resamper
{

/** A clip's waveform, generated on a background thread by the engine's
    thumbnail cache (te::SmartThumbnail), which repaints its target component
    as data arrives; or a recording's, growing as it records. Obtain one from
    ApplicationModel::createWaveform() or createRecordingWaveform().

    A time-stretched clip plays from a proxy file that takes a while to render;
    until it is ready the waveform draws the source file, mapped to where the
    clip plays it.
*/
class ClipWaveform
{
public:
    struct Impl;

    explicit ClipWaveform (std::unique_ptr<Impl>);
    ~ClipWaveform();

    /** True until the waveform is complete: while the clip's audio (a
        time-stretched proxy) is still to be produced, or its peak data is still
        being read on a background thread. */
    bool isGenerating() const;

    /** True once there is something to draw, perhaps partial or from the source
        file while a proxy renders. */
    bool hasDrawableAudio() const;

    /** 0..1 while generating; 1 when complete. */
    double getProgress() const;

    /** Draws the clip's time range [clipStartSeconds, clipEndSeconds] (seconds
        from the clip's start) into area, one lane per channel. sourceOffsetSeconds
        is where in its source the clip starts (ClipInfo::sourceOffsetSeconds). */
    void draw (juce::Graphics&, juce::Rectangle<int> area,
               double clipStartSeconds, double clipEndSeconds, double sourceOffsetSeconds) const;

private:
    std::unique_ptr<Impl> impl;

    JUCE_DECLARE_NON_COPYABLE (ClipWaveform)
};

} // namespace resamper
