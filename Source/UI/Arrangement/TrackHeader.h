#pragma once

#include "Engine/ApplicationModel.h"

namespace papercut
{

class CommandRegistry;
class ThemeManager;

/** One track's header: its name, mute, solo and arm buttons, its audio input,
    a pan knob and a volume fader. Every change goes through a Command; a fader
    or knob drag is one undo step. Clicks on the header's background fall through to the TrackList
    (which selects the track). */
class TrackHeader : public juce::Component
{
public:
    /** inputs: the audio inputs a track can record from. */
    TrackHeader (CommandRegistry&, ThemeManager&, const TrackInfo&, const juce::StringArray& inputs);

    const TrackInfo& getTrack() const noexcept   { return track; }
    void setTrack (const TrackInfo&, const juce::StringArray& inputs);

    void paint (juce::Graphics&) override;
    void resized() override;

    /** Re-applies the Theme's mute, solo and arm colours. */
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

    juce::TextButton muteButton { "M" }, soloButton { "S" }, armButton { "R" };
    juce::ComboBox input;
    juce::StringArray inputs;   ///< the input menu's items after "No Input"

    // The input menu's item IDs: inputs[i] has firstInputId + i.
    static constexpr int noInputId = 1, firstInputId = 2;
    GestureSlider pan, volume;
};

} // namespace papercut
