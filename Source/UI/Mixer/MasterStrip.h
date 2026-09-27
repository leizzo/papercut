#pragma once

#include "Engine/Mixer.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace papercut
{

class CommandRegistry;
class ThemeManager;

/** The Edit's master fader: volume and pan through mixer.setMasterVolume and
    mixer.setMasterPan. A drag is one undo step. */
class MasterStrip : public juce::Component
{
public:
    MasterStrip (CommandRegistry&, ThemeManager&);

    void setMaster (const MasterInfo&);

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

    CommandRegistry& commands;
    ThemeManager& themeManager;
    MasterInfo master;
    GestureSlider pan, volume;
};

} // namespace papercut
