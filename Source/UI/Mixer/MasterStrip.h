#pragma once

#include "Engine/Mixer.h"
#include "StripParts.h"

namespace papercut
{

class CommandRegistry;

/** The master strip (PRD §10.2): head, pan, gain readout, the master fader
    and a stereo meter, through mixer.setMasterVolume / setMasterPan. A drag is
    one undo step. The loudness panel and Mono / Dim / Cue arrive with M2. */
class MasterStrip : public juce::Component
{
public:
    MasterStrip (CommandRegistry&, ThemeManager&);

    void setMaster (const MasterInfo&);
    void setLevel (StereoLevel, double elapsedSeconds);
    void resetPeaks()   { meter.resetPeaks(); }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    CommandRegistry& commands;
    ThemeManager& themeManager;
    Knob pan;
    ValueField gain;
    Fader fader;
    StereoMeter meter;
};

} // namespace papercut
