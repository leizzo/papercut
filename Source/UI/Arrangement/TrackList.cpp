#include "TrackList.h"
#include "UI/State/ArrangementViewState.h"
#include "UI/Theme/ThemeManager.h"

namespace resamper
{

TrackList::TrackList (CommandRegistry& c, ThemeManager& tm, ArrangementViewState& v)
    : commands (c), themeManager (tm), view (v)
{
}

void TrackList::setTracks (const std::vector<TrackInfo>& newTracks, const juce::StringArray& audioInputs,
                           const juce::StringArray& midiInputs)
{
    tracks = newTracks;
    currentAudioInputs = audioInputs;
    currentMidiInputs = midiInputs;
    std::map<juce::String, std::unique_ptr<TrackHeader>> kept;

    for (auto& track : tracks)
    {
        const auto& inputs = track.kind == TrackKind::midi ? midiInputs : audioInputs;

        if (auto existing = headers.find (track.id); existing != headers.end())
        {
            kept[track.id] = std::move (existing->second);
        }
        else
        {
            auto header = std::make_unique<TrackHeader> (commands, themeManager, track, inputs);
            header->onToggleAutomation = [this, id = track.id]
            {
                view.setAutomationShown (id, ! view.isAutomationShown (id));
                setTracks (tracks, currentAudioInputs, currentMidiInputs);
            };
            addAndMakeVisible (*header);
            kept[track.id] = std::move (header);
        }

        kept[track.id]->setTrack (track, inputs, view.isAutomationShown (track.id));
    }

    headers = std::move (kept);
    layoutHeaders();
}

void TrackList::layoutHeaders()
{
    auto& metrics = themeManager.getMetrics();

    for (size_t row = 0; row < tracks.size(); ++row)
        headers[tracks[row].id]->setBounds (juce::Rectangle<int> (0, view.rowToY ((int) row, view.getLaneHeight (metrics.trackHeight)),
                                                                  getWidth(), view.getLaneHeight (metrics.trackHeight)));
}

void TrackList::applyTheme()
{
    for (auto& [id, header] : headers)
        header->applyTheme();

    repaint();
}

void TrackList::paint (juce::Graphics& g)
{
    g.fillAll (themeManager.getTheme().bgPanel);
}

void TrackList::mouseDown (const juce::MouseEvent& e)
{
    if (onRowClicked)
        onRowClicked (view.yToRow (e.y, view.getLaneHeight (themeManager.getMetrics().trackHeight)), e.mods);
}

} // namespace resamper
