#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace papercut
{

class ArrangementViewState;
class ThemeManager;

/** The time ruler above the lanes. */
class TimelineHeader : public juce::Component
{
public:
    TimelineHeader (ThemeManager&, ArrangementViewState&);

    void paint (juce::Graphics&) override;

private:
    ThemeManager& themeManager;
    ArrangementViewState& view;
};

} // namespace papercut
