#pragma once

#include "Engine/ApplicationModel.h"
#include "Engine/PluginRack.h"
#include "InsertSlot.h"
#include "StripParts.h"
#include "UI/Controls/Controls.h"

#include <memory>
#include <vector>

namespace papercut
{

class CommandRegistry;

/** What a strip shows for one track (PRD §10.2). */
struct StripState
{
    int number = 1;                          ///< 1-based position among the tracks
    TrackInfo track;
    std::vector<SendInfo> sends;
    std::vector<PluginInfo> inserts;         ///< the mixer inserts
    std::vector<PluginInfo> deviceChain;     ///< read-only, for the Track chain row
    juce::StringArray inputs;                ///< audio inputs the track can record from
    juce::String output = "Master";          ///< where it goes
    bool isReturn = false;                   ///< returns have no sends and no arm
    juce::String returnLetter;               ///< A..D on a return
};

/** One mixer channel strip, 145 wide (PRD §10.2), top to bottom in signal
    order: head (colour bar, number, name), I/O, the read-only Track chain row,
    a flow arrow, the mixer inserts, sends, channel pan, the fader section
    (gain and peak readouts, fader, stereo meter) and M / S / ●.

    Every change goes through a Command; a fader or knob drag is one undo step.
    Clicking the strip's background selects its track in every view. Whole
    sections can be hidden (the mixer's section chips); the fader takes the
    freed height. */
class ChannelStrip : public juce::Component,
                     public juce::SettableTooltipClient
{
public:
    enum class Section { io, inserts, sends, fader };

    ChannelStrip (CommandRegistry&, ThemeManager&);
    ~ChannelStrip() override;

    void setState (const StripState&);
    const StripState& getState() const noexcept   { return state; }

    void setLevel (StereoLevel, double elapsedSeconds);
    void resetPeaks();

    void setSectionVisible (Section, bool);

    /** The Track chain row was clicked: show this track's device chain. */
    std::function<void()> onTrackChainClicked;

    /** The pointer is over a section (the toolbar's signal-flow indicator); -1: none. */
    std::function<void (int stage)> onFlowStageHovered;

    /** Mixer insert slots, for the mixer's insert behaviour. */
    const std::vector<std::unique_ptr<InsertSlot>>& getInsertSlots() const noexcept   { return insertSlots; }

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    static constexpr int visibleInsertSlots = 4;

private:
    struct SendRow;

    CommandRegistry& commands;
    ThemeManager& themeManager;
    StripState state;
    juce::Colour colour;

    juce::ComboBox input;
    std::vector<std::unique_ptr<InsertSlot>> insertSlots;
    std::vector<std::unique_ptr<SendRow>> sendRows;
    Knob pan;
    ValueField gain;
    Fader fader;
    StereoMeter meter;
    TrackButton mute, solo, arm;

    std::array<bool, 4> sectionShown { true, true, true, true };
    juce::Rectangle<int> headArea, ioArea, chainArea, chainLink, flowArea, insertsArea, sendsArea, panArea,
                         faderArea, peakReadout, buttonsArea;

    bool shown (Section s) const   { return sectionShown[(size_t) s]; }
    void rebuildSends();
    void rebuildInsertSlots();
    juce::String chainSummary() const;
    int flowStageAt (juce::Point<int>) const;
    void paintSectionHeader (juce::Graphics&, juce::Rectangle<int>, const juce::String& title, const juce::String& tag,
                             juce::Colour tagColour) const;
};

} // namespace papercut
