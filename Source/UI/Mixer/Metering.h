#pragma once

#include <juce_core/juce_core.h>

namespace resamper
{

/** The fader's dB law (PRD §10.3, normative): piecewise linear between
    +6 -> 0 %, 0 -> 16 %, -6 -> 31 %, -12 -> 45 %, -24 -> 64 %, -36 -> 79 %,
    -inf -> 100 % of the travel, measured from the top. -inf is floorDb. Meters
    use the same law, so their ticks line up with the fader's. */
struct FaderLaw
{
    static constexpr double floorDb = -100.0;

    /** 0 at the top (+6 dB) .. 1 at the bottom (-inf). */
    static double dbToTravel (double db);
    static double travelToDb (double travel);
};

/** A meter's peak-hold line (PRD §10.3): it jumps up to a new peak, holds it
    for holdSeconds, then falls fallDbPerSecond until it meets the level. */
class PeakHold
{
public:
    static constexpr double holdSeconds = 1.5, fallDbPerSecond = 20.0;

    /** Advances by elapsedSeconds with the latest level; returns the hold line. */
    double update (double levelDb, double elapsedSeconds);

    double get() const noexcept   { return peakDb; }
    void reset()                  { peakDb = FaderLaw::floorDb; held = 0; }

private:
    double peakDb = FaderLaw::floorDb, held = 0;
};

/** How the mixer's meters read (PRD §10.3): the peak, a 300 ms RMS, or a
    momentary loudness over 400 ms. */
enum class MeterMode { peak, rms, lufs };

/** Smooths one meter reading for its mode. Peak passes through; RMS and LUFS
    average power with a 300 ms / 400 ms time constant. The loudness reading
    has no K-weighting yet: it is a 400 ms power average. */
class MeterBallistics
{
public:
    void setMode (MeterMode m)   { mode = m; power = 0; }

    /** Feeds a reading taken elapsedSeconds after the last; returns the level shown. */
    double update (double levelDb, double elapsedSeconds);

private:
    MeterMode mode = MeterMode::peak;
    double power = 0;
};

} // namespace resamper
