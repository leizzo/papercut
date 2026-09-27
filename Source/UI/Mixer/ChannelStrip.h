#pragma once

#include "Engine/ApplicationModel.h"
#include "Engine/Mixer.h"

#include <memory>
#include <vector>

namespace papercut
{

class CommandRegistry;
class ThemeManager;

/** One mixer channel: volume, pan, mute and solo (the existing track Commands),
    plus that track's sends and the names of its inserts.

    A fader or knob drag is one undo step. Insert names are display only.
*/
class ChannelStrip : public juce::Component
{
public:
    ChannelStrip (CommandRegistry&, ThemeManager&, const TrackInfo&,
                  const std::vector<SendInfo>&, const std::vector<InsertSummary>&);

    void setState (const TrackInfo&, const std::vector<SendInfo>&, const std::vector<InsertSummary>&);

    void paint (juce::Graphics&) override;
    void resized() override;
    void applyTheme();

private:
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

    struct SendRow : juce::Component
    {
        explicit SendRow (ThemeManager&);

        juce::String sendId;
        bool muted = false;
        juce::TextButton mute { "M" };
        GestureSlider gain;

        void resized() override;
        void applyTheme();

        ThemeManager& themeManager;
    };

    CommandRegistry& commands;
    ThemeManager& themeManager;
    TrackInfo track;
    std::vector<SendInfo> sends;

    juce::TextButton muteButton { "M" }, soloButton { "S" };
    GestureSlider pan, volume;
    std::vector<std::unique_ptr<SendRow>> sendRows;
    std::vector<std::unique_ptr<juce::Label>> insertLabels;

    void rebuildSends (const std::vector<SendInfo>&);
    void rebuildInserts (const std::vector<InsertSummary>&);
};

} // namespace papercut
