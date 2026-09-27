#include "SessionView.h"
#include "Commands/SessionCommands.h"
#include "Engine/Session.h"
#include "SceneHeader.h"
#include "SlotComponent.h"

#include <algorithm>
#include <map>

namespace papercut
{

namespace
{
    SlotInfo slotFor (const std::vector<SlotInfo>& slots, int sceneIndex)
    {
        for (auto& slot : slots)
            if (slot.sceneIndex == sceneIndex)
                return slot;

        SlotInfo empty;
        empty.sceneIndex = sceneIndex;
        return empty;
    }
}

SessionView::SessionView (ApplicationModel& m, Session& s, CommandRegistry& c, ThemeManager& tm)
    : model (m), session (s), commands (c), themeManager (tm)
{
    setComponentID (componentId);

    addSceneButton.setTooltip ("Add scene");
    stopButton.setTooltip ("Stop all slots");
    recordButton.setTooltip ("Record playing slots into the Arrangement");

    addSceneButton.onClick = [this]
    {
        commands.invoke ("session.setSceneCount", sessionSceneCountArgs ((int) session.getScenes().size() + 1));
    };
    stopButton.onClick = [this] { commands.invoke ("session.stopAll"); };
    recordButton.onClick = [this] { commands.invoke ("session.recordToArrangement"); };

    for (auto* child : { &addSceneButton, &stopButton, &recordButton })
        addAndMakeVisible (child);

    model.addListener (this);
    themeManager.addListener (this);
    applyTheme();
    refresh();
    startTimerHz (15);
}

SessionView::~SessionView()
{
    stopTimer();
    themeManager.removeListener (this);
    model.removeListener (this);
}

void SessionView::refresh()
{
    const auto tracks = model.getTracks();
    const auto scenes = session.getScenes();

    if (! shapeMatches (tracks, scenes))
        rebuild (tracks, scenes);

    updateContents (tracks, scenes);
    resized();
}

void SessionView::rebuild (const std::vector<TrackInfo>& tracks, const std::vector<SceneInfo>& scenes)
{
    headers.clear();
    cells.clear();
    columnTracks.clear();

    for (auto& scene : scenes)
    {
        auto header = std::make_unique<SceneHeader> (commands, themeManager, scene);
        addAndMakeVisible (*header);
        headers.push_back (std::move (header));
    }

    for (int column = 0; column < (int) tracks.size(); ++column)
    {
        const auto& track = tracks[(size_t) column];
        columnTracks.push_back (track.id);
        const auto slots = session.getSlots (track.id);

        for (auto& scene : scenes)
        {
            Cell cell;
            cell.trackId = track.id;
            cell.column = column;
            cell.sceneIndex = scene.index;
            cell.component = std::make_unique<SlotComponent> (commands, themeManager, track.id, track.kind,
                                                              slotFor (slots, scene.index));
            addAndMakeVisible (*cell.component);
            cells.push_back (std::move (cell));
        }
    }
}

void SessionView::updateContents (const std::vector<TrackInfo>& tracks, const std::vector<SceneInfo>& scenes)
{
    for (size_t i = 0; i < headers.size() && i < scenes.size(); ++i)
        headers[i]->setScene (scenes[i]);

    std::map<juce::String, std::vector<SlotInfo>> slotsByTrack;

    for (auto& track : tracks)
        slotsByTrack[track.id] = session.getSlots (track.id);

    for (auto& cell : cells)
        cell.component->setSlot (slotFor (slotsByTrack[cell.trackId], cell.sceneIndex));
}

bool SessionView::shapeMatches (const std::vector<TrackInfo>& tracks, const std::vector<SceneInfo>& scenes) const
{
    if (headers.size() != scenes.size() || columnTracks.size() != tracks.size())
        return false;

    for (size_t i = 0; i < tracks.size(); ++i)
        if (columnTracks[i] != tracks[i].id)
            return false;

    return true;
}

void SessionView::applyTheme()
{
    auto& theme = themeManager.getTheme();

    for (auto* button : { &addSceneButton, &stopButton, &recordButton })
    {
        button->setColour (juce::TextButton::textColourOffId, theme.text);
        button->setColour (juce::TextButton::buttonColourId, theme.trackHeader);
    }

    addSceneButton.setColour (juce::TextButton::buttonColourId, theme.accent);
    addSceneButton.setColour (juce::TextButton::textColourOffId, theme.background);
    recordButton.setColour (juce::TextButton::buttonColourId, theme.recording);
    recordButton.setColour (juce::TextButton::textColourOffId, theme.background);

    for (auto& header : headers)
        header->applyTheme();

    repaint();
}

void SessionView::paint (juce::Graphics& g)
{
    auto& theme = themeManager.getTheme();
    g.fillAll (theme.background);

    if (headers.empty())
    {
        g.setColour (theme.mutedText);
        g.setFont (themeManager.getFont());
        g.drawText ("No scenes", getLocalBounds(), juce::Justification::centred, false);
    }
}

void SessionView::resized()
{
    auto& metrics = themeManager.getMetrics();
    auto r = getLocalBounds();
    auto bar = r.removeFromTop (metrics.trackControlHeight + metrics.inset * 2).reduced (metrics.inset, metrics.inset);
    auto buttons = bar.removeFromLeft (metrics.trackHeaderWidth);
    const auto button = std::max (metrics.trackButtonWidth, metrics.trackControlHeight);

    addSceneButton.setBounds (buttons.removeFromLeft (button));
    buttons.removeFromLeft (metrics.inset);
    stopButton.setBounds (buttons.removeFromLeft (button * 2));
    buttons.removeFromLeft (metrics.inset);
    recordButton.setBounds (buttons.removeFromLeft (button * 2));

    const auto rowHeight = metrics.trackHeight;
    const auto columns = (int) columnTracks.size();
    const auto slotWidth = columns > 0 ? std::max (1, (r.getWidth() - metrics.trackHeaderWidth) / columns) : 0;

    for (int row = 0; row < (int) headers.size(); ++row)
        headers[(size_t) row]->setBounds (juce::Rectangle<int> (r.getX(), r.getY() + row * rowHeight,
                                                                metrics.trackHeaderWidth, rowHeight)
                                              .reduced (metrics.inset));

    for (auto& cell : cells)
        cell.component->setBounds (juce::Rectangle<int> (r.getX() + metrics.trackHeaderWidth + cell.column * slotWidth,
                                                         r.getY() + cell.sceneIndex * rowHeight,
                                                         slotWidth, rowHeight)
                                       .reduced (metrics.inset));
}

void SessionView::timerCallback()
{
    const auto tracks = model.getTracks();
    const auto scenes = session.getScenes();

    if (! shapeMatches (tracks, scenes))
        refresh();
    else
        updateContents (tracks, scenes);
}

} // namespace papercut
