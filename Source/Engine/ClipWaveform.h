#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

namespace papercut
{

/** A clip's waveform, generated on a background thread by the engine's
    thumbnail cache (te::SmartThumbnail). Repaints its target component as
    data arrives. Obtain one from ApplicationModel::createWaveform().
*/
class ClipWaveform
{
public:
    struct Impl;

    explicit ClipWaveform (std::unique_ptr<Impl>);
    ~ClipWaveform();

    /** True while the background thread is still producing peak data. */
    bool isGenerating() const;

    /** 0..1 while generating; 1 when complete. */
    double getProgress() const;

    /** Draws the source-file range [sourceStartSeconds, sourceEndSeconds] into area,
        one lane per channel. */
    void draw (juce::Graphics&, juce::Rectangle<int> area,
               double sourceStartSeconds, double sourceEndSeconds) const;

private:
    std::unique_ptr<Impl> impl;

    JUCE_DECLARE_NON_COPYABLE (ClipWaveform)
};

} // namespace papercut
