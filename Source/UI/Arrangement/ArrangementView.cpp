#include "ArrangementView.h"
#include "UI/Browser/Library.h"
#include "UI/State/UIStateStore.h"
#include "UI/Theme/Interaction.h"

namespace papercut
{

namespace
{
    constexpr float wheelPixelsPerUnit = 300.0f;   // JUCE wheel deltas are fractions of a "notch"
    constexpr double zoomPerWheelUnit = 4.0;
}

ArrangementView::ArrangementView (ApplicationModel& m, CommandRegistry& c, ThemeManager& tm, UIStateStore& uiState,
                                  Automation& autoLanes, Shaper& shapers)
    : model (m), automation (autoLanes), themeManager (tm), view (uiState.getState (componentId)),
      timeline (model, c, themeManager, view),
      trackList (c, themeManager, view),
      lanes (model, c, themeManager, view),
      automationLane (model, automation, c, themeManager, view),
      shaperPanel (model, shapers, c, themeManager),
      commands (c)
{
    setComponentID (componentId);

    parameterBox.onChange = [this]
    {
        const auto row = parameterBox.getSelectedItemIndex();
        const auto targets = automation.getTargets (shownTrackId);

        if (! juce::isPositiveAndBelow (row, (int) targets.size()))
            return;

        parameterKey = targets[(size_t) row].key;
        automationLane.setTarget (shownTrackId, parameterKey);
        shaperPanel.setParameterKey (parameterKey);
    };

    for (auto* child : std::initializer_list<juce::Component*> { &timeline, &trackList, &lanes, &playhead,
                                                                 &parameterBox, &shaperPanel, &automationLane })
        addAndMakeVisible (child);

    // Clicking a track header or an empty lane selects the track (engine
    // selection; never undoable, never through the UndoManager).
    trackList.onRowClicked = lanes.onRowClicked = [this] (int row) { selectRow (row); };
    lanes.onMidiClipOpened = [this] (const juce::String& id) { if (onMidiClipOpened) onMidiClipOpened (id); };
    lanes.onAudioClipOpened = [this] (const juce::String& id) { if (onAudioClipOpened) onAudioClipOpened (id); };

    model.addListener (this);
    themeManager.addListener (this);
    view.getState().addListener (this);
    styleParameterBox();
    refresh();
}

ArrangementView::~ArrangementView()
{
    view.getState().removeListener (this);
    themeManager.removeListener (this);
    model.removeListener (this);
}

void ArrangementView::refresh()
{
    tracks = model.getTracks();
    trackList.setTracks (tracks, model.getAudioInputs());
    lanes.setTracks (tracks);
    syncAutomationTarget();
    timeline.repaint();   // the loop
    clampVerticalScroll();
}

void ArrangementView::syncAutomationTarget()
{
    auto trackId = model.getSelectedTrackId();

    if (trackId.isEmpty() && ! tracks.empty())
        trackId = tracks.front().id;

    if (trackId != shownTrackId)
    {
        shownTrackId = trackId;
        parameterKey = "volume";
        shaperPanel.setTrack (trackId, parameterKey);
    }

    const auto targets = automation.getTargets (trackId);
    parameterBox.clear (juce::dontSendNotification);
    auto selectedId = 0;

    for (int i = 0; i < (int) targets.size(); ++i)
    {
        parameterBox.addItem (targets[(size_t) i].name, i + 1);

        if (targets[(size_t) i].key == parameterKey)
            selectedId = i + 1;
    }

    if (selectedId == 0 && ! targets.empty())
    {
        parameterKey = targets.front().key;
        selectedId = 1;
        shaperPanel.setParameterKey (parameterKey);
    }

    parameterBox.setSelectedId (selectedId, juce::dontSendNotification);
    automationLane.setTarget (trackId, parameterKey);
}

void ArrangementView::styleParameterBox()
{
    auto& theme = themeManager.getTheme();
    parameterBox.setColour (juce::ComboBox::backgroundColourId, theme.background);
    parameterBox.setColour (juce::ComboBox::textColourId, theme.text);
    parameterBox.setColour (juce::ComboBox::outlineColourId, theme.gridLine);
}

void ArrangementView::paint (juce::Graphics& g)
{
    // The corner above the headers, beside the ruler.
    auto& theme = themeManager.getTheme();
    auto corner = juce::Rectangle<int> (0, 0, trackList.getWidth(), timeline.getHeight());
    g.setColour (theme.bgPanel);
    g.fillRect (corner);
    g.setColour (theme.borderSoft);
    g.fillRect (corner.removeFromBottom (1));
    g.fillRect (corner.getRight() - 1, 0, 1, timeline.getHeight());
    drawStyledText (g, themeManager, "Bars", theme.caption, juce::Rectangle<int> (12, 0, trackList.getWidth() - 12, timeline.getHeight()),
                    juce::Justification::centredLeft, theme.textDim);
}

void ArrangementView::resized()
{
    auto& metrics = themeManager.getMetrics();
    auto r = getLocalBounds();
    const auto strip = juce::jlimit (metrics.trackHeight * 2,
                                      shaperPanel.getPreferredHeight() + metrics.timelineHeight,
                                      juce::jmax (metrics.trackHeight * 2, r.getHeight() / 3));
    auto bottom = r.removeFromBottom (strip);
    auto bottomLeft = bottom.removeFromLeft (metrics.trackHeaderWidth);
    parameterBox.setBounds (bottomLeft.removeFromTop (metrics.timelineHeight).reduced (metrics.inset, 0));
    shaperPanel.setBounds (bottomLeft);
    automationLane.setBounds (bottom);

    auto left = r.removeFromLeft (metrics.trackHeaderWidth);
    left.removeFromTop (metrics.timelineHeight);

    trackList.setBounds (left);
    playhead.setBounds (r);
    timeline.setBounds (r.removeFromTop (metrics.timelineHeight));
    lanes.setBounds (r);

    clampVerticalScroll();
    lanes.layoutClips();
}

void ArrangementView::clampVerticalScroll()
{
    const auto contentHeight = (int) tracks.size() * themeManager.getMetrics().trackHeight;
    const auto maxScroll = std::max (0, contentHeight - lanes.getHeight());

    if (view.getScrollY() > maxScroll)
        view.setScrollY (maxScroll);
}

void ArrangementView::selectRow (int row)
{
    model.selectTrack (juce::isPositiveAndBelow (row, (int) tracks.size()) ? tracks[(size_t) row].id : juce::String());
}

void ArrangementView::valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&)
{
    // Zoom or scroll changed: everything re-derives its position from the view state.
    lanes.layoutClips();
    lanes.repaint();
    trackList.layoutHeaders();
    timeline.repaint();
    playhead.update();
}

void ArrangementView::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    const auto timelineX = (float) e.getEventRelativeTo (&timeline).x;

    if (e.mods.isCommandDown() || e.mods.isCtrlDown())
    {
        view.zoomAround (std::pow (2.0, wheel.deltaY * zoomPerWheelUnit), timelineX);
        return;
    }

    const auto dx = e.mods.isShiftDown() ? wheel.deltaY : wheel.deltaX;
    const auto dy = e.mods.isShiftDown() ? 0.0f : wheel.deltaY;

    if (dx != 0.0f)
        view.setScrollSeconds (view.getScrollSeconds() - dx * wheelPixelsPerUnit / view.getPixelsPerSecond());

    if (dy != 0.0f)
    {
        view.setScrollY (view.getScrollY() - juce::roundToInt (dy * wheelPixelsPerUnit));
        clampVerticalScroll();
    }
}

ArrangementView::DropTarget ArrangementView::dropTargetAt (const SourceDetails& details) const
{
    const auto item = itemFromDrag (details.description);
    const auto lanePoint = lanes.getLocalPoint (this, details.localPosition);
    const auto row = view.yToRow (lanePoint.y, themeManager.getMetrics().trackHeight);

    if (! item || ! juce::isPositiveAndBelow (row, (int) tracks.size())
        || ! (lanes.getBounds().contains (details.localPosition) || trackList.getBounds().contains (details.localPosition)))
        return {};

    return { row, canDropOnTrack (*item, tracks[(size_t) row].kind) };
}

bool ArrangementView::isInterestedInDragSource (const SourceDetails& details)
{
    return itemFromDrag (details.description).has_value();
}

void ArrangementView::itemDragMove (const SourceDetails& details)
{
    const auto target = dropTargetAt (details);

    if (target.row != dropTarget.row || target.valid != dropTarget.valid)
    {
        dropTarget = target;
        setMouseCursor (target.row >= 0 && ! target.valid ? notAllowedCursor() : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void ArrangementView::itemDragExit (const SourceDetails&)
{
    dropTarget = {};
    setMouseCursor (juce::MouseCursor::NormalCursor);
    repaint();
}

void ArrangementView::itemDropped (const SourceDetails& details)
{
    const auto target = dropTargetAt (details);
    itemDragExit (details);

    if (auto item = itemFromDrag (details.description); item && target.valid)
    {
        // Onto a lane: where it was dropped. Onto a header: at the insert marker.
        const auto x = (float) lanes.getLocalPoint (this, details.localPosition).x;
        const auto seconds = lanes.getBounds().contains (details.localPosition) ? std::max (0.0, view.xToTime (x))
                                                                                : model.getInsertMarkerSeconds();
        dropOnTrack (commands, *item, tracks[(size_t) target.row].id, seconds);
    }
}

void ArrangementView::paintOverChildren (juce::Graphics& g)
{
    if (dropTarget.row < 0 || ! dropTarget.valid)
        return;

    // Valid targets get an accent-dim outline (§16.3).
    const auto rowHeight = themeManager.getMetrics().trackHeight;
    const auto y = lanes.getY() + view.rowToY (dropTarget.row, rowHeight);
    g.setColour (themeManager.getTheme().accentDim);
    g.drawRoundedRectangle (juce::Rectangle<int> (trackList.getX(), y, lanes.getRight() - trackList.getX(), rowHeight)
                                .toFloat().reduced (1.0f),
                            themeManager.getTheme().radiusMd, 2.0f);
}

void ArrangementView::mouseMagnify (const juce::MouseEvent& e, float scaleFactor)
{
    view.zoomAround (scaleFactor, (float) e.getEventRelativeTo (&timeline).x);
}

} // namespace papercut
