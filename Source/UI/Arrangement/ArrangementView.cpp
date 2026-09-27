#include "ArrangementView.h"
#include "UI/State/UIStateStore.h"

namespace papercut
{

namespace
{
    constexpr float wheelPixelsPerUnit = 300.0f;   // JUCE wheel deltas are fractions of a "notch"
    constexpr double zoomPerWheelUnit = 4.0;
}

ArrangementView::ArrangementView (ApplicationModel& m, CommandRegistry& commands, ThemeManager& tm, UIStateStore& uiState,
                                  Automation& autoLanes, Shaper& shapers)
    : model (m), automation (autoLanes), themeManager (tm), view (uiState.getState (componentId)),
      timeline (model, commands, themeManager, view),
      trackList (commands, themeManager, view),
      lanes (model, commands, themeManager, view),
      automationLane (model, automation, commands, themeManager, view),
      shaperPanel (model, shapers, commands, themeManager)
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

    for (auto* c : std::initializer_list<juce::Component*> { &timeline, &trackList, &lanes, &playhead,
                                                             &parameterBox, &shaperPanel, &automationLane })
        addAndMakeVisible (c);

    // Clicking a track header or an empty lane selects the track (engine
    // selection; never undoable, never through the UndoManager).
    trackList.onRowClicked = lanes.onRowClicked = [this] (int row) { selectRow (row); };
    lanes.onMidiClipOpened = [this] (const juce::String& id)
    {
        if (onMidiClipOpened)
            onMidiClipOpened (id);
    };

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

void ArrangementView::mouseMagnify (const juce::MouseEvent& e, float scaleFactor)
{
    view.zoomAround (scaleFactor, (float) e.getEventRelativeTo (&timeline).x);
}

} // namespace papercut
