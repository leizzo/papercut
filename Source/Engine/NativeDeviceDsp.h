#pragma once

#include <juce_core/juce_core.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <complex>

namespace resamper::dsp
{

//==============================================================================
/** The shape of one EQ Eight band (PRD §9.2.1a): 12 dB / octave cuts, RBJ
    shelves, a bell and a notch. */
enum class EqBandType { lowCut, lowShelf, bell, notch, highShelf, highCut };

inline constexpr int numEqBandTypes = 6;

/** Whether a band type has a gain (the cuts and the notch don't). */
inline bool hasGain (EqBandType t)
{
    return t == EqBandType::lowShelf || t == EqBandType::bell || t == EqBandType::highShelf;
}

/** One second-order section, normalised so a0 is 1. */
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;

    /** Its gain at frequency hz, in dB, at sampleRate. */
    double magnitudeDb (double hz, double sampleRate) const
    {
        const auto w = juce::MathConstants<double>::twoPi * hz / sampleRate;
        const std::complex<double> z1 = std::polar (1.0, -w), z2 = z1 * z1;
        const auto h = (b0 + b1 * z1 + b2 * z2) / (1.0 + a1 * z1 + a2 * z2);
        return 20.0 * std::log10 (std::max (std::abs (h), 1.0e-9));
    }
};

/** Adaptive Q: a band's Q grows as its boost or cut does (×2 at 12 dB). */
inline double adaptiveQ (double q, double gainDb)
{
    return q * std::pow (2.0, std::abs (gainDb) / 12.0);
}

/** The RBJ Audio EQ Cookbook section for a band. Frequency is clamped below Nyquist. */
inline Biquad designBand (EqBandType type, double hz, double gainDb, double q, double sampleRate)
{
    const auto nyquist = sampleRate * 0.5;
    hz = juce::jlimit (10.0, nyquist * 0.98, hz);
    q = juce::jmax (0.05, q);

    const auto w = juce::MathConstants<double>::twoPi * hz / sampleRate;
    const auto cosW = std::cos (w), sinW = std::sin (w);
    const auto alpha = sinW / (2.0 * q);
    const auto a = std::pow (10.0, gainDb / 40.0);

    double b0 = 1, b1 = 0, b2 = 0, a0 = 1, a1 = 0, a2 = 0;

    switch (type)
    {
        case EqBandType::lowCut:
            b0 = (1 + cosW) / 2; b1 = -(1 + cosW); b2 = (1 + cosW) / 2;
            a0 = 1 + alpha; a1 = -2 * cosW; a2 = 1 - alpha;
            break;

        case EqBandType::highCut:
            b0 = (1 - cosW) / 2; b1 = 1 - cosW; b2 = (1 - cosW) / 2;
            a0 = 1 + alpha; a1 = -2 * cosW; a2 = 1 - alpha;
            break;

        case EqBandType::bell:
            b0 = 1 + alpha * a; b1 = -2 * cosW; b2 = 1 - alpha * a;
            a0 = 1 + alpha / a; a1 = -2 * cosW; a2 = 1 - alpha / a;
            break;

        case EqBandType::notch:
            b0 = 1; b1 = -2 * cosW; b2 = 1;
            a0 = 1 + alpha; a1 = -2 * cosW; a2 = 1 - alpha;
            break;

        case EqBandType::lowShelf:
        {
            const auto s = 2 * std::sqrt (a) * alpha;
            b0 = a * ((a + 1) - (a - 1) * cosW + s);
            b1 = 2 * a * ((a - 1) - (a + 1) * cosW);
            b2 = a * ((a + 1) - (a - 1) * cosW - s);
            a0 = (a + 1) + (a - 1) * cosW + s;
            a1 = -2 * ((a - 1) + (a + 1) * cosW);
            a2 = (a + 1) + (a - 1) * cosW - s;
            break;
        }

        case EqBandType::highShelf:
        {
            const auto s = 2 * std::sqrt (a) * alpha;
            b0 = a * ((a + 1) + (a - 1) * cosW + s);
            b1 = -2 * a * ((a - 1) + (a + 1) * cosW);
            b2 = a * ((a + 1) + (a - 1) * cosW - s);
            a0 = (a + 1) - (a - 1) * cosW + s;
            a1 = 2 * ((a - 1) - (a + 1) * cosW);
            a2 = (a + 1) - (a - 1) * cosW - s;
            break;
        }
    }

    return { b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0 };
}

/** The band-pass an audition plays: what one band acts on, alone. */
inline Biquad designAudition (double hz, double q, double sampleRate)
{
    hz = juce::jlimit (10.0, sampleRate * 0.49, hz);
    const auto w = juce::MathConstants<double>::twoPi * hz / sampleRate;
    const auto alpha = std::sin (w) / (2.0 * juce::jmax (0.05, q));
    const auto a0 = 1 + alpha;
    return { alpha / a0, 0, -alpha / a0, -2 * std::cos (w) / a0, (1 - alpha) / a0 };
}

/** One channel's state of a Biquad (transposed direct form II). */
struct BiquadState
{
    double z1 = 0, z2 = 0;

    float process (const Biquad& c, float in) noexcept
    {
        const double x = in;
        const double y = c.b0 * x + z1;
        z1 = c.b1 * x - c.a1 * y + z2;
        z2 = c.b2 * x - c.a2 * y;
        return (float) y;
    }

    void reset() noexcept   { z1 = z2 = 0; }
};

/** The span a band's Q covers, as the ratio of its upper edge to its frequency
    (the edges are hz / ratio and hz * ratio): what the Q-width shading shows. */
inline double qEdgeRatio (double q)
{
    const auto octaves = 2.0 / std::log (2.0) * std::asinh (1.0 / (2.0 * juce::jmax (0.05, q)));
    return std::pow (2.0, octaves / 2.0);
}

//==============================================================================
/** How Compressor v2 reads its input (PRD §9.2.1a): the peak, a 10 ms RMS, or
    peak into a downward expander instead of a compressor. */
enum class DetectMode { peak, rms, expand };

/** The static curve of Compressor v2: output level for an input level, in dB,
    with a soft knee of kneeDb centred on the threshold. */
inline double transferDb (double inputDb, double thresholdDb, double ratio, double kneeDb, bool expand)
{
    const auto over = inputDb - thresholdDb;
    ratio = juce::jmax (1.0, ratio);

    if (! expand)
    {
        if (2 * over <= -kneeDb)
            return inputDb;

        if (kneeDb > 0 && 2 * std::abs (over) < kneeDb)
            return inputDb + (1.0 / ratio - 1.0) * std::pow (over + kneeDb / 2, 2.0) / (2 * kneeDb);

        return thresholdDb + over / ratio;
    }

    if (2 * over >= kneeDb)
        return inputDb;

    if (kneeDb > 0 && 2 * std::abs (over) < kneeDb)
        return inputDb - (ratio - 1.0) * std::pow (over - kneeDb / 2, 2.0) / (2 * kneeDb);

    return thresholdDb + over * ratio;
}

/** Auto makeup: half the reduction a full-scale signal would get, so a
    compressed mix comes back to about the level it went in at. */
inline double autoMakeupDb (double thresholdDb, double ratio, double kneeDb, bool expand)
{
    return expand ? 0.0 : juce::jmax (0.0, (0.0 - transferDb (0.0, thresholdDb, ratio, kneeDb, false)) / 2.0);
}

/** The lookahead choices, in ms: 0, 1 and 10. */
inline double lookaheadMs (int choice)
{
    return choice <= 0 ? 0.0 : choice == 1 ? 1.0 : 10.0;
}

//==============================================================================
/** Stores the largest value written since the last take(): how a meter's
    peak crosses from the audio thread to the UI. Wait-free on both sides. */
struct PeakSince
{
    std::atomic<float> value { 0.0f };

    static_assert (std::atomic<float>::is_always_lock_free);

    /** Audio thread. */
    void raise (float v) noexcept
    {
        auto current = value.load (std::memory_order_relaxed);

        while (v > current && ! value.compare_exchange_weak (current, v, std::memory_order_relaxed))
        {
        }
    }

    /** UI thread: the peak since the last take, and starts again from zero. */
    float take() noexcept   { return value.exchange (0.0f, std::memory_order_relaxed); }
};

} // namespace resamper::dsp
