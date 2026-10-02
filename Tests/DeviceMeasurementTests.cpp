#include "TestFixture.h"
#include "Engine/NativeDevicePlugins.h"

#include <tracktion_engine/tracktion_engine.h>

#include <chrono>
#include <complex>

namespace te = tracktion;

namespace resamper::test
{

namespace
{
    constexpr double sampleRate = 44100.0;
    constexpr int fftOrder = 14, fftSize = 1 << fftOrder;   // Plugin Doctor's "Normal" quality

    /** The frequency of FFT bin k: a tone there fits a whole number of cycles in fftSize. */
    double binHz (int k)   { return k * sampleRate / fftSize; }

    double toDb (double gain)   { return 20.0 * std::log10 (std::max (gain, 1.0e-12)); }
    float toGain (double db)    { return (float) std::pow (10.0, db / 20.0); }

    void set (te::ParameterWithStateValue& p, float v)   { p.setParameter (v, juce::sendNotificationSync); }

    /** One device on the bench, on no track: the test signal goes straight
        through applyToBuffer, block by block, as Plugin Doctor drives a plug-in. */
    template <typename PluginType>
    struct Bench
    {
        explicit Bench (Fixture& f)
            : plugin (f.projects.getEdit().getPluginCache().createNewPlugin (PluginType::xmlTypeName, {})),
              device (*dynamic_cast<PluginType*> (plugin.get()))
        {
        }

        ~Bench()
        {
            if (initialised)
                plugin->baseClassDeinitialise();
        }

        /** Processes buffer in place, blockSize samples at a time. The first call initialises the device. */
        void process (juce::AudioBuffer<float>& buffer, int blockSize = 512)
        {
            if (! std::exchange (initialised, true))
                plugin->baseClassInitialise ({ {}, sampleRate, blockSize });

            for (int start = 0; start < buffer.getNumSamples(); start += blockSize)
            {
                const auto n = juce::jmin (blockSize, buffer.getNumSamples() - start);
                plugin->applyToBuffer ({ &buffer, start, n, nullptr, 0.0, {}, true, false, true, false });
            }
        }

        te::Plugin::Ptr plugin;
        PluginType& device;
        bool initialised = false;
    };

    //==============================================================================
    /** Plugin Doctor's "Delta": one sample of left / right, then silence. */
    juce::AudioBuffer<float> impulse (float left, float right, int numSamples = fftSize)
    {
        juce::AudioBuffer<float> b (2, numSamples);
        b.clear();
        b.setSample (0, 0, left);
        b.setSample (1, 0, right);
        return b;
    }

    /** A sine at FFT bin k on both channels, held at each level (dBFS) for its
        seconds in turn, its phase running on across the steps. */
    juce::AudioBuffer<float> steps (int k, std::initializer_list<std::pair<double, double>> secondsAndDb)
    {
        int total = 0;

        for (auto [seconds, db] : secondsAndDb)
            total += (int) std::round (seconds * sampleRate);

        juce::AudioBuffer<float> b (2, total);
        int i = 0;

        for (auto [seconds, db] : secondsAndDb)
        {
            const auto amplitude = toGain (db);

            for (const auto end = i + (int) std::round (seconds * sampleRate); i < end; ++i)
            {
                const auto x = amplitude * (float) std::sin (juce::MathConstants<double>::twoPi * k * (i % fftSize) / fftSize);
                b.setSample (0, i, x);
                b.setSample (1, i, x);
            }
        }

        return b;
    }

    juce::AudioBuffer<float> sine (int k, double db, double seconds)   { return steps (k, { { seconds, db } }); }

    /** The spectrum of fftSize samples of channel from start; bin k is at binHz (k). */
    std::vector<std::complex<double>> spectrum (const juce::AudioBuffer<float>& b, int channel, int start = 0)
    {
        juce::dsp::FFT fft (fftOrder);
        std::vector<float> data ((size_t) fftSize * 2, 0.0f);
        std::copy_n (b.getReadPointer (channel, start), fftSize, data.begin());
        fft.performRealOnlyForwardTransform (data.data());

        std::vector<std::complex<double>> bins (fftSize / 2 + 1);

        for (size_t k = 0; k < bins.size(); ++k)
            bins[k] = { data[2 * k], data[2 * k + 1] };

        return bins;
    }

    /** A section's complex response at hz, the way BiquadState runs it. */
    std::complex<double> response (const dsp::Biquad& c, double hz)
    {
        const auto z1 = std::polar (1.0, -juce::MathConstants<double>::twoPi * hz / sampleRate), z2 = z1 * z1;
        return (c.b0 + c.b1 * z1 + c.b2 * z2) / (1.0 + c.a1 * z1 + c.a2 * z2);
    }

    /** Plugin Doctor's THD and THD+N, in dB below the fundamental at bin k, over the last fftSize samples. */
    struct Distortion
    {
        double thdDb, thdnDb;
    };

    Distortion distortion (const juce::AudioBuffer<float>& b, int k)
    {
        const auto bins = spectrum (b, 0, b.getNumSamples() - fftSize);
        const auto fundamental = std::norm (bins[(size_t) k]);
        double harmonics = 0, everything = 0;

        for (int h = 2 * k; h < fftSize / 2; h += k)
            harmonics += std::norm (bins[(size_t) h]);

        for (size_t i = 1; i < bins.size(); ++i)
            everything += std::norm (bins[i]);

        return { 10.0 * std::log10 (std::max (harmonics, 1.0e-30) / fundamental),
                 10.0 * std::log10 (std::max (everything - fundamental, 1.0e-30) / fundamental) };
    }

    /** The loudest sample of channel 0 in [from, from + length) seconds. */
    float peakIn (const juce::AudioBuffer<float>& b, double from, double length)
    {
        const auto start = (int) std::round (from * sampleRate);
        return b.getMagnitude (0, start, juce::jmin ((int) std::round (length * sampleRate), b.getNumSamples() - start));
    }

    /** Whether two renders are the same to within float rounding. */
    bool same (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        for (int ch = 0; ch < a.getNumChannels(); ++ch)
            for (int i = 0; i < a.getNumSamples(); ++i)
                if (std::abs (a.getSample (ch, i) - b.getSample (ch, i)) > 1.0e-6f)
                    return false;

        return true;
    }

    /** Nanoseconds per stereo sample at blockSize, over a second of noise. */
    template <typename PluginType, typename Setup>
    double nanosecondsPerSample (Fixture& f, int blockSize, Setup setup)
    {
        Bench<PluginType> bench (f);
        setup (bench.device);

        juce::AudioBuffer<float> noise (2, (int) sampleRate);
        juce::Random random (1);

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < noise.getNumSamples(); ++i)
                noise.setSample (ch, i, random.nextFloat() * 0.5f - 0.25f);

        const auto began = std::chrono::steady_clock::now();
        bench.process (noise, blockSize);
        const std::chrono::duration<double, std::nano> took = std::chrono::steady_clock::now() - began;
        return took.count() / noise.getNumSamples();
    }

    //==============================================================================
    /** The EQ Eight every linear measurement runs: all six band types, and Out. */
    void shapeEq (EqEightPlugin& eq)
    {
        const struct { dsp::EqBandType type; float hz, gainDb, q; } settings[] = {
            { dsp::EqBandType::lowCut, 40.0f, 0.0f, 0.71f },
            { dsp::EqBandType::lowShelf, 120.0f, -4.0f, 0.71f },
            { dsp::EqBandType::bell, 1000.0f, 6.0f, 1.0f },
            { dsp::EqBandType::bell, 3500.0f, -8.0f, 4.0f },
            { dsp::EqBandType::notch, 6000.0f, 0.0f, 8.0f },
            { dsp::EqBandType::highShelf, 8000.0f, 3.0f, 0.71f },
            { dsp::EqBandType::bell, 12000.0f, 0.0f, 0.71f },
            { dsp::EqBandType::highCut, 16000.0f, 0.0f, 0.71f },
        };

        for (int b = 0; b < EqEightPlugin::numBands; ++b)
        {
            auto& band = eq.bands[(size_t) b];
            set (band.on, 1.0f);
            set (band.type, (float) settings[b].type);
            set (band.frequency, settings[b].hz);
            set (band.gain, settings[b].gainDb);
            set (band.q, settings[b].q);
        }

        set (eq.output, 2.0f);
    }

    /** What the Display draws for channel at hz, with Out: every band that is on and applies. */
    std::complex<double> eqCurve (EqEightPlugin& eq, double hz, int channel = 0)
    {
        std::complex<double> h = toGain (eq.output.getCurrentValue());

        for (int b = 0; b < EqEightPlugin::numBands; ++b)
            if (eq.bands[(size_t) b].on.getCurrentValue() >= 0.5f && eq.bandAppliesTo (b, channel))
                h *= response (eq.sectionFor (b), hz);

        return h;
    }

    /** A Compressor with a hard knee, no makeup and the given threshold, ratio, attack and release. */
    void shapeCompressor (CompressorV2Plugin& c, float thresholdDb, float ratio, float attackMs, float releaseMs)
    {
        set (c.threshold, thresholdDb);
        set (c.ratio, ratio);
        set (c.attack, attackMs);
        set (c.release, releaseMs);
        set (c.knee, 0.0f);
        set (c.makeupAuto, 0.0f);
        set (c.makeup, 0.0f);
    }
}

/** EQ Eight and Compressor v2 measured as Plugin Doctor measures a plug-in
    (PRD §9.2.1a): Linear (Delta), Harmonic (THD), Dynamics (Ramp and
    Attack/Release) and Performance. Each runs the device directly at
    44.1 kHz and logs what it read; the expectations are what must hold. */
struct DeviceMeasurementTests : juce::UnitTest
{
    DeviceMeasurementTests() : juce::UnitTest ("Device Measurements", "Resamper") {}

    void runTest() override
    {
        Fixture f;

        //==============================================================================
        beginTest ("EQ Eight, Linear / Delta: the audio's magnitude and phase are the Display's curve");
        {
            Bench<EqEightPlugin> bench (f);
            shapeEq (bench.device);
            auto ir = impulse (0.5f, 0.5f);
            bench.process (ir);

            const auto bins = spectrum (ir, 0);
            double worstDb = 0, worstDegrees = 0;

            for (double hz = 20.0; hz < 20000.0; hz *= std::pow (2.0, 1.0 / 6.0))
            {
                const auto k = (int) std::round (hz * fftSize / sampleRate);
                const auto expected = eqCurve (bench.device, binHz (k));

                if (toDb (std::abs (expected)) < -40.0)
                    continue;   // the notch's and the cuts' depths are below float precision

                const auto measured = bins[(size_t) k] / 0.5;
                worstDb = std::max (worstDb, std::abs (toDb (std::abs (measured)) - toDb (std::abs (expected))));
                worstDegrees = std::max (worstDegrees, std::abs (std::arg (measured / expected)) * 180.0 / juce::MathConstants<double>::pi);
            }

            logMessage ("  EQ Eight audio vs Display: " + juce::String (worstDb, 4) + " dB, " + juce::String (worstDegrees, 4) + " degrees at worst");
            expectLessThan (worstDb, 0.01, "the magnitude is the curve");
            expectLessThan (worstDegrees, 0.1, "the phase is the curve: minimum phase, no latency");
        }

        beginTest ("EQ Eight, Linear: the cuts fall 12 dB an octave; near Nyquist a bell cramps");
        {
            Bench<EqEightPlugin> bench (f);
            auto& cut = bench.device.bands[0];
            set (cut.on, 1.0f);
            set (cut.type, (float) dsp::EqBandType::lowCut);
            set (cut.frequency, 2000.0f);

            auto& bell = bench.device.bands[7];
            set (bell.on, 1.0f);
            set (bell.type, (float) dsp::EqBandType::bell);
            set (bell.frequency, 16000.0f);
            set (bell.gain, 6.0f);
            set (bell.q, 1.0f);

            auto ir = impulse (1.0f, 1.0f);
            bench.process (ir);
            const auto bins = spectrum (ir, 0);
            const auto at = [&] (double hz) { return toDb (std::abs (bins[(size_t) std::round (hz * fftSize / sampleRate)])); };

            // Four octaves under the corner, where the slope has reached its asymptote.
            const auto slope = at (125.0) - at (62.5);
            logMessage ("  Low Cut at 2 kHz, 62.5 -> 125 Hz: " + juce::String (slope, 2) + " dB / octave");
            expectWithinAbsoluteError (slope, 12.0, 0.1);

            // The analog bell the RBJ section is the bilinear transform of: symmetric about its centre in octaves.
            const auto analog = [] (double hz)
            {
                const auto a = std::pow (10.0, 6.0 / 40.0);
                const std::complex<double> s (0.0, hz / 16000.0);
                return toDb (std::abs ((s * s + s * (a / 1.0) + 1.0) / (s * s + s / (a * 1.0) + 1.0)));
            };

            for (auto hz : { 8000.0, 12000.0, 16000.0, 20000.0 })
                logMessage ("  Bell +6 dB at 16 kHz, at " + juce::String (hz / 1000.0, 0) + " kHz: "
                            + juce::String (at (hz), 2) + " dB (analog " + juce::String (analog (hz), 2) + " dB)");

            expectWithinAbsoluteError (at (16000.0), 6.0, 0.05, "the bell peaks at its frequency");
            expectLessThan (at (20000.0), analog (20000.0) - 1.0, "above the bell the curve is pressed toward Nyquist");
        }

        beginTest ("EQ Eight, Linear / M/S: a Side band leaves a centred impulse alone and shapes an anti-phase one");
        {
            const auto measure = [&] (float left, float right)
            {
                Bench<EqEightPlugin> bench (f);
                set (bench.device.mode, 2.0f);
                auto& band = bench.device.bands[2];
                set (band.type, (float) dsp::EqBandType::bell);
                set (band.frequency, 1000.0f);
                set (band.gain, 6.0f);
                set (band.channel, 2.0f);

                auto ir = impulse (left, right);
                bench.process (ir);
                const auto k = (size_t) std::round (1000.0 * fftSize / sampleRate);
                return std::pair { toDb (std::abs (spectrum (ir, 0)[k] / (double) left)),
                                   toDb (std::abs (spectrum (ir, 1)[k] / (double) right)) };
            };

            const auto centred = measure (0.5f, 0.5f), antiPhase = measure (0.5f, -0.5f);
            expectWithinAbsoluteError (centred.first, 0.0, 0.001);
            expectWithinAbsoluteError (centred.second, 0.0, 0.001);
            expectWithinAbsoluteError (antiPhase.first, 6.0, 0.01);
            expectWithinAbsoluteError (antiPhase.second, 6.0, 0.01);
        }

        beginTest ("EQ Eight, Harmonic / THD: it adds no harmonics");
        {
            for (auto k : { 19, 372 })
            {
                Bench<EqEightPlugin> bench (f);
                shapeEq (bench.device);
                auto tone = sine (k, -6.0, 1.0);
                bench.process (tone);

                const auto d = distortion (tone, k);
                logMessage ("  EQ Eight, " + juce::String (binHz (k), 1) + " Hz at -6 dBFS: THD " + juce::String (d.thdDb, 1)
                            + " dB, THD+N " + juce::String (d.thdnDb, 1) + " dB");
                expectLessThan (d.thdDb, -100.0);
                expectLessThan (d.thdnDb, -100.0);
            }
        }

        beginTest ("EQ Eight: the block size changes nothing");
        {
            const auto render = [&] (int blockSize)
            {
                Bench<EqEightPlugin> bench (f);
                shapeEq (bench.device);
                auto ir = impulse (0.5f, -0.25f, 4096);
                bench.process (ir, blockSize);
                return ir;
            };

            const auto reference = render (512);

            for (auto blockSize : { 1, 7, 32, 100 })
                expect (same (render (blockSize), reference), "block size " + juce::String (blockSize));
        }

        //==============================================================================
        beginTest ("Compressor, Dynamics / Ramp: the measured curve is the Display's curve");
        {
            for (auto mode : { dsp::DetectMode::peak, dsp::DetectMode::rms })
            {
                Bench<CompressorV2Plugin> bench (f);
                shapeCompressor (bench.device, -18.0f, 4.0f, 3.0f, 80.0f);
                set (bench.device.knee, 6.0f);
                set (bench.device.detect, (float) mode);

                // Plugin Doctor's ramp: one level a step, each held long enough to settle, read at its end.
                constexpr double hold = 0.3;
                std::vector<double> levels;

                for (double db = -40.0; db <= 0.0; db += 1.0)
                    levels.push_back (db);

                double worst = 0, worstAt = 0;

                for (auto db : levels)
                {
                    auto tone = sine (372, db, hold);
                    bench.process (tone);

                    // An RMS detector reads a sine 3 dB below its peak.
                    const auto detected = mode == dsp::DetectMode::rms ? db - 3.0103 : db;
                    const auto expected = db + dsp::transferDb (detected, -18.0, 4.0, 6.0, false) - detected;
                    const auto error = toDb (peakIn (tone, hold - 0.05, 0.05)) - expected;

                    if (std::abs (error) > std::abs (worst))
                    {
                        worst = error;
                        worstAt = db;
                    }
                }

                const auto name = juce::String (mode == dsp::DetectMode::peak ? "Peak" : "RMS");
                logMessage ("  Compressor " + name + ", ramp -40..0 dB at 1 kHz: worst " + juce::String (worst, 2)
                            + " dB off the curve, at " + juce::String (worstAt, 0) + " dBFS in");

                expectLessThan (std::abs (worst), 0.25, name);
            }
        }

        beginTest ("Compressor, Dynamics / Attack-Release: the gain moves at the set times");
        {
            Bench<CompressorV2Plugin> bench (f);
            constexpr float attackMs = 10.0f, releaseMs = 100.0f;
            shapeCompressor (bench.device, -20.0f, 10.0f, attackMs, releaseMs);

            // Below, above, below the threshold, as Plugin Doctor's Attack/Release sends.
            constexpr double quiet = -40.0, loud = -6.0, rise = 0.2, fall = 0.7;
            auto tone = steps (372, { { rise, quiet }, { fall - rise, loud }, { 0.8, quiet } });
            const auto input = tone;
            bench.process (tone);

            // The gain over one period of the tone at a time.
            const auto period = (double) fftSize / 372 / sampleRate;
            const auto gainDbAt = [&] (double t)
            {
                return toDb (peakIn (tone, t, period)) - toDb (peakIn (input, t, period));
            };

            const auto settled = gainDbAt (fall - 0.02);
            const auto timeTo = [&] (double from, double toward, bool down)
            {
                for (auto t = from; t < from + 0.5; t += period)
                    if (down ? gainDbAt (t) <= toward : gainDbAt (t) >= toward)
                        return (t - from) * 1000.0;

                return 1000.0;
            };

            // One time constant: 63 % of the way.
            const auto attackTook = timeTo (rise, settled * 0.632, true);
            const auto releaseTook = timeTo (fall, settled * 0.368, false);

            logMessage ("  Compressor 10 ms / 100 ms, -40 -> -6 dBFS steps at 1 kHz: settles at " + juce::String (settled, 2)
                        + " dB (curve " + juce::String (dsp::transferDb (loud, -20.0, 10.0, 0.0, false) - loud, 2)
                        + "), attack " + juce::String (attackTook, 1) + " ms, release " + juce::String (releaseTook, 1) + " ms to 63 %");

            expectWithinAbsoluteError (settled, dsp::transferDb (loud, -20.0, 10.0, 0.0, false) - loud, 0.2, "the gain settles on the curve");
            expectWithinAbsoluteError (attackTook, (double) attackMs, attackMs * 0.15);
            expectWithinAbsoluteError (releaseTook, (double) releaseMs, releaseMs * 0.15);
        }

        beginTest ("Compressor, Expand: a tone above the threshold passes untouched");
        {
            for (auto k : { 19, 372 })
            {
                Bench<CompressorV2Plugin> bench (f);
                shapeCompressor (bench.device, -30.0f, 4.0f, 1.0f, 80.0f);
                set (bench.device.detect, (float) dsp::DetectMode::expand);
                auto tone = sine (k, -6.0, 1.0);
                bench.process (tone);

                const auto gain = toDb (peakIn (tone, 0.9, 0.1)) + 6.0;
                const auto d = distortion (tone, k);
                logMessage ("  Expand, " + juce::String (binHz (k), 0) + " Hz at -6 dBFS, 24 dB over the threshold: gain "
                            + juce::String (gain, 2) + " dB, THD " + juce::String (d.thdDb, 1) + " dB");
                expectWithinAbsoluteError (gain, 0.0, 0.1);
                // A low tone spends longer under the threshold at each zero crossing, where
                // the held gain releases a little toward the expander's: a residue there.
                expectLessThan (d.thdDb, k < 100 ? -70.0 : -80.0);
            }
        }

        beginTest ("Compressor, Linear / Delta: the lookahead is the latency it reports, and Mix doesn't comb");
        {
            for (int choice = 0; choice < 3; ++choice)
            {
                Bench<CompressorV2Plugin> bench (f);
                shapeCompressor (bench.device, -18.0f, 4.0f, 3.0f, 80.0f);
                set (bench.device.lookahead, (float) choice);
                set (bench.device.makeup, 6.0f);
                set (bench.device.mix, 0.5f);

                constexpr float level = 0.01f;   // -40 dBFS: below the threshold
                auto ir = impulse (level, level, 2048);
                bench.process (ir);

                const auto latency = juce::roundToInt (bench.device.getLatencySeconds() * sampleRate);
                expectEquals (latency, juce::roundToInt (dsp::lookaheadMs (choice) * sampleRate / 1000.0));

                // Half dry, half +6 dB: one sample, so a flat magnitude and a pure delay.
                expectWithinAbsoluteError (ir.getSample (0, latency), level * (0.5f + 0.5f * toGain (6.0)), 1.0e-6f);
                ir.setSample (0, latency, 0.0f);
                ir.setSample (1, latency, 0.0f);
                expectLessThan (ir.getMagnitude (0, ir.getNumSamples()), 1.0e-9f, "nothing but the delayed impulse");
            }
        }

        beginTest ("Compressor, Attack/Release: the lookahead catches the onset");
        {
            const auto overshootDb = [&] (int choice)
            {
                Bench<CompressorV2Plugin> bench (f);
                shapeCompressor (bench.device, -20.0f, 20.0f, 1.0f, 80.0f);
                set (bench.device.lookahead, (float) choice);

                auto tone = steps (372, { { 0.1, -40.0 }, { 0.3, -6.0 } });
                bench.process (tone);
                const auto latency = bench.device.getLatencySeconds();
                return toDb (peakIn (tone, 0.1 + latency, 0.003)) - toDb (peakIn (tone, 0.35, 0.05));
            };

            const auto without = overshootDb (0), with1 = overshootDb (1), with10 = overshootDb (2);
            logMessage ("  Compressor 1 ms attack, -40 -> -6 dBFS: the onset is over the settled level by "
                        + juce::String (without, 2) + " dB, " + juce::String (with1, 2) + " dB with 1 ms lookahead, "
                        + juce::String (with10, 2) + " dB with 10 ms");

            expectGreaterThan (without, 3.0, "without lookahead the attack lets the onset through");
            expectLessThan (with10, 0.5, "10 ms is ten attack times: the gain is down before the onset");
            expectLessThan (with1, without);
        }

        beginTest ("Compressor, Harmonic / THD: below the threshold a pure gain; a fast release distorts a low tone");
        {
            const auto measure = [&] (int k, double db, float releaseMs)
            {
                Bench<CompressorV2Plugin> bench (f);
                shapeCompressor (bench.device, -24.0f, 4.0f, 1.0f, releaseMs);
                set (bench.device.makeup, 6.0f);
                auto tone = sine (k, db, 1.0);
                bench.process (tone);
                return distortion (tone, k);
            };

            const auto below = measure (372, -40.0, 10.0f);
            logMessage ("  Compressor below the threshold: THD " + juce::String (below.thdDb, 1) + " dB");
            expectLessThan (below.thdDb, -100.0);

            for (auto k : { 19, 372 })
            {
                juce::String line = "  Compressor, " + juce::String (binHz (k), 0) + " Hz at -6 dBFS, 1 ms attack, THD by release:";

                for (auto release : { 10.0f, 80.0f, 500.0f })
                    line << " " << juce::String (release, 0) << " ms " << juce::String (measure (k, -6.0, release).thdDb, 1) << " dB;";

                logMessage (line);
            }

            expectGreaterThan (measure (19, -6.0, 10.0f).thdDb, measure (19, -6.0, 500.0f).thdDb + 10.0,
                               "a release shorter than the tone's period rides its waveform");
        }

        beginTest ("Compressor: the channels are linked, and the block size changes nothing");
        {
            Bench<CompressorV2Plugin> bench (f);
            shapeCompressor (bench.device, -20.0f, 4.0f, 3.0f, 80.0f);
            auto tone = sine (372, -6.0, 0.5);
            tone.applyGain (1, 0, tone.getNumSamples(), toGain (-30.0));   // the right channel well below the threshold
            const auto input = tone;
            bench.process (tone);

            const auto gainOf = [&] (int ch) { return toDb (tone.getMagnitude (ch, 20000, 2000) / input.getMagnitude (ch, 20000, 2000)); };
            expectLessThan (gainOf (0), -5.0);
            expectWithinAbsoluteError (gainOf (1), gainOf (0), 0.01, "the quiet channel is turned down with the loud one");

            const auto render = [&] (int blockSize)
            {
                Bench<CompressorV2Plugin> b (f);
                shapeCompressor (b.device, -20.0f, 4.0f, 3.0f, 80.0f);
                set (b.device.lookahead, 1.0f);
                auto burst = steps (372, { { 0.05, -40.0 }, { 0.1, -6.0 }, { 0.05, -40.0 } });
                b.process (burst, blockSize);
                return burst;
            };

            const auto reference = render (512);

            for (auto blockSize : { 1, 7, 32, 100 })
                expect (same (render (blockSize), reference), "block size " + juce::String (blockSize));
        }

        //==============================================================================
        beginTest ("Performance: the time per sample at each block size (logged, not checked)");
        {
            for (auto blockSize : { 1, 8, 32, 64, 512 })
            {
                const auto eq = nanosecondsPerSample<EqEightPlugin> (f, blockSize, shapeEq);
                const auto compressor = nanosecondsPerSample<CompressorV2Plugin> (f, blockSize, [] (CompressorV2Plugin&) {});
                logMessage ("  block " + juce::String (blockSize).paddedLeft (' ', 3) + ": EQ Eight " + juce::String (eq, 0)
                            + " ns, Compressor " + juce::String (compressor, 0) + " ns a stereo sample");
            }
        }
    }
};

static DeviceMeasurementTests deviceMeasurementTests;

} // namespace resamper::test
