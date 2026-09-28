#include "TrackLanes.h"
#include "Commands/AppCommands.h"
#include "UI/PianoRoll/BeatGrid.h"
#include "UI/State/ArrangementViewState.h"
#include "UI/Theme/ThemeManager.h"

namespace papercut
{

TrackLanes::TrackLanes (ApplicationModel& m, CommandRegistry& c, ThemeManager& tm, ArrangementViewState& v)
    : model (m), commands (c), themeManager (tm), view (v)
{
    startTimerHz (30);
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
            const auto& palette = themeManager.getTheme().trackPalette;
            it->second->setTrackLook (clip.colourIndex >= 0 ? palette[(size_t) juce::jmin (clip.colourIndex, (int) palette.size() - 1)]
                                                            : trackColour (tracks[trackRow]),
                                      tracks[trackRow].muted);

            const auto x = view.timeToX (clip.startSeconds);
            const auto y = view.rowToY (row, metrics.trackHeight);
            const auto width = (float) (clip.lengthSeconds * pixelsPerSecond);
            it->second->setBounds (juce::Rectangle<float> (x, (float) y, width, (float) metrics.trackHeight)
                                       .getSmallestIntegerContainer()
                                       .reduced (0, metrics.spaceSm));
        }
    }
}

juce::Colour TrackLanes::trackColour (const TrackInfo& track) const
{
    auto& palette = themeManager.getTheme().trackPalette;
    return palette[(size_t) juce::jlimit (0, (int) palette.size() - 1, track.colourIndex)];
}

void TrackLanes::paint (juce::Graphics& g)
{
    auto& theme = themeManager.getTheme();
    const auto rowHeight = themeManager.getMetrics().trackHeight;

    g.fillAll (theme.bgDeep);

    // Grid lines where the ruler numbers its bars.
    g.setColour (theme.borderSoft);

    for (auto& line : barLines (model, view.xToTime (0.0f), view.xToTime ((float) getWidth()), barsPerGridLine (model, view)))
        g.fillRect (juce::roundToInt (view.timeToX (line.seconds)), 0, 1, getHeight());

    for (size_t row = 0; row < tracks.size(); ++row)
        g.fillRect (0, view.rowToY ((int) row, rowHeight) + rowHeight - 1, getWidth(), 1);
}

void TrackLanes::paintOverChildren (juce::Graphics& g)
{
    auto& theme = themeManager.getTheme();
    auto& metrics = themeManager.getMetrics();

    for (auto& recording : recordings)
    {
        const auto row = rowOfTrack (recording.trackId);

        if (row < 0)
            continue;

        const auto x = view.timeToX (recording.startSeconds);
        const auto width = (float) (recording.lengthSeconds * view.getPixelsPerSecond());
        const auto area = juce::Rectangle<float> (x, (float) view.rowToY (row, metrics.trackHeight), width, (float) metrics.trackHeight)
                              .getSmallestIntegerContainer()
                              .reduced (0, metrics.spaceSm);

        g.setColour (theme.recording);
        g.fillRoundedRectangle (area.toFloat(), 5.0f);

        if (auto waveform = recordingWaveforms.find (recording.trackId); waveform != recordingWaveforms.end())
        {
            g.setColour (theme.waveform);
            waveform->second->draw (g, area.withTrimmedTop (metrics.clipHeaderHeight), 0.0, recording.lengthSeconds);
        }
    }
}

void TrackLanes::timerCallback()
{
    auto now = model.getRecordings();

    if (now.empty() && recordings.empty())
        return;

    std::map<juce::String, std::unique_ptr<ClipWaveform>> kept;

    for (auto& recording : now)
    {
        if (auto existing = recordingWaveforms.find (recording.trackId); existing != recordingWaveforms.end())
            kept[recording.trackId] = std::move (existing->second);
        else if (auto waveform = model.createRecordingWaveform (recording.trackId))
            kept[recording.trackId] = std::move (waveform);
    }

    recordings = std::move (now);
    recordingWaveforms = std::move (kept);
    repaint();
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

    if (x >= clip.getWidth() - handle && p.y - clip.getY() < themeManager.getMetrics().clipHeaderHeight)
        return DragMode::loopExtend;

    if (x < handle)
        return DragMode::resizeStart;

    if (x >= clip.getWidth() - handle)
        return DragMode::resizeEnd;

    return DragMode::move;
}

int TrackLanes::rowOfTrack (const juce::String& trackId) const
{
    for (size_t row = 0; row < tracks.size(); ++row)
        if (tracks[row].id == trackId)
            return (int) row;

    return -1;
}

double TrackLanes::snap (double seconds, bool bypass) const
{
    if (bypass)
        return seconds;

    return model.beatsToSeconds (std::round (model.secondsToBeats (seconds)));
}

void TrackLanes::showClipMenu (const ClipInfo& clip)
{
    auto& theme = themeManager.getTheme();
    const auto cmd = juce::ModifierKeys::commandModifier;

    auto item = [] (const juce::String& text, juce::KeyPress key, std::function<void()> action, bool enabled = true)
    {
        auto i = juce::PopupMenu::Item (text).setEnabled (enabled).setAction (std::move (action));
        i.shortcutKeyDescription = key.isValid() ? key.getTextDescriptionWithIcons() : juce::String();
        return i;
    };

    juce::PopupMenu colours;
    colours.addItem (juce::PopupMenu::Item ("Track Colour").setTicked (clip.colourIndex < 0)
                         .setAction ([this, id = clip.id] { commands.invoke ("clip.setColour", clipColourArgs (id, -1)); }));

    for (int i = 0; i < ApplicationModel::trackPaletteSize; ++i)
        colours.addItem (juce::PopupMenu::Item ("Colour " + juce::String (i + 1)).setColour (theme.trackPalette[(size_t) i])
                             .setTicked (clip.colourIndex == i)
                             .setAction ([this, id = clip.id, i] { commands.invoke ("clip.setColour", clipColourArgs (id, i)); }));

    juce::PopupMenu menu;
    menu.addItem (item ("Rename", {}, [this, clip] { startRename (clip); }));
    menu.addSubMenu ("Colour", colours);
    menu.addSeparator();
    menu.addItem (item ("Duplicate", juce::KeyPress ('d', cmd, 0), [this] { commands.invoke ("clip.duplicate"); }));
    menu.addItem (item ("Split", juce::KeyPress ('e', cmd, 0), [this] { commands.invoke ("clip.split"); },
                        model.canSplitClip (clip.id, model.getTransportPositionSeconds())));
    menu.addItem (item ("Consolidate", juce::KeyPress ('j', cmd, 0), [this] { commands.invoke ("clip.consolidate"); },
                        model.getSelectedClipIds().size() > 1));

    if (clip.kind == TrackKind::audio)
        menu.addItem (item (clip.reversed ? "Play Forwards" : "Reverse", {},
                            [this, id = clip.id] { commands.invoke ("clip.reverse", clipArgs (id)); }));
    else
        menu.addItem (item ("Quantize", juce::KeyPress ('q'),
                            [this, id = clip.id] { commands.invoke ("note.quantize", noteQuantizeArgs (id, "1/16")); }));

    if (clip.numTakes > 0)
    {
        juce::PopupMenu takes;

        for (int take = 0; take < clip.numTakes; ++take)
            takes.addItem ("Take " + juce::String (take + 1), true, take == clip.currentTake,
                           [this, id = clip.id, take] { commands.invoke ("clip.setTake", clipTakeArgs (id, take)); });

        menu.addSubMenu ("Takes", takes);
    }

    menu.addSeparator();
    menu.addItem (item ("Delete", juce::KeyPress (juce::KeyPress::deleteKey), [this] { commands.invoke ("clip.delete"); }));
    menu.showMenuAsync (juce::PopupMenu::Options().withMousePosition());
}

void TrackLanes::startRename (const ClipInfo& clip)
{
    auto it = clips.find (clip.id);

    if (it == clips.end())
        return;

    renameEditor = std::make_unique<juce::TextEditor>();
    renameEditor->setFont (themeManager.font (TypeStyle { 11.0f, false, 600 }));
    renameEditor->setText (clip.name, false);
    renameEditor->selectAll();
    renameEditor->setBounds (it->second->getBounds().withHeight (themeManager.getMetrics().clipHeaderHeight + 4)
                                 .withWidth (juce::jmax (120, it->second->getWidth())));

    auto finish = [this, id = clip.id] (bool commit)
    {
        if (renameEditor == nullptr || ! renameEditor->isVisible())
            return;

        const auto text = renameEditor->getText();
        renameEditor->setVisible (false);

        if (commit)
            commands.invoke ("clip.rename", clipRenameArgs (id, text));

        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<TrackLanes> (this)]
        {
            if (safe != nullptr)
                safe->renameEditor.reset();
        });
    };

    renameEditor->onReturnKey = [finish] { finish (true); };
    renameEditor->onEscapeKey = [finish] { finish (false); };
    renameEditor->onFocusLost = [finish] { finish (true); };
    addAndMakeVisible (*renameEditor);
    renameEditor->grabKeyboardFocus();
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
    const auto mode = clip != nullptr ? dragModeAt (*clip, e.getPosition()) : DragMode::move;
    setMouseCursor (mode == DragMode::loopExtend ? juce::MouseCursor::TopRightCornerResizeCursor
                  : mode != DragMode::move       ? juce::MouseCursor::LeftRightResizeCursor
                                                 : juce::MouseCursor::NormalCursor);
}

void TrackLanes::mouseDown (const juce::MouseEvent& e)
{
    drag.reset();

    if (auto* clip = clipAt (e.getPosition()))
    {
        auto info = clip->getClip();

        if (e.mods.isPopupMenu())
        {
            // A right-click inside the selection keeps it, so Consolidate can act on all of it.
            if (! info.selected)
                model.selectClip (info.id);

            showClipMenu (info);
            return;
        }

        if (e.getNumberOfClicks() == 2)
        {
            model.selectClip (info.id);
            auto& open = info.kind == TrackKind::midi ? onMidiClipOpened : onAudioClipOpened;

            if (open)
                open (info.id);

            return;
        }

        using Mode = ApplicationModel::SelectionMode;

        if (e.mods.isShiftDown() || e.mods.isCommandDown())
        {
            // Extending or toggling the selection is a click, not the start of a drag.
            model.selectClip (info.id, e.mods.isShiftDown() ? Mode::add : Mode::toggle);
            return;
        }

        info.selected = true;
        drag = Drag { dragModeAt (*clip, e.getPosition()), info, info, rowOf (info.id), view.xToTime ((float) e.x), false };

        if (! clip->getClip().selected)
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
        {
            drag->copy = e.mods.isAltDown();
            to.startSeconds = std::max (0.0, snap (from.startSeconds + delta, e.mods.isCommandDown()));
            const auto row = juce::jlimit (0, (int) tracks.size() - 1, view.yToRow (e.y, themeManager.getMetrics().trackHeight));

            // A clip only lands on a track of its own kind; the Command refuses the other.
            if (tracks[(size_t) row].kind == from.kind)
                drag->row = row;

            break;
        }

        case DragMode::resizeStart:
        {
            const auto earliest = std::max (0.0, sourceStart);
            const auto latest = std::max (earliest, end - minLength);
            to.startSeconds = juce::jlimit (earliest, latest, snap (from.startSeconds + delta, e.mods.isCommandDown()));
            to.lengthSeconds = end - to.startSeconds;
            to.sourceOffsetSeconds = from.sourceOffsetSeconds + (to.startSeconds - from.startSeconds);
            break;
        }

        case DragMode::resizeEnd:
        {
            const auto earliest = from.startSeconds + minLength;
            const auto latest = std::max (earliest, sourceStart + from.sourceLengthSeconds);
            to.lengthSeconds = juce::jlimit (earliest, latest, snap (end + delta, e.mods.isCommandDown())) - from.startSeconds;
            break;
        }

        case DragMode::loopExtend:
        {
            // Past the source's end is fine: the content repeats.
            to.lengthSeconds = std::max (from.startSeconds + minLength, snap (end + delta, e.mods.isCommandDown())) - from.startSeconds;
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
            commands.invoke (released.copy ? "clip.copy" : "clip.move",
                             clipMoveArgs (to.id, to.startSeconds, tracks[(size_t) released.row].id));
        else if (released.mode == DragMode::loopExtend)
            commands.invoke ("clip.loopExtend", clipResizeArgs (to.id, to.startSeconds, to.startSeconds + to.lengthSeconds));
        else
            commands.invoke ("clip.resize", clipResizeArgs (to.id, to.startSeconds, to.startSeconds + to.lengthSeconds));
    }

    // Settle on the committed position now (or snap back if the Command changed
    // nothing) rather than showing the stale one until the async model update.
    setTracks (model.getTracks());
}

} // namespace papercut
