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
                        public juce::DragAndDropTarget,
                        private ApplicationModel::Listener,
                        private ThemeManager::Listener,
                        private juce::ValueTree::Listener
{
public:
    static constexpr const char* componentId = "arrangement";

    ArrangementView (ApplicationModel&, CommandRegistry&, ThemeManager&, UIStateStore&, Automation&, Shaper&);
    ~ArrangementView() override;

    /** Double-click on a MIDI clip (Piano Roll) or an audio clip (Editor). */
    std::function<void (const juce::String& clipId)> onMidiClipOpened, onAudioClipOpened;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMagnify (const juce::MouseEvent&, float scaleFactor) override;
    void paintOverChildren (juce::Graphics&) override;

    // Browser items dropped on a track header or lane (PRD §6.2)
    bool isInterestedInDragSource (const SourceDetails&) override;
    void itemDragMove (const SourceDetails&) override;
    void itemDragExit (const SourceDetails&) override;
    void itemDropped (const SourceDetails&) override;

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
    CommandRegistry& commands;

    struct DropTarget
    {
        int row = -1;
        bool valid = false;
    };

    DropTarget dropTarget;
    DropTarget dropTargetAt (const SourceDetails&) const;

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
