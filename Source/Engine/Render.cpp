#include "Render.h"

namespace papercut::render
{

juce::BigInteger bitForTrack (te::Track& track)
{
    juce::BigInteger bits;
    const auto all = te::getAllTracks (track.edit);

    if (const int index = all.indexOf (&track); index >= 0)
        bits.setBit (index);

    return bits;
}

juce::Result toWav (te::Edit& edit, const juce::File& destFile, const juce::BigInteger& tracksToDo,
                    te::TimeRange range, bool usePlugins)
{
    if (destFile == juce::File() || destFile.getFullPathName().isEmpty() || destFile.isDirectory())
        return juce::Result::fail ("Offline render needs a WAV file path");

    if (auto r = destFile.getParentDirectory().createDirectory(); r.failed())
        return r;

    destFile.deleteFile();

    auto& dm = edit.engine.getDeviceManager();

    if (dm.getSampleRate() <= 7000.0 || dm.getBlockSize() <= 0)
        return juce::Result::fail ("Offline render needs a sample rate and block size");

    if (tracksToDo.isZero())
        return juce::Result::fail ("Offline render has no tracks");

    if (range.isEmpty())
        range = { te::TimePosition(), edit.getLength() };

    if (range.getLength().inSeconds() <= 0.0)
        return juce::Result::fail ("The Edit has no length to render");

    // A later Edit allocates a playback context once the device has been used.
    // ScopedRenderStatus frees it and stops it being rebuilt for the duration
    // of the render. false: don't reattach afterwards; tests are headless.
    const te::Edit::ScopedRenderStatus renderStatus (edit, false);

    // Not Renderer::renderToFile (Edit&, File): that un-mutes and solo-isolates every track.
    te::Renderer::Parameters params (edit);
    params.destFile = destFile;
    params.audioFormat = edit.engine.getAudioFileFormatManager().getWavFormat();
    params.bitDepth = 24;
    params.sampleRateForAudio = dm.getSampleRate();
    params.blockSizeForAudio = dm.getBlockSize();
    params.time = range;
    params.tracksToDo = tracksToDo;
    params.usePlugins = params.useMasterPlugins = usePlugins;
    // An empty track has no audio nodes. Still write the silent file.
    params.checkNodesForAudio = false;

    auto rendered = te::Renderer::renderToFile ({}, params);

    if (! rendered.existsAsFile() || rendered.getSize() <= 44)
        return juce::Result::fail ("Offline render produced no audio");

    return juce::Result::ok();
}

} // namespace papercut::render
