#include "ValueFormat.h"

namespace resamper
{

namespace
{
    /** A whole string that is a number, once the unit (any case) is removed. */
    bool readNumber (juce::String text, const juce::String& unit, double& value)
    {
        text = text.trim();

        if (unit.isNotEmpty() && text.endsWithIgnoreCase (unit))
            text = text.dropLastCharacters (unit.length()).trim();

        if (text.startsWith ("+"))
            text = text.substring (1);

        if (text.isEmpty() || ! text.containsOnly ("0123456789.-") || text.lastIndexOf ("-") > 0)
            return false;

        value = text.getDoubleValue();
        return true;
    }
}

ValueFormat ValueFormat::decibels (double floorDb)
{
    return {
        [floorDb] (double db)
        {
            if (db <= floorDb)
                return juce::String ("-inf dB");

            const auto rounded = std::round (db * 10.0) / 10.0;
            return (rounded > 0 ? "+" : "") + juce::String (rounded == 0 ? 0.0 : rounded, 1) + " dB";
        },
        [floorDb] (const juce::String& text, double& value)
        {
            const auto t = text.trim().toLowerCase().replace ("db", "").trim();

            if (t == "-inf" || t == juce::String (juce::CharPointer_UTF8 ("-\xe2\x88\x9e")))
            {
                value = floorDb;
                return true;
            }

            return readNumber (t, {}, value);
        }
    };
}

ValueFormat ValueFormat::pan()
{
    return {
        [] (double pan)
        {
            const auto percent = juce::roundToInt (std::abs (pan) * 100.0);
            return percent == 0 ? juce::String ("C") : (pan < 0 ? "L" : "R") + juce::String (percent);
        },
        [] (const juce::String& text, double& value)
        {
            const auto t = text.trim().toUpperCase();

            if (t == "C")
            {
                value = 0;
                return true;
            }

            double percent = 0;

            if ((t.startsWith ("L") || t.startsWith ("R")) && readNumber (t.substring (1), {}, percent) && percent >= 0)
            {
                value = (t.startsWith ("L") ? -percent : percent) / 100.0;
                return true;
            }

            if (readNumber (t, "%", percent))
            {
                value = percent / 100.0;
                return true;
            }

            return false;
        }
    };
}

ValueFormat ValueFormat::percent()
{
    return {
        [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + "%"; },
        [] (const juce::String& text, double& value)
        {
            double percent = 0;

            if (! readNumber (text, "%", percent))
                return false;

            value = percent / 100.0;
            return true;
        }
    };
}

ValueFormat ValueFormat::bpm()
{
    return {
        [] (double v) { return juce::String (v, 2); },
        [] (const juce::String& text, double& value) { return readNumber (text, "bpm", value); }
    };
}

ValueFormat ValueFormat::number (int decimals, juce::String unit)
{
    return {
        [decimals, unit] (double v) { return juce::String (v, decimals) + (unit.isEmpty() ? juce::String() : " " + unit); },
        [unit] (const juce::String& text, double& value) { return readNumber (text, unit, value); }
    };
}

} // namespace resamper
