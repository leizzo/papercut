#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace papercut
{

class LayoutSource;

/** Visual style only: colours, corner radii, fonts (ADR-0005). Never geometry. */
struct Theme
{
    juce::Colour background, panel, text, mutedText, accent,
                 laneA, laneB, trackHeader, trackHeaderSelected, mute, solo, armed,
                 recording,   ///< a recording in progress in its lane
                 loop,        ///< the loop range on the ruler
                 clip, clipSelected, clipText, waveform,
                 midiClip, midiClipSelected, midiNote,
                 pianoWhite, pianoBlack, noteSelected, gridLine, velocity,
                 ruler, playhead, error;
    float cornerRadius = 0;
    float fontSize = 0;
};

/** UI geometry (ADR-0005). Components read sizes from here, never hard-code them. */
struct LayoutMetrics
{
    int transportHeight = 0;
    int statusBarHeight = 0;
    int timelineHeight = 0;
    int trackHeight = 0;
    int trackHeaderWidth = 0;
    int clipHeaderHeight = 0;
    int inset = 0;          ///< gap between adjacent boxes (track rows, clips)
    int textPadding = 0;    ///< space between a box edge and its text
    int playheadWidth = 0;
    int clipResizeHandleWidth = 0;   ///< grab zone at each clip edge
    int trackControlHeight = 0;      ///< one row of track header controls
    int trackButtonWidth = 0;        ///< mute and solo buttons
    int midiNoteHeight = 0;          ///< compact note preview inside a MIDI clip
    int pianoKeyWidth = 0;           ///< piano roll keyboard
    int pianoKeyHeight = 0;          ///< one semitone row in the piano roll
    int pianoBlackKeyWidth = 0;      ///< black keys, shorter than a white key
    int pianoScrollMargin = 0;       ///< key rows kept above middle C on open
    int velocityLaneHeight = 0;      ///< piano roll velocity lane
    int gridEighthPixels = 0;        ///< pixels per beat before the grid shows 1/8
    int gridSixteenthPixels = 0;     ///< pixels per beat before the grid shows 1/16
};

/** Loads Theme and Layout Metrics from one JSON file under separate keys
    ("colors" and "style" for the Theme, "metrics" for geometry — ADR-0005) and
    applies the Theme to the app's LookAndFeel.

    Layout Metrics are read once at load(). reloadTheme() re-styles only: it
    never changes geometry, so nothing is re-laid-out or recreated.
*/
class ThemeManager
{
public:
    struct Listener
    {
        virtual ~Listener() = default;
        virtual void themeChanged() = 0;   ///< repaint
    };

    ThemeManager (const LayoutSource&, juce::String themeFile);
    ~ThemeManager();

    /** Reads Theme and Layout Metrics. Called once at startup. */
    juce::Result load();

    /** Re-reads the Theme only and notifies listeners. On failure, the current
        Theme is kept. Geometry is left as load() set it. */
    juce::Result reloadTheme();

    /** Stores themeFile and calls reloadTheme(). Colours change; Layout Metrics
        stay at the values from the last load(). */
    juce::Result useTheme (juce::String themeFile);

    const Theme& getTheme() const noexcept                  { return theme; }
    const LayoutMetrics& getMetrics() const noexcept        { return metrics; }
    juce::LookAndFeel& getLookAndFeel() noexcept            { return *lookAndFeel; }
    juce::Font getFont (float scale = 1.0f) const;

    void addListener (Listener* l)      { listeners.add (l); }
    void removeListener (Listener* l)   { listeners.remove (l); }

    /** Parses a theme file. Every key is required: a missing one fails loudly
        rather than falling back to a hard-coded value. */
    static juce::Result parse (const juce::String& json, Theme&, LayoutMetrics&);

private:
    const LayoutSource& source;
    juce::String themeFile;
    Theme theme;
    LayoutMetrics metrics;
    std::unique_ptr<juce::LookAndFeel_V4> lookAndFeel;

    juce::Result read (Theme&, LayoutMetrics&) const;
    juce::ListenerList<Listener> listeners;
};

} // namespace papercut
