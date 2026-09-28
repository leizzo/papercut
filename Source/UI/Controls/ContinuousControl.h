#pragma once

#include "ContinuousValue.h"
#include "UI/Theme/Interaction.h"

#include <optional>

namespace papercut
{

/** The accent pill that follows the pointer while a value is dragged (`ValueTag`):
    an optional position and the value, both mono. It floats over the top-level
    component and ignores the mouse. */
class ValueTag : public juce::Component
{
public:
    explicit ValueTag (ThemeManager&);

    /** Shows the tag beside screenPosition, over owner's top-level component. */
    void show (juce::Component& owner, juce::Point<int> screenPosition,
               const juce::String& value, const juce::String& position = {});
    void hide();

    void paint (juce::Graphics&) override;

private:
    ThemeManager& themeManager;
    juce::String value, position;
};

/** Base of every continuous control (knob, slider, fader, bar, value field),
    carrying the one PRD §16.2 interaction model (see ContinuousValue):

    - drag along the axis; Shift is fine; a ValueTag follows the pointer;
    - double-click (or Alt+click) resets to the default;
    - the wheel steps;
    - clicking the readout opens inline entry: Enter commits, Esc cancels;
    - focusable, and the arrow keys step (Shift: fine).

    onChange gets every change with continuesGesture set for all but the first
    change of a drag, so a gesture is one Engine Undo step. setValue() (from the
    model) is ignored while the user is dragging or typing.
*/
class ContinuousControl : public juce::Component,
                          public juce::SettableTooltipClient
{
public:
    enum class Axis { vertical, horizontal };

    ContinuousControl (ThemeManager&, ContinuousValue::Spec, Axis);
    ~ContinuousControl() override;

    std::function<void (double value, bool continuesGesture)> onChange;

    /** The value from the model. Ignored mid-gesture. */
    void setValue (double);
    double getValue() const noexcept                { return model.getValue(); }
    const ContinuousValue& getModel() const noexcept { return model; }

    /** Whether a double-click resets (true) or opens text entry (a value field). */
    void setDoubleClickEdits (bool b)               { doubleClickEdits = b; }

    /** Opens the inline text entry over the readout. */
    void showTextEntry();

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseEnter (const juce::MouseEvent&) override   { repaint(); }
    void mouseExit (const juce::MouseEvent&) override    { repaint(); }
    bool keyPressed (const juce::KeyPress&) override;
    void focusGained (FocusChangeType) override          { repaint(); }
    void focusLost (FocusChangeType) override            { repaint(); }
    void enablementChanged() override;
    void paintOverChildren (juce::Graphics&) override;
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

    /** Name and current value with its unit (PRD §16.6). */
    juce::String getTooltip() override;

protected:
    ThemeManager& themeManager;

    /** Where the value text is drawn; a click there opens text entry. Empty: nowhere. */
    virtual juce::Rectangle<int> getReadoutBounds() const   { return {}; }

    /** The shape of the focus ring. */
    virtual juce::Rectangle<float> getFocusBounds() const   { return getLocalBounds().toFloat(); }
    virtual float getFocusRadius() const                    { return themeManager.getTheme().radiusMd; }

    bool isEditingText() const noexcept                     { return editor != nullptr; }

private:
    ContinuousValue model;
    Axis axis;
    ValueTag tag;
    std::unique_ptr<juce::TextEditor> editor;
    juce::Point<float> lastDragPosition;
    float wheelAccumulator = 0;
    bool doubleClickEdits = false, pressedOnReadout = false, resetOnPress = false;

    void closeTextEntry (bool commit);
    void showTag (const juce::MouseEvent&);
};

/** `Knob` (arc from the start) and `Knob/Bipolar` (arc from 12 o'clock): a dial
    with a caption label and a mono value under it. Vertical drag. */
class Knob : public ContinuousControl
{
public:
    Knob (ThemeManager&, ContinuousValue::Spec, juce::String label = {}, bool bipolar = false);

    /** Greys the arc (a bypassed device). */
    void setDimmed (bool);

    /** The arc's colour; accent unless set (a DeviceCard's knobs take the device colour). */
    void setArcColour (std::optional<juce::Colour>);

    /** Diameter of the dial; the rest of the height holds the label and value. */
    void setDialSize (int);

    /** Label and value to the right of the dial, left-aligned (a strip's pan row), not under it. */
    void setReadoutBeside (bool);

    void paint (juce::Graphics&) override;

protected:
    juce::Rectangle<int> getReadoutBounds() const override;
    juce::Rectangle<float> getFocusBounds() const override;
    float getFocusRadius() const override;

private:
    juce::String label;
    bool bipolar, dimmed = false;
    std::optional<juce::Colour> arcColour;
    bool readoutBeside = false;
    int dialSize = 30;

    juce::Rectangle<float> dialBounds() const;
};

/** `Slider`: a 4 px track, an accent-dim fill (from the centre when bipolar)
    and a 12 px thumb. Horizontal drag. */
class Slider : public ContinuousControl
{
public:
    Slider (ThemeManager&, ContinuousValue::Spec, bool bipolar = false);
    void paint (juce::Graphics&) override;

private:
    bool bipolar;
};

/** `ValueField` (a mono value in a well) and `ValueField/Stacked` (a caption
    over a larger value). Double-click types a value; a spec with unitsPerPixel
    also drags vertically (the tempo field). */
class ValueField : public ContinuousControl
{
public:
    ValueField (ThemeManager&, ContinuousValue::Spec, juce::String caption = {});

    /** The value's type style (default fs-body; numbers are drawn mono). */
    void setValueStyle (const TypeStyle& s)   { valueStyle = s; repaint(); }

    /** A dim unit after the value, e.g. "BPM". */
    void setSuffix (juce::String s)           { suffix = std::move (s); repaint(); }

    /** A top-bar field: bg-elevated with a border, radius-lg, instead of a well. */
    void setRaised (bool b)                   { raised = b; repaint(); }

    void paint (juce::Graphics&) override;

protected:
    juce::Rectangle<int> getReadoutBounds() const override   { return getLocalBounds(); }
    float getFocusRadius() const override;

private:
    juce::String caption, suffix;
    std::optional<TypeStyle> valueStyle;
    bool raised = false;
};

} // namespace papercut
