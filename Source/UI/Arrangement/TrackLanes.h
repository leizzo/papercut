#pragma once

#include "ClipComponent.h"

#include <map>

namespace papercut
{

class ArrangementViewState;

/** One lane per track, holding that track's ClipComponents at their time
    positions. Clip components are kept by clip ID across model updates so their
    waveforms aren't regenerated. */
class TrackLanes : public juce::Component
{
public:
    TrackLanes (ApplicationModel&, ThemeManager&, ArrangementViewState&);

    void setTracks (const std::vector<TrackInfo>&);

    /** Re-positions every clip from the view state (after zoom/scroll). */
    void layoutClips();

    void paint (juce::Graphics&) override;
    void resized() override   { layoutClips(); }
    void mouseDown (const juce::MouseEvent&) override;

    /** Called with the row index under a click (may be out of range). */
    std::function<void (int row)> onRowClicked;

private:
    ApplicationModel& model;
    ThemeManager& themeManager;
    ArrangementViewState& view;

    std::vector<TrackInfo> tracks;
    std::map<juce::String, std::unique_ptr<ClipComponent>> clips;
};

} // namespace papercut
