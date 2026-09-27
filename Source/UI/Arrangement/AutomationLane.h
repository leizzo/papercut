#pragma once

#include "Engine/ApplicationModel.h"
#include "Engine/Automation.h"
#include "UI/Theme/ThemeManager.h"

#include <optional>
#include <vector>

namespace papercut
{

class ArrangementViewState;
class CommandRegistry;

/** Breakpoint lane for one parameter. Local x is the timeline x from
    ArrangementViewState. Hosts should give it LayoutMetrics::trackHeight.

    A click on empty lane adds a point; dragging a point moves it. Both go
    through Commands, committed on mouse-up as one step.
*/
class AutomationLane : public juce::Component,
                       private ApplicationModel::Listener,
                       private ThemeManager::Listener,
                       private juce::ValueTree::Listener
{
public:
    AutomationLane (ApplicationModel&, Automation&, CommandRegistry&, ThemeManager&, ArrangementViewState&);
    ~AutomationLane() override;

    void setTarget (const juce::String& trackId, const juce::String& parameterKey);

    /** trackHeight from the layout metrics. */
    int getPreferredHeight() const;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    ApplicationModel& model;
    Automation& automation;
    CommandRegistry& commands;
    ThemeManager& themeManager;
    ArrangementViewState& view;

    juce::String trackId, parameterKey;
    std::vector<AutomationPointInfo> points;

    bool dragging = false;
    bool adding = false;
    std::optional<int> draggedIndex;
    std::optional<AutomationPointInfo> preview;

    struct ValueSpan { float min = 0, max = 1; };

    void reload();
    ValueSpan valueSpan() const;
    juce::Rectangle<int> curveArea() const;
    juce::Point<float> pointToXY (const AutomationPointInfo&) const;
    AutomationPointInfo pointFromPosition (juce::Point<float>) const;
    std::optional<int> pointAt (juce::Point<float>) const;

    void modelChanged() override;
    void themeChanged() override { repaint(); }
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override { repaint(); }
};

} // namespace papercut
