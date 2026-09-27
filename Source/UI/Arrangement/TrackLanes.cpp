#include "TrackLanes.h"
#include "UI/State/ArrangementViewState.h"
#include "UI/Theme/ThemeManager.h"

namespace papercut
{

TrackLanes::TrackLanes (ApplicationModel& m, ThemeManager& tm, ArrangementViewState& v)
    : model (m), themeManager (tm), view (v)
{
}

void TrackLanes::setTracks (const std::vector<TrackInfo>& newTracks)
{
    tracks = newTracks;
    std::map<juce::String, std::unique_ptr<ClipComponent>> kept;

    for (auto& track : tracks)
    {
        for (auto& clip : track.clips)
        {
            auto existing = clips.find (clip.id);

            if (existing != clips.end())
            {
                existing->second->setClip (clip);
                kept[clip.id] = std::move (existing->second);
            }
            else
            {
                auto c = std::make_unique<ClipComponent> (model, themeManager, clip);
                addAndMakeVisible (*c);
                kept[clip.id] = std::move (c);
            }
        }
    }

    clips = std::move (kept);
    layoutClips();
    repaint();
}

void TrackLanes::layoutClips()
{
    auto& metrics = themeManager.getMetrics();
    const auto pixelsPerSecond = view.getPixelsPerSecond();

    for (size_t row = 0; row < tracks.size(); ++row)
    {
        const auto y = view.rowToY ((int) row, metrics.trackHeight);

        for (auto& clip : tracks[row].clips)
        {
            if (auto it = clips.find (clip.id); it != clips.end())
            {
                const auto x = view.timeToX (clip.startSeconds);
                const auto width = (float) (clip.lengthSeconds * pixelsPerSecond);
                it->second->setBounds (juce::Rectangle<float> (x, (float) y, width, (float) metrics.trackHeight)
                                           .getSmallestIntegerContainer()
                                           .reduced (0, metrics.inset));
            }
        }
    }
}

void TrackLanes::paint (juce::Graphics& g)
{
    auto& theme = themeManager.getTheme();
    const auto rowHeight = themeManager.getMetrics().trackHeight;

    g.fillAll (theme.background);

    for (size_t row = 0; row < tracks.size(); ++row)
    {
        g.setColour (row % 2 == 0 ? theme.laneA : theme.laneB);
        g.fillRect (0, view.rowToY ((int) row, rowHeight), getWidth(), rowHeight);
    }
}

void TrackLanes::mouseDown (const juce::MouseEvent& e)
{
    if (onRowClicked)
        onRowClicked (view.yToRow (e.y, themeManager.getMetrics().trackHeight));
}

} // namespace papercut
