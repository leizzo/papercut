#include "TrackList.h"
#include "UI/State/ArrangementViewState.h"
#include "UI/Theme/ThemeManager.h"

namespace papercut
{

TrackList::TrackList (ThemeManager& tm, ArrangementViewState& v)
    : themeManager (tm), view (v)
{
}

void TrackList::setTracks (const std::vector<TrackInfo>& newTracks)
{
    tracks = newTracks;
    repaint();
}

void TrackList::paint (juce::Graphics& g)
{
    auto& theme = themeManager.getTheme();
    auto& metrics = themeManager.getMetrics();

    g.fillAll (theme.panel);
    g.setFont (themeManager.getFont());

    for (size_t row = 0; row < tracks.size(); ++row)
    {
        auto& track = tracks[row];
        auto r = juce::Rectangle<int> (0, view.rowToY ((int) row, metrics.trackHeight), getWidth(), metrics.trackHeight)
                     .reduced (metrics.inset);

        g.setColour (track.selected ? theme.trackHeaderSelected : theme.trackHeader);
        g.fillRoundedRectangle (r.toFloat(), theme.cornerRadius);

        g.setColour (track.selected ? theme.text : theme.mutedText);
        g.drawText (track.name, r.reduced (metrics.textPadding), juce::Justification::topLeft, true);
    }
}

void TrackList::mouseDown (const juce::MouseEvent& e)
{
    if (onRowClicked)
        onRowClicked (view.yToRow (e.y, themeManager.getMetrics().trackHeight));
}

} // namespace papercut
