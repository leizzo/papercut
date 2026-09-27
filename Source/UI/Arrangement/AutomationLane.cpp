#include "AutomationLane.h"
#include "Commands/AutomationCommands.h"
#include "UI/State/ArrangementViewState.h"

#include <algorithm>

namespace papercut
{

AutomationLane::AutomationLane (ApplicationModel& m, Automation& a, CommandRegistry& c, ThemeManager& tm,
                                ArrangementViewState& v)
    : model (m), automation (a), commands (c), themeManager (tm), view (v)
{
    setOpaque (true);
    model.addListener (this);
    themeManager.addListener (this);
    view.getState().addListener (this);
}

AutomationLane::~AutomationLane()
{
    view.getState().removeListener (this);
    themeManager.removeListener (this);
    model.removeListener (this);
}

void AutomationLane::setTarget (const juce::String& newTrackId, const juce::String& newParameterKey)
{
    trackId = newTrackId;
    parameterKey = newParameterKey;
    reload();
}

int AutomationLane::getPreferredHeight() const
{
    return themeManager.getMetrics().trackHeight;
}

void AutomationLane::reload()
{
    points = trackId.isEmpty() || parameterKey.isEmpty() ? std::vector<AutomationPointInfo>()
                                                         : automation.getPoints (trackId, parameterKey);
    repaint();
}

AutomationLane::ValueSpan AutomationLane::valueSpan() const
{
    if (parameterKey == "volume" || parameterKey.startsWith ("send:"))
        return { (float) ApplicationModel::minVolumeDb, (float) ApplicationModel::maxVolumeDb };

    if (parameterKey == "pan")
        return { -1.0f, 1.0f };

    return { 0.0f, 1.0f };
}

juce::Rectangle<int> AutomationLane::curveArea() const
{
    auto area = getLocalBounds();
    area.removeFromTop (themeManager.getMetrics().timelineHeight);
    return area;
}

juce::Point<float> AutomationLane::pointToXY (const AutomationPointInfo& point) const
{
    auto body = curveArea().toFloat();
    const auto span = valueSpan();
    const auto width = span.max - span.min;
    const auto norm = width == 0.0f ? 0.0f : (point.value - span.min) / width;
    return { view.timeToX (point.timeSeconds),
             body.getBottom() - juce::jlimit (0.0f, 1.0f, norm) * body.getHeight() };
}

AutomationPointInfo AutomationLane::pointFromPosition (juce::Point<float> position) const
{
    auto body = curveArea().toFloat();
    const auto span = valueSpan();
    const auto norm = body.getHeight() <= 0.0f ? 0.0f
                                                : juce::jlimit (0.0f, 1.0f, 1.0f - (position.y - body.getY()) / body.getHeight());

    AutomationPointInfo point;
    point.timeSeconds = juce::jmax (0.0, view.xToTime (position.x));
    point.value = span.min + norm * (span.max - span.min);
    return point;
}

std::optional<int> AutomationLane::pointAt (juce::Point<float> position) const
{
    // timelineHeight is the grab radius so a point is hittable without a new metric.
    const auto hit = (float) themeManager.getMetrics().timelineHeight * 0.5f;
    std::optional<int> best;
    auto bestDistance = hit * hit;

    for (auto& point : points)
    {
        const auto distance = pointToXY (point).getDistanceSquaredFrom (position);

        if (distance <= bestDistance)
        {
            bestDistance = distance;
            best = point.index;
        }
    }

    return best;
}

void AutomationLane::paint (juce::Graphics& g)
{
    auto& theme = themeManager.getTheme();
    auto& metrics = themeManager.getMetrics();

    g.fillAll (theme.laneA);

    auto header = getLocalBounds().removeFromTop (metrics.timelineHeight);
    g.setColour (theme.panel);
    g.fillRect (header);
    g.setColour (theme.mutedText);
    g.setFont (themeManager.getFont (0.8f));
    g.drawText (parameterKey.isEmpty() ? "Automation" : parameterKey,
                header.reduced (metrics.textPadding, 0), juce::Justification::centredLeft, true);

    auto shown = points;

    if (dragging && preview.has_value())
    {
        if (draggedIndex.has_value())
        {
            for (auto& point : shown)
                if (point.index == *draggedIndex)
                    point = *preview;
        }
        else
        {
            shown.push_back (*preview);
        }
    }

    std::sort (shown.begin(), shown.end(), [] (const AutomationPointInfo& a, const AutomationPointInfo& b) {
        return a.timeSeconds < b.timeSeconds;
    });

    const float unity = parameterKey == "volume" || parameterKey.startsWith ("send:") ? 0.0f
                                                                                     : parameterKey == "pan" ? 0.0f : 0.5f;
    AutomationPointInfo reference;
    reference.value = unity;
    const auto referenceY = pointToXY (reference).y;
    g.setColour (theme.gridLine);
    g.drawHorizontalLine (juce::roundToInt (referenceY), 0.0f, (float) getWidth());

    juce::Path line;
    const float radius = (float) juce::jmax (metrics.playheadWidth, metrics.timelineHeight / 4);

    for (size_t i = 0; i < shown.size(); ++i)
    {
        const auto xy = pointToXY (shown[i]);

        if (i == 0)
            line.startNewSubPath (xy);
        else
            line.lineTo (xy);
    }

    g.setColour (theme.accent);
    g.strokePath (line, juce::PathStrokeType ((float) metrics.playheadWidth));

    for (auto& point : shown)
    {
        const auto xy = pointToXY (point);
        g.fillEllipse (xy.x - radius * 0.5f, xy.y - radius * 0.5f, radius, radius);
    }
}

void AutomationLane::mouseDown (const juce::MouseEvent& e)
{
    if (trackId.isEmpty() || parameterKey.isEmpty() || e.mods.isPopupMenu())
        return;

    reload();
    draggedIndex = pointAt (e.position);
    adding = ! draggedIndex.has_value();
    preview = pointFromPosition (e.position);

    if (draggedIndex.has_value())
        preview->index = *draggedIndex;

    dragging = true;
    repaint();
}

void AutomationLane::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging)
        return;

    preview = pointFromPosition (e.position);

    if (draggedIndex.has_value())
        preview->index = *draggedIndex;

    repaint();
}

void AutomationLane::mouseUp (const juce::MouseEvent& e)
{
    if (! dragging)
        return;

    dragging = false;
    auto point = pointFromPosition (e.position);

    if (draggedIndex.has_value())
        commands.invoke ("automation.movePoint", automationMoveArgs (trackId, parameterKey, *draggedIndex,
                                                                     point.timeSeconds, point.value));
    else if (adding)
        commands.invoke ("automation.addPoint", automationPointArgs (trackId, parameterKey, point.timeSeconds, point.value));

    draggedIndex.reset();
    preview.reset();
    adding = false;
    reload();
}

void AutomationLane::modelChanged()
{
    if (! dragging)
        reload();
    else
        repaint();
}

} // namespace papercut
