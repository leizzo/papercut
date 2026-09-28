#pragma once

#include "Engine/Mixer.h"
#include "StripParts.h"

namespace papercut
{

class CommandRegistry;

/** The master strip (PRD §10.2, design `Master Strip`): accent colour bar,
    head, then the fader section — the master fader through
    mixer.setMasterVolume (a drag is one undo step) with a wide meter. The master
    has no pan. The track chain, inserts, loudness panel and Mono / Dim / Cue
    arrive with M2. */
class MasterStrip : public juce::Component
{
public:
    MasterStrip (CommandRegistry&, ThemeManager&);

    void setMaster (const MasterInfo&);
    void setLevel (StereoLevel level, double elapsedSeconds)   { faderSection.setLevel (level, elapsedSeconds); }
    void resetPeaks()                                          { faderSection.resetPeaks(); }
    void setMeterMode (MeterMode m)                            { faderSection.setMeterMode (m); }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    CommandRegistry& commands;
    ThemeManager& themeManager;
    FaderSection faderSection;
    juce::Rectangle<int> headArea;
};

} // namespace papercut
