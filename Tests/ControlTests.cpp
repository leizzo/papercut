#include "TestFixture.h"

#include "UI/Controls/ContinuousValue.h"
#include "UI/Controls/ValueFormat.h"
#include "UI/Mixer/Metering.h"

namespace resamper::test
{

/** The continuous-control model (PRD §16.2) and value text in and out. */
struct ControlTests : juce::UnitTest
{
    ControlTests() : juce::UnitTest ("Controls", "Resamper") {}

    double parsed (const ValueFormat& format, const juce::String& text)
    {
        auto value = std::numeric_limits<double>::quiet_NaN();
        expect (format.parse (text, value), "should parse: " + text);
        return value;
    }

    void expectRejects (const ValueFormat& format, const juce::String& text)
    {
        double value = 0;
        expect (! format.parse (text, value), "should reject: " + text);
    }

    static ContinuousValue::Spec spec (double minimum, double maximum, double defaultValue, ValueFormat format)
    {
        ContinuousValue::Spec s;
        s.minimum = minimum;
        s.maximum = maximum;
        s.defaultValue = defaultValue;
        s.format = std::move (format);
        return s;
    }

    void runTest() override
    {
        const auto db = ValueFormat::decibels();
        const auto pan = ValueFormat::pan();
        const auto percent = ValueFormat::percent();
        const auto bpm = ValueFormat::bpm();

        beginTest ("dB: formats with one decimal and a sign, -inf at the floor, parses units");
        {
            expectEquals (db.format (-6.0), juce::String ("-6.0 dB"));
            expectEquals (db.format (2.4), juce::String ("+2.4 dB"));
            expectEquals (db.format (0.0), juce::String ("0.0 dB"));
            expectEquals (db.format (-100.0), juce::String ("-inf dB"));
            expectEquals (parsed (db, "-6db"), -6.0);
            expectEquals (parsed (db, "-6 dB"), -6.0);
            expectEquals (parsed (db, " +3.5 "), 3.5);
            expectEquals (parsed (db, "-inf"), -100.0);
            expectEquals (parsed (db, juce::String (juce::CharPointer_UTF8 ("-\xe2\x88\x9e"))), -100.0);
            expectRejects (db, "loud");
            expectRejects (db, "");
        }

        beginTest ("Pan: C, L30, R20; parses L/R, C and signed percents");
        {
            expectEquals (pan.format (0.0), juce::String ("C"));
            expectEquals (pan.format (-0.3), juce::String ("L30"));
            expectEquals (pan.format (0.2), juce::String ("R20"));
            expectEquals (parsed (pan, "L30"), -0.3);
            expectEquals (parsed (pan, "r20"), 0.2);
            expectEquals (parsed (pan, "C"), 0.0);
            expectEquals (parsed (pan, "-50"), -0.5);
            expectEquals (parsed (pan, "L 100"), -1.0);
            expectRejects (pan, "X20");
        }

        beginTest ("Percent: 40%; parses with or without the sign");
        {
            expectEquals (percent.format (0.4), juce::String ("40%"));
            expectEquals (parsed (percent, "40%"), 0.4);
            expectEquals (parsed (percent, "72"), 0.72);
            expectRejects (percent, "half");
        }

        beginTest ("BPM: two decimals; parses with or without the unit");
        {
            expectEquals (bpm.format (124.0), juce::String ("124.00"));
            expectEquals (parsed (bpm, "124.5 bpm"), 124.5);
            expectEquals (parsed (bpm, "90"), 90.0);
            expectRejects (bpm, "fast");
        }

        //==============================================================================
        auto panSpec = spec (-1.0, 1.0, 0.0, pan);
        auto dbSpec = spec (-100.0, 6.0, 0.0, db);
        dbSpec.wheelStep = 0.5;

        beginTest ("A 200 px drag covers the full range; Shift makes it 10 times finer");
        {
            ContinuousValue v (panSpec);
            v.setValue (-1.0);
            v.beginDrag();
            v.dragBy (200.0f, false);
            expectWithinAbsoluteError (v.getValue(), 1.0, 1.0e-9);

            v.setValue (0.0);
            v.beginDrag();
            v.dragBy (100.0f, true);
            expectWithinAbsoluteError (v.getValue(), 0.1, 1.0e-9);
        }

        beginTest ("Dragging past the ends clamps; dragging back moves off the end again at once");
        {
            ContinuousValue v (panSpec);
            v.beginDrag();
            v.dragBy (1000.0f, false);
            expectEquals (v.getValue(), 1.0);
            v.dragBy (-20.0f, false);
            expectWithinAbsoluteError (v.getValue(), 0.8, 1.0e-9);
        }

        beginTest ("Reset returns to the default");
        {
            ContinuousValue v (dbSpec);
            v.setValue (-12.0);
            v.reset();
            expectEquals (v.getValue(), 0.0);
        }

        beginTest ("The wheel steps 0.5 dB on a dB control and 1% of the range otherwise");
        {
            ContinuousValue d (dbSpec);
            d.setValue (-6.0);
            d.step (1);
            expectEquals (d.getValue(), -5.5);
            d.step (-2);
            expectEquals (d.getValue(), -6.5);

            ContinuousValue p (panSpec);
            p.step (1);
            expectWithinAbsoluteError (p.getValue(), 0.02, 1.0e-9);
        }

        beginTest ("A custom travel mapping drives the drag (e.g. a fader curve)");
        {
            auto spec = dbSpec;
            spec.toProportion = [] (double value) { return (value + 100.0) / 106.0; };
            spec.fromProportion = [] (double p) { return p * 106.0 - 100.0; };
            ContinuousValue v (spec);
            v.setValue (-100.0);
            v.beginDrag();
            v.dragBy (100.0f, false);
            expectWithinAbsoluteError (v.getValue(), -47.0, 1.0e-9);
        }

        beginTest ("A per-pixel drag moves in value units: tempo is 1 BPM/px, Shift 0.01");
        {
            auto tempo = spec (20.0, 999.0, 120.0, bpm);
            tempo.unitsPerPixel = 1.0;
            tempo.fineUnitsPerPixel = 0.01;
            ContinuousValue v (tempo);
            v.beginDrag();
            v.dragBy (4.0f, false);
            expectWithinAbsoluteError (v.getValue(), 124.0, 1.0e-9);
            v.dragBy (50.0f, true);
            expectWithinAbsoluteError (v.getValue(), 124.5, 1.0e-9);
        }

        beginTest ("Within one drag, only the first change starts an undo step");
        {
            ContinuousValue v (panSpec);
            juce::Array<bool> continues;
            v.onChange = [&] (double, bool c) { continues.add (c); };

            v.beginDrag();
            v.dragBy (10.0f, false);
            v.dragBy (20.0f, false);
            v.dragBy (30.0f, false);
            v.endDrag();

            v.step (1);
            v.reset();

            expectEquals (continues.size(), 5);
            expect (! continues[0] && continues[1] && continues[2]);
            expect (! continues[3], "a wheel step is its own gesture");
            expect (! continues[4], "a reset is its own gesture");
        }

        beginTest ("A change that doesn't move the value reports nothing");
        {
            ContinuousValue v (panSpec);
            int calls = 0;
            v.onChange = [&] (double, bool) { ++calls; };
            v.reset();
            v.beginDrag();
            v.dragBy (0.0f, false);
            expectEquals (calls, 0);
        }

        beginTest ("Text entry parses units and clamps; a bad entry changes nothing");
        {
            ContinuousValue v (dbSpec);
            expect (v.commitText ("-6db"));
            expectEquals (v.getValue(), -6.0);
            expect (v.commitText ("+20 dB"));
            expectEquals (v.getValue(), 6.0);
            expect (! v.commitText ("nope"));
            expectEquals (v.getValue(), 6.0);
            expectEquals (v.getText(), juce::String ("+6.0 dB"));
        }

        beginTest ("The fader law puts the PRD's dB marks at their travel, both ways");
        {
            const std::pair<double, double> marks[] = { { 6.0, 0.0 }, { 0.0, 0.16 }, { -6.0, 0.31 }, { -12.0, 0.45 },
                                                        { -24.0, 0.64 }, { -36.0, 0.79 }, { FaderLaw::floorDb, 1.0 } };

            for (auto [markDb, travel] : marks)
            {
                expectWithinAbsoluteError (FaderLaw::dbToTravel (markDb), travel, 1.0e-9);
                expectWithinAbsoluteError (FaderLaw::travelToDb (travel), markDb, 1.0e-9);
            }

            // Piecewise linear: half-way between two marks.
            expectWithinAbsoluteError (FaderLaw::dbToTravel (-9.0), 0.38, 1.0e-9);
            expectWithinAbsoluteError (FaderLaw::travelToDb (0.38), -9.0, 1.0e-9);

            // Out of range clamps.
            expectEquals (FaderLaw::dbToTravel (20.0), 0.0);
            expectEquals (FaderLaw::dbToTravel (-500.0), 1.0);

            for (double t = 0.0; t <= 1.0; t += 0.01)
                expectWithinAbsoluteError (FaderLaw::dbToTravel (FaderLaw::travelToDb (t)), t, 1.0e-9);
        }

        beginTest ("Peak hold jumps up, holds 1.5 s, then falls 20 dB/s to the level");
        {
            PeakHold hold;
            expectEquals (hold.update (-6.0, 0.02), -6.0);
            expectEquals (hold.update (-30.0, 1.0), -6.0);
            expectEquals (hold.update (-30.0, 0.5), -6.0);            // 1.5 s held
            expectWithinAbsoluteError (hold.update (-30.0, 0.5), -16.0, 1.0e-9);
            expectWithinAbsoluteError (hold.update (-30.0, 1.0), -30.0, 1.0e-9);   // never below the level
            expectEquals (hold.update (-3.0, 0.02), -3.0);
            hold.reset();
            expectEquals (hold.get(), FaderLaw::floorDb);
        }

        beginTest ("Meter modes: peak passes through; RMS averages over 300 ms, LUFS over 400 ms");
        {
            MeterBallistics meter;
            expectEquals (meter.update (-6.0, 0.03), -6.0);

            meter.setMode (MeterMode::rms);
            double shown = 0;

            for (int i = 0; i < 10; ++i)      // a 0 dB step, fed for 300 ms
                shown = meter.update (0.0, 0.03);

            // One time constant: 1 - 1/e of the power, about -2 dB.
            expectWithinAbsoluteError (shown, 10.0 * std::log10 (1.0 - std::exp (-1.0)), 0.05);

            MeterBallistics loudness;
            loudness.setMode (MeterMode::lufs);
            double slower = 0;

            for (int i = 0; i < 10; ++i)
                slower = loudness.update (0.0, 0.03);

            expectLessThan (slower, shown);
        }
    }
};

static ControlTests controlTests;

} // namespace resamper::test
