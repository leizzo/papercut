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
    juce::StringArray inputs;                ///< inputs of the track's kind it can record from
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
                     public juce::SettableTooltipClient,
                     public juce::DragAndDropTarget
{
public:
    enum class Section { io, inserts, sends, fader };

    ChannelStrip (CommandRegistry&, ThemeManager&);
    ~ChannelStrip() override;

    void setState (const StripState&);
    const StripState& getState() const noexcept   { return state; }

    void setLevel (StereoLevel, double elapsedSeconds);
    void resetPeaks();
    void setMeterMode (MeterMode m)   { faderSection.setMeterMode (m); }

    void setSectionVisible (Section, bool);

    /** The Track chain row was clicked: show this track's device chain. */
    std::function<void()> onTrackChainClicked;

    /** A right-click on the strip: its track's mixer menu (sends, bus). */
    std::function<void()> onShowMenu;

    /** The pointer is over a section (the toolbar's signal-flow indicator); -1: none. */
    std::function<void (int stage)> onFlowStageHovered;

    /** An insert slot wants the effects picker: to fill it (replacing empty) or to replace that insert. */
    std::function<void (InsertSlot&, const juce::String& replacing)> onPickInsert;

    /** A filled insert slot was clicked: open the plug-in's window. */
    std::function<void (const juce::String& pluginId)> onOpenPlugin;

    // Mixer inserts (PRD §10.6): drop a Browser effect on an empty slot,
    // drag an insert to reorder it, Alt+drag it onto another strip to copy.
    bool isInterestedInDragSource (const SourceDetails&) override;
    void itemDragMove (const SourceDetails&) override;
    void itemDragExit (const SourceDetails&) override;
    void itemDropped (const SourceDetails&) override;

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
    FaderSection faderSection;
    TrackButton mute, solo, arm;

    std::array<bool, 4> sectionShown { true, true, true, true };
    juce::Rectangle<int> headArea, ioArea, chainArea, chainLink, flowArea, insertsArea, sendsArea, panArea,
                         faderArea, buttonsArea;

    bool shown (Section s) const   { return sectionShown[(size_t) s]; }
    void setUpInsertSlot (InsertSlot&);
    void showInsertMenu (InsertSlot&);
    InsertSlot* slotAt (juce::Point<int>) const;

    /** Whether a drop on that slot would do something, and if not, why not. */
    juce::String dropRefusal (const SourceDetails&, const InsertSlot&) const;
    void clearDropHighlights();
    void rebuildSends();
    void rebuildInsertSlots();
    juce::String chainSummary() const;
    int flowStageAt (juce::Point<int>) const;
    void paintSectionHeader (juce::Graphics&, juce::Rectangle<int>, const juce::String& title, const juce::String& tag,
                             juce::Colour tagColour) const;
};

} // namespace papercut
