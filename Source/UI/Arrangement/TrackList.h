#pragma once

#include "TrackHeader.h"

#include <map>

namespace resamper
{

class ArrangementViewState;
class CommandRegistry;
class ThemeManager;

/** The column of track headers beside the lanes. Headers are kept by track ID
    across model updates, so a fader drag survives the updates it causes. */
class TrackList : public juce::Component
{
public:
    TrackList (CommandRegistry&, ThemeManager&, ArrangementViewState&);

    /** The inputs a track can record from: audio inputs for an audio track, MIDI inputs for a MIDI track. */
    void setTracks (const std::vector<TrackInfo>&, const juce::StringArray& audioInputs, const juce::StringArray& midiInputs);

    /** Re-positions every header from the view state (after scroll). */
    void layoutHeaders();

    /** Re-applies the Theme to every header. */
    void applyTheme();

    void paint (juce::Graphics&) override;
    void resized() override   { layoutHeaders(); }
    void mouseDown (const juce::MouseEvent&) override;

    /** Called with the row index under a click (may be out of range). */
    std::function<void (int row, juce::ModifierKeys)> onRowClicked;

private:
    CommandRegistry& commands;
    ThemeManager& themeManager;
    ArrangementViewState& view;
    std::vector<TrackInfo> tracks;
    juce::StringArray currentAudioInputs, currentMidiInputs;
    std::map<juce::String, std::unique_ptr<TrackHeader>> headers;
};

} // namespace resamper
