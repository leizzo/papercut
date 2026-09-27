#include "TrackLanes.h"
#include "Commands/AppCommands.h"
#include "UI/State/ArrangementViewState.h"
#include "UI/Theme/ThemeManager.h"

namespace papercut
{

TrackLanes::TrackLanes (ApplicationModel& m, CommandRegistry& c, ThemeManager& tm, ArrangementViewState& v)
    : model (m), commands (c), themeManager (tm), view (v)
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
            if (auto existing = clips.find (clip.id); existing != clips.end())
            {
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

    // The model changed under a drag (e.g. a shortcut): drop it if its clip or
    // target row is gone.
    if (drag && (! clips.contains (drag->original.id) || drag->row >= (int) tracks.size()))
        drag.reset();

    layoutClips();
    repaint();
}

void TrackLanes::layoutClips()
{
    auto& metrics = themeManager.getMetrics();
    const auto pixelsPerSecond = view.getPixelsPerSecond();

    for (size_t trackRow = 0; trackRow < tracks.size(); ++trackRow)
    {
        for (auto& trackClip : tracks[trackRow].clips)
        {
            auto it = clips.find (trackClip.id);

            if (it == clips.end())
                continue;

            // A clip being dragged shows where it would land, until the Command commits it.
            const bool dragged = drag && drag->original.id == trackClip.id;
            const auto& clip = dragged ? drag->preview : trackClip;
            const auto row = dragged ? drag->row : (int) trackRow;

            it->second->setClip (clip);

            const auto x = view.timeToX (clip.startSeconds);
            const auto y = view.rowToY (row, metrics.trackHeight);
            const auto width = (float) (clip.lengthSeconds * pixelsPerSecond);
            it->second->setBounds (juce::Rectangle<float> (x, (float) y, width, (float) metrics.trackHeight)
                                       .getSmallestIntegerContainer()
                                       .reduced (0, metrics.inset));
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

//==============================================================================
ClipComponent* TrackLanes::clipAt (juce::Point<int> p) const
{
    // Topmost first, as painted.
    for (int i = getNumChildComponents(); --i >= 0;)
        if (auto* c = dynamic_cast<ClipComponent*> (getChildComponent (i)); c != nullptr && c->getBounds().contains (p))
            return c;

    return nullptr;
}

TrackLanes::DragMode TrackLanes::dragModeAt (const ClipComponent& clip, juce::Point<int> p) const
{
    const auto handle = themeManager.getMetrics().clipResizeHandleWidth;
    const auto x = p.x - clip.getX();

    // Too narrow for two handles and a body: moving matters more than resizing.
    if (clip.getWidth() < 3 * handle)
        return DragMode::move;

    if (x < handle)
        return DragMode::resizeStart;

    if (x >= clip.getWidth() - handle)
        return DragMode::resizeEnd;

    return DragMode::move;
}

int TrackLanes::rowOf (const juce::String& clipId) const
{
    for (size_t row = 0; row < tracks.size(); ++row)
        for (auto& clip : tracks[row].clips)
            if (clip.id == clipId)
                return (int) row;

    return -1;
}

void TrackLanes::mouseMove (const juce::MouseEvent& e)
{
    auto* clip = clipAt (e.getPosition());
    const bool onEdge = clip != nullptr && dragModeAt (*clip, e.getPosition()) != DragMode::move;
    setMouseCursor (onEdge ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor);
}

void TrackLanes::mouseDown (const juce::MouseEvent& e)
{
    drag.reset();

    if (auto* clip = clipAt (e.getPosition()))
    {
        auto info = clip->getClip();
        info.selected = true;
        drag = Drag { dragModeAt (*clip, e.getPosition()), info, info, rowOf (info.id), view.xToTime ((float) e.x) };
        model.selectClip (info.id);
        return;
    }

    if (onRowClicked)
        onRowClicked (view.yToRow (e.y, themeManager.getMetrics().trackHeight));
}

void TrackLanes::mouseDrag (const juce::MouseEvent& e)
{
    if (! drag || tracks.empty())
        return;

    const auto pixelsPerSecond = view.getPixelsPerSecond();
    // Through the view state, so a scroll or zoom mid-drag keeps the clip under the pointer.
    const auto delta = view.xToTime ((float) e.x) - drag->grabSeconds;
    // Never narrower than both grab handles, so a resized clip can still be grabbed.
    const auto minLength = 2 * themeManager.getMetrics().clipResizeHandleWidth / pixelsPerSecond;

    const auto& from = drag->original;
    const auto end = from.startSeconds + from.lengthSeconds;
    const auto sourceStart = from.startSeconds - from.sourceOffsetSeconds;
    auto& to = drag->preview;

    switch (drag->mode)
    {
        case DragMode::move:
            to.startSeconds = std::max (0.0, from.startSeconds + delta);
            drag->row = juce::jlimit (0, (int) tracks.size() - 1, view.yToRow (e.y, themeManager.getMetrics().trackHeight));
            break;

        case DragMode::resizeStart:
        {
            const auto earliest = std::max (0.0, sourceStart);
            const auto latest = std::max (earliest, end - minLength);
            to.startSeconds = juce::jlimit (earliest, latest, from.startSeconds + delta);
            to.lengthSeconds = end - to.startSeconds;
            to.sourceOffsetSeconds = from.sourceOffsetSeconds + (to.startSeconds - from.startSeconds);
            break;
        }

        case DragMode::resizeEnd:
        {
            const auto earliest = from.startSeconds + minLength;
            const auto latest = std::max (earliest, sourceStart + from.sourceLengthSeconds);
            to.lengthSeconds = juce::jlimit (earliest, latest, end + delta) - from.startSeconds;
            break;
        }
    }

    layoutClips();
}

void TrackLanes::mouseUp (const juce::MouseEvent& e)
{
    if (! drag)
        return;

    const auto released = *drag;
    drag.reset();

    if (e.mouseWasDraggedSinceMouseDown())
    {
        const auto& to = released.preview;

        if (released.mode == DragMode::move)
            commands.invoke ("clip.move", clipMoveArgs (to.id, to.startSeconds, tracks[(size_t) released.row].id));
        else
            commands.invoke ("clip.resize", clipResizeArgs (to.id, to.startSeconds, to.startSeconds + to.lengthSeconds));
    }

    // Settle on the committed position now (or snap back if the Command changed
    // nothing) rather than showing the stale one until the async model update.
    setTracks (model.getTracks());
}

} // namespace papercut
