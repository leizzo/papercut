#include "NativeDevicePlugins.h"

#include <utility>

namespace te = tracktion;

namespace resamper
{

namespace
{
    /** A typed value as the texts below write it: "1.5 kHz" is 1500, "50%" is 0.5,
        "4:1" is 4, a minus sign (−) counts. Anything else reads its leading number. */
    float parseValue (const juce::String& text)
    {
        const auto t = text.trim().replace (juce::String (juce::CharPointer_UTF8 ("\xe2\x88\x92")), "-");
        auto value = t.getFloatValue();

        if (t.containsIgnoreCase ("khz"))
            value *= 1000.0f;
        else if (t.endsWithChar ('%'))
            value /= 100.0f;

        return value;
    }

    te::ParameterWithStateValue makeParameter (te::Plugin& plugin, const juce::String& id, const juce::String& name,
                                               float defaultValue, juce::NormalisableRange<float> range,
                                               std::function<juce::String (float)> toText)
    {
        return { plugin, id, juce::Identifier (id), name, defaultValue, std::move (range), std::move (toText),
                 parseValue };
    }

    /** A range whose travel is logarithmic: frequency and Q. */
    juce::NormalisableRange<float> logRange (float low, float high)
    {
        return { low, high,
                 [] (float start, float end, float proportion) { return start * std::pow (end / start, proportion); },
                 [] (float start, float end, float value) { return std::log (value / start) / std::log (end / start); },
                 [] (float start, float end, float value) { return juce::jlimit (start, end, value); } };
    }

    juce::NormalisableRange<float> choiceRange (int numChoices)
    {
        return { 0.0f, (float) (numChoices - 1), 1.0f };
    }

    std::function<juce::String (float)> choiceText (juce::StringArray names)
    {
        return [names] (float v) { return names[juce::jlimit (0, names.size() - 1, juce::roundToInt (v))]; };
    }

    juce::String decibelText (float db)
    {
        const auto rounded = std::round (db * 10.0f) / 10.0f;
        return (rounded > 0 ? "+" : "") + juce::String (juce::exactlyEqual (rounded, 0.0f) ? 0.0f : rounded, 1) + " dB";
    }

    juce::String frequencyText (float hz)
    {
        return hz < 1000.0f ? juce::String (juce::roundToInt (hz)) + " Hz" : juce::String (hz / 1000.0f, 2) + " kHz";
    }

    juce::String percentText (float proportion)
    {
        return juce::String (juce::roundToInt (proportion * 100.0f)) + "%";
    }

    juce::String msText (float ms)
    {
        return ms < 10.0f ? juce::String (ms, 1) + " ms" : juce::String (juce::roundToInt (ms)) + " ms";
    }

    float dbToGain (double db)
    {
        return (float) std::pow (10.0, db / 20.0);
    }

    double gainToDb (double gain)
    {
        return gain > 1.0e-6 ? 20.0 * std::log10 (gain) : -120.0;
    }

    // EQ Eight's bands as a new device opens: transparent (bells and shelves
    // at 0 dB, the cuts off), spread across the range.
    constexpr std::array<dsp::EqBandType, EqEightPlugin::numBands> defaultTypes {
        dsp::EqBandType::lowCut, dsp::EqBandType::lowShelf, dsp::EqBandType::bell, dsp::EqBandType::bell,
        dsp::EqBandType::bell, dsp::EqBandType::bell, dsp::EqBandType::highShelf, dsp::EqBandType::highCut
    };
    constexpr std::array<float, EqEightPlugin::numBands> defaultFrequencies { 30, 100, 250, 800, 2500, 6000, 10000, 18000 };
}

void registerNativeDevices (te::PluginManager& manager)
{
    manager.createBuiltInType<EqEightPlugin>();
    manager.createBuiltInType<CompressorV2Plugin>();
}

//==============================================================================
SpectrumTap::SpectrumTap()
    : ring (std::make_unique<std::atomic<float>[]> (ringSize)),
      scratch ((size_t) fftSize * 2, 0.0f),
      smoothed ((size_t) numBins, -120.0f)
{
    for (juce::uint32 i = 0; i < ringSize; ++i)
        ring[i].store (0.0f, std::memory_order_relaxed);
}

void SpectrumTap::push (const juce::AudioBuffer<float>& buffer, int start, int numSamples) noexcept
{
    const auto channels = juce::jmin (2, buffer.getNumChannels());

    if (channels == 0 || numSamples <= 0)
        return;

    const float* left = buffer.getReadPointer (0, start);
    const float* right = channels > 1 ? buffer.getReadPointer (1, start) : nullptr;
    const float scale = right != nullptr ? 0.5f : 1.0f;
    const auto at = written.load (std::memory_order_relaxed);

    for (int i = 0; i < numSamples; ++i)
        ring[(at + (juce::uint32) i) % ringSize].store ((left[i] + (right != nullptr ? right[i] : 0.0f)) * scale,
                                                        std::memory_order_relaxed);

    written.store (at + (juce::uint32) numSamples, std::memory_order_release);
}

bool SpectrumTap::update()
{
    const auto at = written.load (std::memory_order_acquire);

    if (at == lastRead)
    {
        // Nothing new (a stopped transport, a bypassed device): the spectrum falls away.
        for (auto& level : smoothed)
            level = juce::jmax (-120.0f, level - 1.5f);

        return false;
    }

    lastRead = at;

    for (juce::uint32 i = 0; i < (juce::uint32) fftSize; ++i)
        scratch[(size_t) i] = ring[(at - (juce::uint32) fftSize + i) % ringSize].load (std::memory_order_relaxed);

    std::fill (scratch.begin() + fftSize, scratch.end(), 0.0f);
    window.multiplyWithWindowingTable (scratch.data(), (size_t) fftSize);
    fft.performFrequencyOnlyForwardTransform (scratch.data(), true);

    // A full-scale sine reads about 0 dB: the Hann window halves the amplitude.
    const auto norm = 4.0f / (float) fftSize;

    for (int bin = 0; bin < numBins; ++bin)
    {
        const auto db = (float) gainToDb (scratch[(size_t) bin] * norm);
        auto& level = smoothed[(size_t) bin];
        level = db > level ? db : juce::jmax (db, level - 1.5f);
    }

    return true;
}

float SpectrumTap::levelAt (double hz, double sampleRate) const
{
    // The loudest bin within a sixth of an octave either side: a log display reads peaks.
    const auto binWidth = sampleRate / fftSize;
    const auto low = juce::jlimit (1, numBins - 1, (int) std::floor (hz / 1.12 / binWidth));
    const auto high = juce::jlimit (low, numBins - 1, (int) std::ceil (hz * 1.12 / binWidth));
    float level = -120.0f;

    for (int bin = low; bin <= high; ++bin)
        level = juce::jmax (level, smoothed[(size_t) bin]);

    return level;
}

//==============================================================================
const char* EqEightPlugin::xmlTypeName = "resamperEqEight";

EqEightPlugin::EqEightPlugin (te::PluginCreationInfo info) : Plugin (info)
{
    const juce::StringArray typeNames { "Low Cut", "Low Shelf", "Bell", "Notch", "High Shelf", "High Cut" };

    for (int i = 0; i < numBands; ++i)
    {
        auto& band = bands[(size_t) i];
        const auto prefix = "b" + juce::String (i + 1);
        const auto name = "Band " + juce::String (i + 1) + " ";

        band.on = makeParameter (*this, prefix + "On", name + "On", defaultTypes[(size_t) i] == dsp::EqBandType::lowCut
                                                                     || defaultTypes[(size_t) i] == dsp::EqBandType::highCut ? 0.0f : 1.0f,
                                 choiceRange (2), choiceText ({ "Off", "On" }));
        band.type = makeParameter (*this, prefix + "Type", name + "Type", (float) defaultTypes[(size_t) i],
                                   choiceRange (dsp::numEqBandTypes), choiceText (typeNames));
        band.frequency = makeParameter (*this, prefix + "Freq", name + "Frequency", defaultFrequencies[(size_t) i],
                                        logRange (20.0f, 20000.0f), frequencyText);
        band.gain = makeParameter (*this, prefix + "Gain", name + "Gain", 0.0f, { -15.0f, 15.0f }, decibelText);
        band.q = makeParameter (*this, prefix + "Q", name + "Q", 0.71f, logRange (0.1f, 18.0f),
                                [] (float q) { return juce::String (q, 2); });
        band.channel = makeParameter (*this, prefix + "Channel", name + "Channel", 0.0f, choiceRange (3),
                                      choiceText ({ "Both", "L / M", "R / S" }));
    }

    adaptive = makeParameter (*this, "adaptiveQ", "Adaptive Q", 0.0f, choiceRange (2), choiceText ({ "Off", "On" }));
    mode = makeParameter (*this, "mode", "Mode", 0.0f, choiceRange (3), choiceText ({ "St", "L/R", "M/S" }));
    scale = makeParameter (*this, "scale", "Scale", 1.0f, { 0.0f, 2.0f }, percentText);
    output = makeParameter (*this, "output", "Out", 0.0f, { -24.0f, 24.0f }, decibelText);
}

EqEightPlugin::~EqEightPlugin()
{
    notifyListenersOfDeletion();

    for (auto* p : allParameters())
        p->reset();
}

std::vector<te::ParameterWithStateValue*> EqEightPlugin::allParameters()
{
    std::vector<te::ParameterWithStateValue*> list;

    for (auto& b : bands)
        for (auto* p : { &b.on, &b.type, &b.frequency, &b.gain, &b.q, &b.channel })
            list.push_back (p);

    for (auto* p : { &adaptive, &mode, &scale, &output })
        list.push_back (p);

    return list;
}

void EqEightPlugin::initialise (const te::PluginInitialisationInfo&)
{
    glideFromSettings = true;
    outputGain = dbToGain (output.getCurrentValue());

    for (auto& channels : states)
        for (auto& s : channels)
            s.reset();

    for (auto& s : auditionStates)
        s.reset();
}

void EqEightPlugin::restorePluginStateFromValueTree (const juce::ValueTree& v)
{
    for (auto* p : allParameters())
        p->setFromValueTree (v);
}

dsp::Biquad EqEightPlugin::sectionFor (int band) const
{
    const auto& b = bands[(size_t) band];
    return sectionFor (band, b.frequency.getCurrentValue(), b.gain.getCurrentValue(), b.q.getCurrentValue());
}

dsp::Biquad EqEightPlugin::sectionFor (int band, double hz, double gainDb, double q) const
{
    const auto type = (dsp::EqBandType) juce::jlimit (0, dsp::numEqBandTypes - 1,
                                                      juce::roundToInt (bands[(size_t) band].type.getCurrentValue()));
    gainDb *= scale.getCurrentValue();

    if (adaptive.getCurrentValue() >= 0.5f && type == dsp::EqBandType::bell)
        q = dsp::adaptiveQ (q, gainDb);

    return dsp::designBand (type, hz, gainDb, q, sampleRate);
}

void EqEightPlugin::glideBands (int numSamples, bool jump) noexcept
{
    // A one-pole glide of about 20 ms; frequency glides in octaves.
    const auto k = jump ? 1.0 : 1.0 - std::exp (-numSamples / (glideSeconds * sampleRate));

    for (size_t i = 0; i < glides.size(); ++i)
    {
        const auto& b = bands[i];
        auto& glide = glides[i];
        glide.log2Hz += (std::log2 ((double) b.frequency.getCurrentValue()) - glide.log2Hz) * k;
        glide.gainDb += ((double) b.gain.getCurrentValue() - glide.gainDb) * k;
        glide.q += ((double) b.q.getCurrentValue() - glide.q) * k;
    }
}

bool EqEightPlugin::bandAppliesTo (int band, int channel) const
{
    if (juce::roundToInt (mode.getCurrentValue()) == 0)
        return true;

    const auto placement = juce::roundToInt (bands[(size_t) band].channel.getCurrentValue());
    return placement == 0 || placement == channel + 1;
}

void EqEightPlugin::applyToBuffer (const te::PluginRenderContext& fc)
{
    if (fc.destBuffer == nullptr)
        return;

    SCOPED_REALTIME_CHECK

    auto& buffer = *fc.destBuffer;
    const auto start = fc.bufferStartSample, n = fc.bufferNumSamples;
    const auto channels = juce::jmin (2, buffer.getNumChannels());

    if (channels == 0 || n <= 0)
        return;

    pre.push (buffer, start, n);

    float* data[2] = { buffer.getWritePointer (0, start), channels > 1 ? buffer.getWritePointer (1, start) : nullptr };
    const auto midSide = channels == 2 && juce::roundToInt (mode.getCurrentValue()) == 2;

    if (midSide)
        for (int i = 0; i < n; ++i)
        {
            const auto l = data[0][i], r = data[1][i];
            data[0][i] = (l + r) * 0.5f;
            data[1][i] = (l - r) * 0.5f;
        }

    if (const auto audition = auditionBand.load (std::memory_order_relaxed); juce::isPositiveAndBelow (audition, numBands))
    {
        const auto& b = bands[(size_t) audition];
        const auto section = dsp::designAudition (b.frequency.getCurrentValue(), b.q.getCurrentValue(), sampleRate);

        for (int ch = 0; ch < channels; ++ch)
            for (int i = 0; i < n; ++i)
                data[ch][i] = auditionStates[(size_t) ch].process (section, data[ch][i]);
    }
    else
    {
        // The bands glide to new settings, their sections redesigned every
        // glideBlock samples, so a moved node or knob never clicks.
        for (int done = 0; done < n; done += glideBlock)
        {
            const auto length = juce::jmin (glideBlock, n - done);
            glideBands (length, std::exchange (glideFromSettings, false));

            for (int band = 0; band < numBands; ++band)
            {
                if (bands[(size_t) band].on.getCurrentValue() < 0.5f)
                    continue;

                const auto& glide = glides[(size_t) band];
                const auto section = sectionFor (band, std::exp2 (glide.log2Hz), glide.gainDb, glide.q);

                for (int ch = 0; ch < channels; ++ch)
                {
                    if (! bandAppliesTo (band, ch))
                        continue;

                    auto& state = states[(size_t) band][(size_t) ch];

                    for (int i = done; i < done + length; ++i)
                        data[ch][i] = state.process (section, data[ch][i]);
                }
            }
        }
    }

    if (midSide)
        for (int i = 0; i < n; ++i)
        {
            const auto m = data[0][i], s = data[1][i];
            data[0][i] = m + s;
            data[1][i] = m - s;
        }

    // Out ramps across the block from where the last one ended.
    const auto gain = dbToGain (output.getCurrentValue());

    for (int ch = 0; ch < channels; ++ch)
        buffer.applyGainRamp (ch, start, n, outputGain, gain);

    outputGain = gain;
    te::clearChannels (buffer, 2, -1, start, n);
    post.push (buffer, start, n);
}

//==============================================================================
const char* CompressorV2Plugin::xmlTypeName = "resamperCompressor";

CompressorV2Plugin::CompressorV2Plugin (te::PluginCreationInfo info) : Plugin (info)
{
    auto ratioRange = juce::NormalisableRange<float> (1.0f, 20.0f);
    ratioRange.setSkewForCentre (4.0f);
    auto attackRange = juce::NormalisableRange<float> (0.1f, 100.0f);
    attackRange.setSkewForCentre (5.0f);
    auto releaseRange = juce::NormalisableRange<float> (1.0f, 2000.0f);
    releaseRange.setSkewForCentre (100.0f);

    threshold = makeParameter (*this, "threshold", "Threshold", -18.0f, { -60.0f, 0.0f }, decibelText);
    ratio = makeParameter (*this, "ratio", "Ratio", 4.0f, ratioRange,
                           [] (float r) { return (r < 9.95f ? juce::String (r, 1) : juce::String (juce::roundToInt (r))) + ":1"; });
    attack = makeParameter (*this, "attack", "Attack", 3.0f, attackRange, msText);
    release = makeParameter (*this, "release", "Release", 80.0f, releaseRange, msText);
    knee = makeParameter (*this, "knee", "Knee", 6.0f, { 0.0f, 18.0f }, decibelText);
    lookahead = makeParameter (*this, "lookahead", "Lookahead", 0.0f, choiceRange (3), choiceText ({ "0 ms", "1 ms", "10 ms" }));
    detect = makeParameter (*this, "detect", "Detect", 0.0f, choiceRange (3), choiceText ({ "Peak", "RMS", "Expand" }));
    makeupAuto = makeParameter (*this, "makeupAuto", "Makeup Auto", 1.0f, choiceRange (2), choiceText ({ "Off", "On" }));
    makeup = makeParameter (*this, "makeup", "Makeup", 0.0f, { 0.0f, 24.0f }, decibelText);
    mix = makeParameter (*this, "mix", "Mix", 1.0f, { 0.0f, 1.0f }, percentText);
    output = makeParameter (*this, "output", "Out", 0.0f, { -24.0f, 24.0f }, decibelText);
}

CompressorV2Plugin::~CompressorV2Plugin()
{
    notifyListenersOfDeletion();

    for (auto* p : { &threshold, &ratio, &attack, &release, &knee, &lookahead, &detect, &makeupAuto, &makeup, &mix, &output })
        p->reset();
}

double CompressorV2Plugin::getLatencySeconds()
{
    return dsp::lookaheadMs (juce::roundToInt (lookahead.getCurrentValue())) / 1000.0;
}

double CompressorV2Plugin::makeupDb() const
{
    if (makeupAuto.getCurrentValue() < 0.5f)
        return makeup.getCurrentValue();

    return dsp::autoMakeupDb (threshold.getCurrentValue(), ratio.getCurrentValue(), knee.getCurrentValue(),
                              juce::roundToInt (detect.getCurrentValue()) == (int) dsp::DetectMode::expand);
}

void CompressorV2Plugin::initialise (const te::PluginInitialisationInfo& info)
{
    // Room for the longest lookahead at this rate; never resized on the audio thread.
    delaySize = juce::jmax (1, (int) std::ceil (info.sampleRate * dsp::lookaheadMs (2) / 1000.0) + 1);
    delayLine.assign ((size_t) delaySize * 2, 0.0f);
    delayWrite = 0;
    envelopeDb = 0;
    meanSquare = 0;
    glideFromSettings = true;
}

void CompressorV2Plugin::restorePluginStateFromValueTree (const juce::ValueTree& v)
{
    for (auto* p : { &threshold, &ratio, &attack, &release, &knee, &lookahead, &detect, &makeupAuto, &makeup, &mix, &output })
        p->setFromValueTree (v);
}

void CompressorV2Plugin::valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& id)
{
    // The lookahead is the device's latency: the graph is rebuilt to compensate for it.
    if (v == state && id.toString() == lookahead.parameter->paramID)
        edit.restartPlayback();

    Plugin::valueTreePropertyChanged (v, id);
}

void CompressorV2Plugin::applyToBuffer (const te::PluginRenderContext& fc)
{
    if (fc.destBuffer == nullptr || delaySize == 0)
        return;

    SCOPED_REALTIME_CHECK

    auto& buffer = *fc.destBuffer;
    const auto start = fc.bufferStartSample, n = fc.bufferNumSamples;
    const auto channels = juce::jmin (2, buffer.getNumChannels());

    if (channels == 0 || n <= 0)
        return;

    const auto thresholdDb = (double) threshold.getCurrentValue();
    const auto ratioValue = (double) ratio.getCurrentValue();
    const auto kneeDb = (double) knee.getCurrentValue();
    const auto mode = (dsp::DetectMode) juce::jlimit (0, 2, juce::roundToInt (detect.getCurrentValue()));
    const auto expand = mode == dsp::DetectMode::expand;
    const auto attackCoeff = std::exp (-1.0 / (juce::jmax (0.01, (double) attack.getCurrentValue()) * 0.001 * sampleRate));
    const auto releaseCoeff = std::exp (-1.0 / (juce::jmax (0.01, (double) release.getCurrentValue()) * 0.001 * sampleRate));
    const auto rmsCoeff = std::exp (-1.0 / (0.010 * sampleRate));
    // Makeup, Mix and Out glide per sample (about 20 ms), so moving them never clicks.
    const auto glide = 1.0 - std::exp (-1.0 / (0.020 * sampleRate));
    const auto makeupTarget = makeupDb(), wetTarget = (double) mix.getCurrentValue(), outTarget = (double) output.getCurrentValue();

    if (std::exchange (glideFromSettings, false))
    {
        makeupGlideDb = makeupTarget;
        wetGlide = wetTarget;
        outGlideDb = outTarget;
    }
    const auto lag = juce::jmin (delaySize - 1, (int) std::round (getLatencySeconds() * sampleRate));

    float* data[2] = { buffer.getWritePointer (0, start), channels > 1 ? buffer.getWritePointer (1, start) : nullptr };
    float peakLeft = 0, peakRight = 0, mostReduction = 0, loudest = 0;

    for (int i = 0; i < n; ++i)
    {
        const auto l = data[0][i], r = channels > 1 ? data[1][i] : l;
        const auto absL = std::abs (l), absR = std::abs (r);
        peakLeft = juce::jmax (peakLeft, absL);
        peakRight = juce::jmax (peakRight, absR);

        double level = juce::jmax (absL, absR);

        if (mode == dsp::DetectMode::rms)
        {
            meanSquare = rmsCoeff * meanSquare + (1.0 - rmsCoeff) * 0.5 * (l * l + r * r);
            level = std::sqrt (meanSquare);
        }

        loudest = juce::jmax (loudest, (float) level);
        const auto levelDb = gainToDb (level);

        // The gain the curve wants, smoothed: attack while reduction grows, release while it shrinks.
        const auto targetDb = dsp::transferDb (levelDb, thresholdDb, ratioValue, kneeDb, expand) - levelDb;
        const auto coeff = targetDb < envelopeDb ? attackCoeff : releaseCoeff;
        envelopeDb = targetDb + coeff * (envelopeDb - targetDb);
        JUCE_UNDENORMALISE (envelopeDb);
        mostReduction = juce::jmax (mostReduction, (float) -envelopeDb);

        makeupGlideDb += (makeupTarget - makeupGlideDb) * glide;
        wetGlide += (wetTarget - wetGlide) * glide;
        outGlideDb += (outTarget - outGlideDb) * glide;
        const auto gain = dbToGain (envelopeDb + makeupGlideDb);
        const auto wet = (float) wetGlide;
        const auto outGain = dbToGain (outGlideDb);
        const auto read = (delayWrite - lag + delaySize) % delaySize;

        delayLine[(size_t) (delayWrite * 2)] = l;
        delayLine[(size_t) (delayWrite * 2 + 1)] = r;

        const auto dryL = delayLine[(size_t) (read * 2)], dryR = delayLine[(size_t) (read * 2 + 1)];
        delayWrite = (delayWrite + 1) % delaySize;

        data[0][i] = (dryL * (1.0f - wet) + dryL * gain * wet) * outGain;

        if (channels > 1)
            data[1][i] = (dryR * (1.0f - wet) + dryR * gain * wet) * outGain;
    }

    te::clearChannels (buffer, 2, -1, start, n);

    inputLeft.raise (peakLeft);
    inputRight.raise (channels > 1 ? peakRight : peakLeft);
    reduction.raise (mostReduction);
    detector.raise (loudest);
}

} // namespace resamper
