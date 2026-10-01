#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

namespace resamper
{

/** A clip's waveform, generated on a background thread by the engine's
    thumbnail cache (te::SmartThumbnail), which repaints its target component
    as data arrives; or a recording's, growing as it records. Obtain one from
    ApplicationModel::createWaveform() or createRecordingWaveform().

    A warped clip's file is drawn where the clip plays it, following its trim
    and the tempo as they change.
*/
class ClipWaveform
{
public:
    struct Impl;

    explicit ClipWaveform (std::unique_ptr<Impl>);
    ~ClipWaveform();

    /** True until the waveform is complete: while its peak data is still being
        read on a background thread. */
    bool isGenerating() const;

    /** True once there is something to draw, perhaps partial. */
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
