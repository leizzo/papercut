#pragma once

#include "Icons.h"
#include "UI/Theme/Interaction.h"

#include <optional>

namespace papercut
{

/** Base of the design-system buttons (PRD §15.4 Controls). Paints nothing
    itself: it keeps the §15.5 states consistent — disabled opacity and cursor,
    and the keyboard focus ring, which is never removed. State is the juce::Button
    toggle state; variants are constructor arguments (§15.6 rule 5). */
class ThemedButton : public juce::Button
{
public:
    ThemedButton (ThemeManager&, const juce::String& name);

    void enablementChanged() override;
    void focusGained (FocusChangeType) override   { repaint(); }
    void focusLost (FocusChangeType) override     { repaint(); }

protected:
    ThemeManager& themeManager;

    ControlState state (bool down) const   { return ControlState::of (*this, getToggleState(), down); }
    void paintFocus (juce::Graphics&, float cornerRadius);
};

/** `Button/Primary · Secondary · Outline · Ghost`: a text button, optionally with an icon. */
class Button : public ThemedButton
{
public:
    enum class Variant { primary, secondary, outline, ghost };

    Button (ThemeManager&, const juce::String& text, Variant = Variant::secondary, std::optional<Icon> = {});

    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

    /** Width that fits the label and icon at the design's padding. */
    int getIdealWidth() const;

private:
    Variant variant;
    std::optional<Icon> icon;
};

/** `IconButton/Transport` (34 square, radius-xl), `IconButton/Transport Active`
    (toggle on: filled with the active colour) and `IconButton/Small` (24, radius 5). */
class IconButton : public ThemedButton
{
public:
    enum class Kind { transport, small };

    IconButton (ThemeManager&, const juce::String& name, Icon, Kind = Kind::transport);

    void setIcon (Icon i)                          { icon = i; repaint(); }

    /** The icon's colour when off (e.g. the red Record glyph); text-secondary when unset. */
    void setIconColour (std::optional<juce::Colour> c)   { iconColour = c; repaint(); }

    /** The fill when on; accent when unset. */
    void setActiveColour (std::optional<juce::Colour> c) { activeColour = c; repaint(); }

    /** Draws an outline in the active colour instead of a fill when on (Automation Arm). */
    void setOutlineWhenActive (bool b)             { outlineWhenActive = b; repaint(); }

    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

private:
    Icon icon;
    Kind kind;
    std::optional<juce::Colour> iconColour, activeColour;
    bool outlineWhenActive = false;
};

/** `Toggle/On · Off`: a 26 x 14 switch. Clicking flips it. */
class Toggle : public ThemedButton
{
public:
    Toggle (ThemeManager&, const juce::String& name);
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
};

/** `Chip/Toggle On · Off`: a toggling pill with an optional icon. */
class Chip : public ThemedButton
{
public:
    Chip (ThemeManager&, const juce::String& text, std::optional<Icon> = {});
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
    int getIdealWidth() const;

private:
    std::optional<Icon> icon;
};

/** `TrackBtn/Off · Mute On · Solo On · Arm On` plus the header's Auto button.
    The letter or glyph always shows, so state is never colour-only (§18). */
class TrackButton : public ThemedButton
{
public:
    enum class Kind { mute, solo, arm, automation };

    TrackButton (ThemeManager&, Kind);
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

private:
    Kind kind;
};

/** `Segmented/Item · Item Active` in a well, or `Tab/View · Tab/View Active`
    (the view switcher). One item is selected; clicking another, or the arrow
    keys when focused, selects it and calls onChange. */
class Segmented : public juce::Component
{
public:
    enum class Style { segmented, tabs };

    Segmented (ThemeManager&, juce::StringArray items, Style = Style::segmented);

    void setSelectedIndex (int, juce::NotificationType = juce::sendNotification);
    int getSelectedIndex() const noexcept   { return selected; }
    const juce::StringArray& getItems() const noexcept   { return items; }

    std::function<void (int index)> onChange;

    /** Width that fits every item at the design's padding. */
    int getIdealWidth() const;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    void focusGained (FocusChangeType) override   { repaint(); }
    void focusLost (FocusChangeType) override     { repaint(); }
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

private:
    ThemeManager& themeManager;
    juce::StringArray items;
    Style style;
    int selected = 0, hovered = -1;

    juce::Rectangle<float> itemBounds (int index) const;
    int itemAt (juce::Point<int>) const;
    const TypeStyle& textStyle() const;
};

} // namespace papercut
