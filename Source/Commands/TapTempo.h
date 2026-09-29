#pragma once

#include <deque>

namespace resamper
{

/** Tap tempo (PRD §6.1, `T`): the tempo implied by the last few taps. A pause
    longer than resetSeconds starts a new count. */
class TapTempo
{
public:
    static constexpr double resetSeconds = 2.0;
    static constexpr size_t maxIntervals = 4;

    /** Records a tap at nowSeconds; returns the averaged BPM, or 0 until there are two taps. */
    double tap (double nowSeconds)
    {
        if (hasLast && nowSeconds - last > resetSeconds)
            intervals.clear();
        else if (hasLast && nowSeconds > last)
            intervals.push_back (nowSeconds - last);

        if (intervals.size() > maxIntervals)
            intervals.pop_front();

        last = nowSeconds;
        hasLast = true;

        if (intervals.empty())
            return 0.0;

        double total = 0;

        for (auto i : intervals)
            total += i;

        return 60.0 / (total / (double) intervals.size());
    }

private:
    std::deque<double> intervals;
    double last = 0;
    bool hasLast = false;
};

} // namespace resamper
