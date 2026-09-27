#pragma once

#include "Engine/ApplicationModel.h"

namespace papercut
{

class ArrangementViewState;
class ThemeManager;

/** The column of track headers beside the lanes. */
class TrackList : public juce::Component
{
public:
    TrackList (ThemeManager&, ArrangementViewState&);

    void setTracks (const std::vector<TrackInfo>&);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

    /** Called with the row index under a click (may be out of range). */
    std::function<void (int row)> onRowClicked;

private:
    ThemeManager& themeManager;
    ArrangementViewState& view;
    std::vector<TrackInfo> tracks;
};

} // namespace papercut
