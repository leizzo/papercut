#include "NoteComponent.h"
#include "UI/Theme/ThemeManager.h"

namespace resamper
{

NoteComponent::NoteComponent (ThemeManager& tm, const MidiNoteInfo& info)
    : themeManager (tm)
{
    setInterceptsMouseClicks (false, false);
    setNote (info);
}

void NoteComponent::setNote (const MidiNoteInfo& info)
{
    note = info;
    repaint();
}

void NoteComponent::paint (juce::Graphics& g)
{
    auto& theme = themeManager.getTheme();
    auto bounds = getLocalBounds().toFloat();
    const auto radius = std::min (theme.cornerRadius, bounds.getHeight() * 0.5f);

    g.setColour (note.selected ? theme.noteSelected : theme.midiNote);
    g.fillRoundedRectangle (bounds, radius);
}

} // namespace resamper
