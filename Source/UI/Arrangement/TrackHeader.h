#pragma once

#include "Engine/ApplicationModel.h"

namespace papercut
{

class CommandRegistry;
class ThemeManager;

/** One track's header: its name, mute and solo buttons, a pan knob and a volume
    fader. Every change goes through a Command; a fader or knob drag is one
    undo step. Clicks on the header's background fall through to the TrackList
    (which selects the track). */
class TrackHeader : public juce::Component
{
public:
    TrackHeader (CommandRegistry&, ThemeManager&, const TrackInfo&);

    const TrackInfo& getTrack() const noexcept   { return track; }
    void setTrack (const TrackInfo&);

    void paint (juce::Graphics&) override;
    void resized() override;

    /** Re-applies the Theme's mute and solo colours. */
    void applyTheme();

private:
    /** A slider that invokes a Command per value change, marking every value
        after the first of a drag as continuing that gesture. */
    struct GestureSlider : juce::Slider
    {
        std::function<void (double value, bool continuesGesture)> onGestureValue;

        void startedDragging() override   { dragging = true; sentInDrag = false; }
        void stoppedDragging() override   { dragging = false; }

        void valueChanged() override
        {
            if (onGestureValue)
                onGestureValue (getValue(), dragging && sentInDrag);

            sentInDrag = dragging;
        }

        bool dragging = false, sentInDrag = false;
    };

    CommandRegistry& commands;
    ThemeManager& themeManager;
    TrackInfo track;

    juce::TextButton muteButton { "M" }, soloButton { "S" };
    GestureSlider pan, volume;
};

} // namespace papercut
