#pragma once

#include "Engine/ApplicationModel.h"

namespace papercut
{

class ThemeManager;

/** One audio clip in a lane: a themed box with its name and waveform.
    Its bounds are set by TrackLanes; it draws the part of the source file
    that its visible area covers. */
class ClipComponent : public juce::Component
{
public:
    ClipComponent (ApplicationModel&, ThemeManager&, const ClipInfo&);

    const ClipInfo& getClip() const noexcept   { return clip; }
    void setClip (const ClipInfo&);

    void paint (juce::Graphics&) override;

private:
    ApplicationModel& model;
    ThemeManager& themeManager;
    ClipInfo clip;
    std::unique_ptr<ClipWaveform> waveform;
};

} // namespace papercut
