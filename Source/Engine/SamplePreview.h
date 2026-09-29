#pragma once

#include <juce_core/juce_core.h>
#include <memory>

namespace resamper
{

class EngineManager;

/** Auditions an audio file from the Browser (PRD §6.2: hover or `→`). It plays
    through the engine's audio device alongside the Edit, outside the Edit's
    mixer. Resamper has no separate Cue output yet, so "Cue" is the main output.
    One file at a time; playing another replaces it. */
class SamplePreview
{
public:
    explicit SamplePreview (EngineManager&);
    ~SamplePreview();

    /** Starts the file from its beginning. Returns false, stopping any preview,
        if it isn't a readable audio file. */
    bool play (const juce::File&);
    void stop();

    bool isPlaying() const;

    /** The file playing, or none. */
    juce::File getFile() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;

    JUCE_DECLARE_NON_COPYABLE (SamplePreview)
};

} // namespace resamper
