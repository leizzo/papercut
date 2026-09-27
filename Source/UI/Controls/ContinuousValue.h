#pragma once

#include "ValueFormat.h"

namespace papercut
{

/** The value behind a knob, slider, fader or bar, and the PRD §16.2 rules for
    changing it. Every continuous control uses one, so they all behave alike:

    - dragging moves along the control's travel: 200 px is the full range,
      and Shift makes it ten times finer (or, for a field such as tempo, a
      fixed number of units per pixel);
    - reset (double-click, Alt+click) returns to the default;
    - a wheel notch or an arrow key steps by wheelStep in value units, or 1 %
      of the travel when wheelStep is 0;
    - typed text is parsed by the format, units and all, and clamped.

    onChange reports each change with whether it continues the gesture that
    changed the value last: within one drag, every change after the first does.
    That is how a whole gesture becomes one Engine Undo step. A change that
    leaves the value where it was is not reported.
*/
class ContinuousValue
{
public:
    struct Spec
    {
        double minimum = 0, maximum = 1, defaultValue = 0;
        ValueFormat format = ValueFormat::number (2);

        /** Value units per wheel notch or arrow key; 0 steps 1 % of the travel. */
        double wheelStep = 0;

        /** When non-zero, a drag moves this many value units per pixel (Shift:
            fineUnitsPerPixel) instead of covering the travel in 200 px. */
        double unitsPerPixel = 0, fineUnitsPerPixel = 0;

        /** Where a value sits along the control's travel (0..1) and back. Linear when empty. */
        std::function<double (double value)> toProportion;
        std::function<double (double proportion)> fromProportion;
    };

    /** Pixels of drag for the whole travel. */
    static constexpr float pixelsForFullRange = 200.0f;

    explicit ContinuousValue (Spec);

    const Spec& getSpec() const noexcept   { return spec; }

    double getValue() const noexcept       { return value; }
    juce::String getText() const           { return spec.format.format (value); }

    /** Sets the value from the model without reporting it (clamped). */
    void setValue (double);

    double getProportion() const;

    void beginDrag();

    /** Moves by pixels along the travel (positive = up / right = larger). */
    void dragBy (float pixels, bool fine);

    void endDrag();
    bool isDragging() const noexcept       { return dragging; }

    void reset();

    /** Steps by notches (wheel) or presses (arrow keys); fine divides by ten. */
    void step (int notches, bool fine = false);

    /** Sets the value as one user gesture (accessibility, a picker), reporting it. */
    void setByUser (double);

    /** Parses text and sets the value. Returns false, changing nothing, if it doesn't parse. */
    bool commitText (const juce::String&);

    std::function<void (double value, bool continuesGesture)> onChange;

private:
    Spec spec;
    double value = 0;
    double dragProportion = 0;   ///< position along the travel during a drag
    bool dragging = false, changedInDrag = false;

    double proportionOf (double v) const;
    double valueAt (double proportion) const;
    void change (double newValue, bool continuesGesture);
};

} // namespace papercut
