#include "TestFixture.h"

#include "UI/Controls/ContinuousValue.h"
#include "UI/Controls/ValueFormat.h"

namespace papercut::test
{

/** The continuous-control model (PRD §16.2) and value text in and out. */
struct ControlTests : juce::UnitTest
{
    ControlTests() : juce::UnitTest ("Controls", "Papercut") {}

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
    }
};

static ControlTests controlTests;

} // namespace papercut::test
