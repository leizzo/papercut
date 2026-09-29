#pragma once

#include "ComponentFactory.h"

namespace resamper
{

class CommandRegistry;
class ThemeManager;

/** A themed text label. Layouts create it with "label"; hosts may update its
    text by component ID. */
class TextLabel : public juce::Component
{
public:
    TextLabel (ThemeManager&, juce::String text, bool muted, juce::Justification);

    void setText (const juce::String&);
    const juce::String& getText() const noexcept   { return text; }

    void paint (juce::Graphics&) override;

private:
    ThemeManager& themeManager;
    juce::String text;
    bool muted;
    juce::Justification justification;
};

/** Registers JSON grammar v1:

    vstack / hstack   "children": [...], optional "gap", "padding" (px)
    panel             a themed box
    button            "text", "command" (a registered Command ID)
    label             "text", optional "style": "muted", "justify": "left" | "centred" | "right"

    Inside a stack, a child sizes itself along the stack's axis with either
    "size" (fixed px) or "flex" (share of the remaining space; the default is 1).
*/
void registerPrimitives (ComponentFactory&, CommandRegistry&, ThemeManager&);

} // namespace resamper
