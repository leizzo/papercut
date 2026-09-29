#pragma once

#include "Engine/ApplicationModel.h"

namespace resamper
{

class ThemeManager;

/** One note in the Piano Roll. Its bounds are set by NoteGrid. */
class NoteComponent : public juce::Component
{
public:
    NoteComponent (ThemeManager&, const MidiNoteInfo&);

    const MidiNoteInfo& getNote() const noexcept   { return note; }
    void setNote (const MidiNoteInfo&);

    void paint (juce::Graphics&) override;

private:
    ThemeManager& themeManager;
    MidiNoteInfo note;
};

} // namespace resamper
