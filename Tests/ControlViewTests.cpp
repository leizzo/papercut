#include "TestFixture.h"
#include "UI/Controls/ContinuousControl.h"
#include "UI/Mixer/StripParts.h"

namespace resamper::test
{

namespace
{
    /** The ValueTag a drag on control shows, if one is showing. */
    juce::Component* visibleValueTag (juce::Component& control)
    {
        for (auto* child : control.getTopLevelComponent()->getChildren())
            if (dynamic_cast<ValueTag*> (child) != nullptr && child->isVisible())
                return child;

        return nullptr;
    }

    /** Presses control at downAt, then drags to each point, returning the tag's bounds after each move. */
    std::vector<juce::Rectangle<int>> tagBoundsWhileDragging (ContinuousControl& control, juce::Point<int> downAt,
                                                              std::initializer_list<juce::Point<int>> path)
    {
        std::vector<juce::Rectangle<int>> bounds;
        auto last = downAt;
        control.mouseDown (mouseEvent (control, downAt, downAt, false));

        for (auto p : path)
        {
            last = p;
            control.mouseDrag (mouseEvent (control, p, downAt, true));
            auto* tag = visibleValueTag (control);
            bounds.push_back (tag != nullptr ? tag->getBounds() : juce::Rectangle<int>());
        }

        control.mouseUp (mouseEvent (control, last, downAt, path.size() > 0));
        return bounds;
    }

    struct ProbeKnob : Knob
    {
        using Knob::Knob;
        using Knob::getFocusBounds;
        using Knob::getFocusRadius;
    };

    struct ProbeFader : Fader
    {
        using Fader::Fader;
        using Fader::getFocusBounds;
    };

    /** The y of db on fader's travel, by the fader's dB law. */
    int yForDb (const Fader& fader, double db)
    {
        const auto travel = fader.getTravelBounds().toFloat();
        return juce::roundToInt (travel.getY() + (float) FaderLaw::dbToTravel (db) * travel.getHeight());
    }
}

/** Knob and Fader as the user handles them (PRD §16.2, #18): focus, the drag ValueTag, clicks. */
struct ControlViewTests : juce::UnitTest
{
    ControlViewTests() : juce::UnitTest ("Control Views", "Resamper") {}

    void runTest() override
    {
        Fixture f;
        f.theme.load();

        ContinuousValue::Spec panSpec;
        panSpec.minimum = -1.0;
        panSpec.maximum = 1.0;
        panSpec.format = ValueFormat::pan();

        beginTest ("A Knob's focus ring is a whole circle inside the control, readout under or beside (#92)");
        {
            for (auto beside : { false, true })
            {
                ProbeKnob knob (f.theme, panSpec, "Pan");
                knob.setReadoutBeside (beside);
                knob.setBounds (beside ? juce::Rectangle<int> (0, 0, 80, 30) : juce::Rectangle<int> (0, 0, 60, 60));

                const auto ring = knob.getFocusBounds();
                expect (knob.getLocalBounds().toFloat().contains (ring), "ring " + ring.toString() + " leaves the knob");
                expectEquals (ring.getWidth(), ring.getHeight());
                expectEquals (knob.getFocusRadius(), ring.getWidth() / 2.0f);
            }
        }

        beginTest ("A Knob's ValueTag stays put beside the dial while the pointer moves (#93)");
        {
            juce::Component window;
            window.setSize (400, 400);
            ProbeKnob knob (f.theme, panSpec, "Pan");
            window.addAndMakeVisible (knob);
            knob.setBounds (0, 0, 60, 60);

            const auto bounds = tagBoundsWhileDragging (knob, { 30, 15 }, { { 30, 5 }, { 45, -10 }, { 10, 25 } });
            const auto dial = knob.getFocusBounds();

            for (auto& b : bounds)
            {
                expect (! b.isEmpty(), "no tag while dragging");
                expectEquals (b.getX(), (int) dial.getRight() + 6);
                expectEquals (b.getCentreY(), juce::roundToInt (dial.getCentreY()));
            }

            expect (visibleValueTag (knob) == nullptr, "the tag hides on mouse-up");
        }

        beginTest ("A Fader's ValueTag sits beside the cap, not the pointer (#94)");
        {
            juce::Component window;
            window.setSize (400, 400);
            ProbeFader fader (f.theme);
            window.addAndMakeVisible (fader);
            fader.setBounds (0, 0, 60, 300);
            const auto capCentre = fader.getFocusBounds().getCentre().toInt();
            std::vector<juce::Rectangle<float>> caps;

            fader.mouseDown (mouseEvent (fader, capCentre, capCentre, false));

            for (auto p : { capCentre.translated (20, -30), capCentre.translated (-15, -60), capCentre.translated (5, 40) })
            {
                fader.mouseDrag (mouseEvent (fader, p, capCentre, true));
                const auto cap = fader.getFocusBounds();
                auto* tag = visibleValueTag (fader);
                expect (tag != nullptr, "no tag while dragging");

                if (tag != nullptr)
                {
                    expectEquals (tag->getX(), (int) cap.getRight() + 6);
                    expectEquals (tag->getBounds().getCentreY(), juce::roundToInt (cap.getCentreY()));
                }
            }

            fader.mouseUp (mouseEvent (fader, capCentre, capCentre, true));
        }

        beginTest ("Clicking a Fader away from the cap jumps it to that dB, as one undo step (#95)");
        {
            std::vector<std::pair<double, bool>> changes;
            Fader fader (f.theme);
            fader.setBounds (0, 0, 60, 300);
            fader.onChange = [&] (double v, bool continues) { changes.emplace_back (v, continues); };

            // On the scale, at the -12 dB mark.
            const juce::Point<int> mark (10, yForDb (fader, -12.0));
            fader.mouseDown (mouseEvent (fader, mark, mark, false));
            fader.mouseUp (mouseEvent (fader, mark, mark, false));

            expectEquals ((int) changes.size(), 1);
            expectWithinAbsoluteError (fader.getValue(), -12.0, 0.2);
            expect (changes.size() == 1 && ! changes[0].second, "the jump starts its own undo step");

            // Then a drag from a jump continues the same gesture.
            changes.clear();
            const juce::Point<int> track (40, yForDb (fader, -24.0));
            fader.mouseDown (mouseEvent (fader, track, track, false));
            fader.mouseDrag (mouseEvent (fader, track.translated (0, -20), track, true));
            fader.mouseUp (mouseEvent (fader, track.translated (0, -20), track, true));

            expectEquals ((int) changes.size(), 2);
            expect (changes.size() == 2 && ! changes[0].second && changes[1].second, "press and drag are one gesture");
            expectGreaterThan (fader.getValue(), -24.0);

            // Pressing the cap itself is still a relative drag: no jump.
            changes.clear();
            const auto before = fader.getValue();
            const juce::Point<int> cap (40, yForDb (fader, before) + 10);
            fader.mouseDown (mouseEvent (fader, cap, cap, false));
            fader.mouseUp (mouseEvent (fader, cap, cap, false));
            expect (changes.empty(), "a click on the cap moves nothing");
            expectEquals (fader.getValue(), before);
        }
    }
};

static ControlViewTests controlViewTests;

} // namespace resamper::test
