#include "SlotComponent.h"
#include "Commands/SessionCommands.h"
#include "UI/Theme/ThemeManager.h"

namespace papercut
{

SlotComponent::SlotComponent (CommandRegistry& c, ThemeManager& tm, const juce::String& id, TrackKind trackKind, const SlotInfo& info)
    : commands (c), themeManager (tm), trackId (id), kind (trackKind)
{
    setSlot (info);
    setMouseCursor (kind == TrackKind::midi ? juce::MouseCursor::NormalCursor
                                            : juce::MouseCursor::PointingHandCursor);
}

void SlotComponent::setSlot (const SlotInfo& info)
{
    slot = info;
    repaint();
}

void SlotComponent::paint (juce::Graphics& g)
{
    auto& theme = themeManager.getTheme();
    auto& metrics = themeManager.getMetrics();
    const auto bounds = getLocalBounds().toFloat();

    juce::Colour fill = (slot.sceneIndex % 2 == 0) ? theme.laneA : theme.laneB;

    if (slot.playing)
        fill = theme.recording;
    else if (slot.queued)
        fill = theme.accent;
    else if (slot.hasClip)
        fill = theme.clip;

    g.setColour (fill);
    g.fillRoundedRectangle (bounds, theme.cornerRadius);

    if (slot.queued || slot.playing)
    {
        g.setColour (theme.background);
        g.drawRoundedRectangle (bounds.reduced (1.0f), theme.cornerRadius, 1.5f);
    }

    auto text = slot.hasClip ? slot.name : (kind == TrackKind::midi ? juce::String ("MIDI") : juce::String ("Empty"));

    if (slot.playing)
        text << "  playing";
    else if (slot.queued)
        text << "  queued";

    g.setColour (slot.hasClip || slot.queued || slot.playing ? theme.clipText : theme.mutedText);
    g.setFont (themeManager.getFont (0.85f));
    g.drawText (text, getLocalBounds().reduced (metrics.textPadding), juce::Justification::centredLeft, true);
}

void SlotComponent::mouseDown (const juce::MouseEvent& e)
{
    if (kind == TrackKind::midi)
        return;

    const auto args = sessionSlotArgs (trackId, slot.sceneIndex);

    if (e.mods.isPopupMenu())
    {
        if (slot.hasClip)
            commands.invoke ("session.clearSlot", args);

        return;
    }

    commands.invoke (slot.hasClip ? "session.launchSlot" : "session.addSlotClip", args);
}

} // namespace papercut
