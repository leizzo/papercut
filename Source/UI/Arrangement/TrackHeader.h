#pragma once

#include "Engine/ApplicationModel.h"
#include "UI/Controls/Controls.h"

namespace papercut
{

class CommandRegistry;

/** One track's header (PRD §8.1, §8.4): fold chevron, colour dot, name, and
    Arm / Solo / Mute / Auto (a return has no Arm). The right-click menu sets
    the colour and the input: audio inputs on an audio track, MIDI inputs on a
    MIDI track. Volume and pan live in the mixer.

    States: armed, solo and muted fill their buttons; Auto shows a lime tint
    and turns the chevron down; a selected header is bg-elevated with an
    accent left edge. Clicks on the background fall through to the TrackList,
    which selects the track. */
class TrackHeader : public juce::Component
{
public:
    /** inputs: the inputs the track can record from (of its kind). */
    TrackHeader (CommandRegistry&, ThemeManager&, const TrackInfo&, const juce::StringArray& inputs);

    const TrackInfo& getTrack() const noexcept   { return track; }
    void setTrack (const TrackInfo&, const juce::StringArray& inputs, bool automationShown);

    /** The Auto button or the chevron. */
    std::function<void()> onToggleAutomation;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

    /** Kept for the view's theme pass; the header reads the Theme when it paints. */
    void applyTheme()   { repaint(); }

private:
    CommandRegistry& commands;
    ThemeManager& themeManager;
    TrackInfo track;
    juce::StringArray inputs;
    bool automationShown = false;

    TrackButton arm, solo, mute, automation;

    juce::Rectangle<int> chevronBounds() const;
    void showMenu();
};

} // namespace papercut
