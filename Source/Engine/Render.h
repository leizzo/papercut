#pragma once

#include <tracktion_engine/tracktion_engine.h>

namespace resamper
{

namespace te = tracktion;

/** Offline rendering shared by the engine facades (bounce, export, consolidate).
    Engine-internal: it takes Tracktion types. */
namespace render
{
    /** Bit for one track in Renderer::Parameters::tracksToDo.

        te::toBitSet sets every track whenever the array is non-empty, so a
        one-track bounce cannot use it. The renderer indexes getAllTracks. */
    juce::BigInteger bitForTrack (te::Track&);

    /** Renders the tracks to a 24-bit WAV, as they play (honouring mute and
        solo). range: the whole Edit when empty. usePlugins false renders the
        clips' own audio, without the tracks' plug-ins or faders. */
    juce::Result toWav (te::Edit&, const juce::File& destFile, const juce::BigInteger& tracksToDo,
                        te::TimeRange range = {}, bool usePlugins = true);
}

} // namespace resamper
