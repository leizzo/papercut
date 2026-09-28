#include "Metering.h"

#include <array>

namespace papercut
{

namespace
{
    struct Mark { double db, travel; };

    constexpr std::array<Mark, 7> marks
    { {
        { 6.0, 0.0 }, { 0.0, 0.16 }, { -6.0, 0.31 }, { -12.0, 0.45 }, { -24.0, 0.64 }, { -36.0, 0.79 },
        { FaderLaw::floorDb, 1.0 },
    } };
}

double FaderLaw::dbToTravel (double db)
{
    db = juce::jlimit (floorDb, marks.front().db, db);

    for (size_t i = 1; i < marks.size(); ++i)
        if (db >= marks[i].db)
        {
            const auto& a = marks[i - 1];
            const auto& b = marks[i];
            return a.travel + (a.db - db) / (a.db - b.db) * (b.travel - a.travel);
        }

    return 1.0;
}

double FaderLaw::travelToDb (double travel)
{
    travel = juce::jlimit (0.0, 1.0, travel);

    for (size_t i = 1; i < marks.size(); ++i)
        if (travel <= marks[i].travel)
        {
            const auto& a = marks[i - 1];
            const auto& b = marks[i];
            return a.db - (travel - a.travel) / (b.travel - a.travel) * (a.db - b.db);
        }

    return floorDb;
}

double PeakHold::update (double levelDb, double elapsedSeconds)
{
    if (levelDb >= peakDb)
    {
        peakDb = levelDb;
        held = 0;
        return peakDb;
    }

    held += elapsedSeconds;

    if (held > holdSeconds)
    {
        const auto falling = std::min (elapsedSeconds, held - holdSeconds);
        peakDb = std::max ({ levelDb, FaderLaw::floorDb, peakDb - falling * fallDbPerSecond });
    }

    return peakDb;
}

} // namespace papercut
