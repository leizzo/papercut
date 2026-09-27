#include "TrackList.h"
#include "UI/State/ArrangementViewState.h"
#include "UI/Theme/ThemeManager.h"

namespace papercut
{

TrackList::TrackList (CommandRegistry& c, ThemeManager& tm, ArrangementViewState& v)
    : commands (c), themeManager (tm), view (v)
{
}

void TrackList::setTracks (const std::vector<TrackInfo>& newTracks)
{
    tracks = newTracks;
    std::map<juce::String, std::unique_ptr<TrackHeader>> kept;

    for (auto& track : tracks)
    {
        if (auto existing = headers.find (track.id); existing != headers.end())
        {
            existing->second->setTrack (track);
            kept[track.id] = std::move (existing->second);
        }
        else
        {
            auto header = std::make_unique<TrackHeader> (commands, themeManager, track);
            addAndMakeVisible (*header);
            kept[track.id] = std::move (header);
        }
    }

    headers = std::move (kept);
    layoutHeaders();
}

void TrackList::layoutHeaders()
{
    auto& metrics = themeManager.getMetrics();

    for (size_t row = 0; row < tracks.size(); ++row)
        headers[tracks[row].id]->setBounds (juce::Rectangle<int> (0, view.rowToY ((int) row, metrics.trackHeight),
                                                                  getWidth(), metrics.trackHeight)
                                                .reduced (metrics.inset));
}

void TrackList::applyTheme()
{
    for (auto& [id, header] : headers)
        header->applyTheme();

    repaint();
}

void TrackList::paint (juce::Graphics& g)
{
    g.fillAll (themeManager.getTheme().panel);
}

void TrackList::mouseDown (const juce::MouseEvent& e)
{
    if (onRowClicked)
        onRowClicked (view.yToRow (e.y, themeManager.getMetrics().trackHeight));
}

} // namespace papercut
