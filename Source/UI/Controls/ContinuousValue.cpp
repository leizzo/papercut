#include "ContinuousValue.h"

namespace resamper
{

ContinuousValue::ContinuousValue (Spec s) : spec (std::move (s))
{
    value = juce::jlimit (spec.minimum, spec.maximum, spec.defaultValue);
}

void ContinuousValue::setValue (double v)
{
    value = juce::jlimit (spec.minimum, spec.maximum, v);
}

double ContinuousValue::proportionOf (double v) const
{
    if (spec.toProportion)
        return juce::jlimit (0.0, 1.0, spec.toProportion (v));

    return spec.maximum > spec.minimum ? (v - spec.minimum) / (spec.maximum - spec.minimum) : 0.0;
}

double ContinuousValue::valueAt (double proportion) const
{
    proportion = juce::jlimit (0.0, 1.0, proportion);

    if (spec.fromProportion)
        return juce::jlimit (spec.minimum, spec.maximum, spec.fromProportion (proportion));

    return spec.minimum + proportion * (spec.maximum - spec.minimum);
}

double ContinuousValue::getProportion() const
{
    return proportionOf (value);
}

void ContinuousValue::beginDrag()
{
    dragging = true;
    changedInDrag = false;
    dragProportion = proportionOf (value);
}

void ContinuousValue::dragBy (float pixels, bool fine)
{
    if (! dragging)
        beginDrag();

    if (spec.unitsPerPixel > 0)
    {
        const auto perPixel = fine && spec.fineUnitsPerPixel > 0 ? spec.fineUnitsPerPixel : spec.unitsPerPixel;
        change (juce::jlimit (spec.minimum, spec.maximum, value + pixels * perPixel), changedInDrag);
        return;
    }

    // Clamped at each step, so dragging back off an end responds at once.
    dragProportion = juce::jlimit (0.0, 1.0, dragProportion + (double) pixels * (fine ? 0.1 : 1.0) / (double) pixelsForFullRange);
    change (valueAt (dragProportion), changedInDrag);
}

void ContinuousValue::endDrag()
{
    dragging = false;
}

void ContinuousValue::reset()
{
    change (juce::jlimit (spec.minimum, spec.maximum, spec.defaultValue), false);
}

void ContinuousValue::step (int notches, bool fine)
{
    if (notches == 0)
        return;

    const auto scale = fine ? 0.1 : 1.0;

    if (spec.wheelStep > 0)
        change (juce::jlimit (spec.minimum, spec.maximum, value + notches * spec.wheelStep * scale), false);
    else
        change (valueAt (proportionOf (value) + notches * 0.01 * scale), false);
}

void ContinuousValue::setByUser (double v)
{
    change (juce::jlimit (spec.minimum, spec.maximum, v), false);
}

bool ContinuousValue::commitText (const juce::String& text)
{
    double parsed = 0;

    if (! spec.format.parse || ! spec.format.parse (text, parsed))
        return false;

    change (juce::jlimit (spec.minimum, spec.maximum, parsed), false);
    return true;
}

void ContinuousValue::change (double newValue, bool continuesGesture)
{
    if (juce::exactlyEqual (newValue, value))
        return;

    value = newValue;

    if (dragging)
        changedInDrag = true;

    if (onChange)
        onChange (value, continuesGesture);
}

} // namespace resamper
