#include "ArrangementView.h"
#include "UI/State/UIStateStore.h"

namespace papercut
{

namespace
{
    constexpr float wheelPixelsPerUnit = 300.0f;   // JUCE wheel deltas are fractions of a "notch"
    constexpr double zoomPerWheelUnit = 4.0;
}

ArrangementView::ArrangementView (ApplicationModel& m, CommandRegistry& commands, ThemeManager& tm, UIStateStore& uiState)
    : model (m), themeManager (tm), view (uiState.getState (componentId)),
      lanes (model, commands, themeManager, view)
{
    setComponentID (componentId);

    for (auto* c : std::initializer_list<juce::Component*> { &timeline, &trackList, &lanes, &playhead })
        addAndMakeVisible (c);

    // Clicking a track header or an empty lane selects the track (engine
    // selection; never undoable, never through the UndoManager).
    trackList.onRowClicked = lanes.onRowClicked = [this] (int row) { selectRow (row); };

    model.addListener (this);
    themeManager.addListener (this);
    view.getState().addListener (this);
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
    trackList.setTracks (tracks);
    lanes.setTracks (tracks);
    clampVerticalScroll();
}

void ArrangementView::resized()
{
    auto& metrics = themeManager.getMetrics();
    auto r = getLocalBounds();
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
    trackList.repaint();
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

void ArrangementView::mouseMagnify (const juce::MouseEvent& e, float scaleFactor)
{
    view.zoomAround (scaleFactor, (float) e.getEventRelativeTo (&timeline).x);
}

} // namespace papercut
