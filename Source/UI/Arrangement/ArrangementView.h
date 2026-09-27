#pragma once

#include "AutomationLane.h"
#include "Playhead.h"
#include "ShaperPanel.h"
#include "TimelineHeader.h"
#include "TrackLanes.h"
#include "TrackList.h"
#include "UI/State/ArrangementViewState.h"
#include "UI/Theme/ThemeManager.h"

namespace papercut
{

class CommandRegistry;
class UIStateStore;

/** The hand-coded Arrangement: ruler, track headers, lanes with clips, and
    the playhead, all positioned through one ArrangementViewState.

    Mouse wheel scrolls (vertical, and horizontal with shift or a trackpad);
    cmd/ctrl + wheel zooms around the pointer. Clip gestures live in TrackLanes.
*/
class ArrangementView : public juce::Component,
                        private ApplicationModel::Listener,
                        private ThemeManager::Listener,
                        private juce::ValueTree::Listener
{
public:
    static constexpr const char* componentId = "arrangement";

    ArrangementView (ApplicationModel&, CommandRegistry&, ThemeManager&, UIStateStore&, Automation&, Shaper&);
    ~ArrangementView() override;

    /** Double-click on a MIDI clip. */
    std::function<void (const juce::String& clipId)> onMidiClipOpened;

    void resized() override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMagnify (const juce::MouseEvent&, float scaleFactor) override;

private:
    ApplicationModel& model;
    Automation& automation;
    ThemeManager& themeManager;
    ArrangementViewState view;

    TimelineHeader timeline;
    TrackList trackList;
    TrackLanes lanes;
    Playhead playhead { model, themeManager, view };
    AutomationLane automationLane;
    ShaperPanel shaperPanel;
    juce::ComboBox parameterBox;
    std::vector<TrackInfo> tracks;
    juce::String shownTrackId;
    juce::String parameterKey { "volume" };

    void refresh();
    void syncAutomationTarget();
    void clampVerticalScroll();
    void selectRow (int row);
    void styleParameterBox();

    void modelChanged() override        { refresh(); }
    void themeChanged() override        { trackList.applyTheme(); styleParameterBox(); repaint(); }
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;
};

} // namespace papercut
