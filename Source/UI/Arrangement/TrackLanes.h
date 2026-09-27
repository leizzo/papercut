#pragma once

#include "ClipComponent.h"

#include <map>
#include <optional>

namespace papercut
{

class ArrangementViewState;
class CommandRegistry;

/** One lane per track, holding that track's ClipComponents at their time
    positions. Clip components are kept by clip ID across model updates so their
    waveforms aren't regenerated.

    Clip gestures: clicking a clip selects it; dragging its body moves it (across
    lanes too), dragging an edge resizes it. The drag is previewed here and
    committed on release as one clip.move / clip.resize Command.
*/
class TrackLanes : public juce::Component
{
public:
    TrackLanes (ApplicationModel&, CommandRegistry&, ThemeManager&, ArrangementViewState&);

    void setTracks (const std::vector<TrackInfo>&);

    /** Re-positions every clip from the view state (after zoom/scroll). */
    void layoutClips();

    void paint (juce::Graphics&) override;
    void resized() override   { layoutClips(); }
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

    /** Called with the row index under a click on an empty lane (may be out of range). */
    std::function<void (int row)> onRowClicked;

private:
    enum class DragMode { move, resizeStart, resizeEnd };

    struct Drag
    {
        DragMode mode;
        ClipInfo original, preview;
        int row = 0;               ///< the preview's row
        double grabSeconds = 0;    ///< timeline position under the pointer at mouse-down
    };

    ApplicationModel& model;
    CommandRegistry& commands;
    ThemeManager& themeManager;
    ArrangementViewState& view;

    std::vector<TrackInfo> tracks;
    std::map<juce::String, std::unique_ptr<ClipComponent>> clips;
    std::optional<Drag> drag;

    ClipComponent* clipAt (juce::Point<int>) const;
    DragMode dragModeAt (const ClipComponent&, juce::Point<int>) const;
    int rowOf (const juce::String& clipId) const;
};

} // namespace papercut
