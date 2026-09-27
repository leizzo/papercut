#pragma once

#include "Engine/ApplicationModel.h"

#include <optional>

namespace papercut
{

class ArrangementViewState;
class CommandRegistry;
class ThemeManager;

/** The time ruler above the lanes. It shows the loop range, bright while
    looping; dragging along it sets a new loop (transport.setLoopRange, on release). */
class TimelineHeader : public juce::Component
{
public:
    TimelineHeader (ApplicationModel&, CommandRegistry&, ThemeManager&, ArrangementViewState&);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    ApplicationModel& model;
    CommandRegistry& commands;
    ThemeManager& themeManager;
    ArrangementViewState& view;

    double dragStartSeconds = 0;
    std::optional<TimeRangeSeconds> draggedLoop;   ///< previewed until release
};

} // namespace papercut
