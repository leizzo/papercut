#pragma once

#include "Playhead.h"
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

    ArrangementView (ApplicationModel&, CommandRegistry&, ThemeManager&, UIStateStore&);
    ~ArrangementView() override;

    /** Double-click on a MIDI clip. */
    std::function<void (const juce::String& clipId)> onMidiClipOpened;

    void resized() override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMagnify (const juce::MouseEvent&, float scaleFactor) override;

private:
    ApplicationModel& model;
    ThemeManager& themeManager;
    ArrangementViewState view;

    TimelineHeader timeline;
    TrackList trackList;
    TrackLanes lanes;
    Playhead playhead { model, themeManager, view };
    std::vector<TrackInfo> tracks;

    void refresh();
    void clampVerticalScroll();
    void selectRow (int row);

    void modelChanged() override        { refresh(); }
    void themeChanged() override        { trackList.applyTheme(); repaint(); }
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;
};

} // namespace papercut
