#include "Primitives.h"
#include "Commands/CommandRegistry.h"
#include "UI/Theme/ThemeManager.h"

namespace resamper
{

namespace
{
    int readPixels (const juce::var& node, const char* field)
    {
        auto v = node[field];

        if (v.isVoid())
            return 0;

        if (! (v.isInt() || v.isInt64() || v.isDouble()) || (double) v < 0)
            throw LayoutError ("\"" + juce::String (field) + "\" must be a non-negative number (id \"" + node["id"].toString() + "\")");

        return (int) v;
    }

    //==============================================================================
    class StackComponent : public juce::Component
    {
    public:
        StackComponent (bool isHorizontal, int gapPx, int paddingPx)
            : horizontal (isHorizontal), gap (gapPx), padding (paddingPx) {}

        void addItem (std::unique_ptr<juce::Component> c, int fixedSize, float flex)
        {
            addAndMakeVisible (*c);
            items.push_back ({ std::move (c), fixedSize, flex });
        }

        void resized() override
        {
            juce::FlexBox box;
            box.flexDirection = horizontal ? juce::FlexBox::Direction::row : juce::FlexBox::Direction::column;
            box.alignItems = juce::FlexBox::AlignItems::stretch;

            for (size_t i = 0; i < items.size(); ++i)
            {
                auto& item = items[i];
                juce::FlexItem f (*item.component);

                if (item.fixedSize > 0)
                    f = horizontal ? f.withWidth ((float) item.fixedSize) : f.withHeight ((float) item.fixedSize);
                else
                    f = f.withFlex (item.flex);

                if (i > 0)
                    f = horizontal ? f.withMargin ({ 0, 0, 0, (float) gap }) : f.withMargin ({ (float) gap, 0, 0, 0 });

                box.items.add (f);
            }

            box.performLayout (getLocalBounds().reduced (padding));
        }

    private:
        struct Item
        {
            std::unique_ptr<juce::Component> component;
            int fixedSize;
            float flex;
        };

        const bool horizontal;
        const int gap, padding;
        std::vector<Item> items;
    };

    //==============================================================================
    class PanelComponent : public juce::Component
    {
    public:
        explicit PanelComponent (ThemeManager& tm) : themeManager (tm) {}

        void paint (juce::Graphics& g) override
        {
            auto& t = themeManager.getTheme();
            g.setColour (t.panel);
            g.fillRoundedRectangle (getLocalBounds().toFloat(), t.cornerRadius);
        }

    private:
        ThemeManager& themeManager;
    };

    juce::Justification readJustification (const juce::var& node)
    {
        auto j = node["justify"].toString();

        if (j.isEmpty() || j == "left")   return juce::Justification::centredLeft;
        if (j == "centred")               return juce::Justification::centred;
        if (j == "right")                 return juce::Justification::centredRight;

        throw LayoutError ("\"justify\" must be left, centred or right (id \"" + node["id"].toString() + "\")");
    }

    ComponentFactory::Creator stackCreator (bool horizontal)
    {
        return [horizontal] (const juce::var& node, ComponentFactory& factory) -> std::unique_ptr<juce::Component>
        {
            auto stack = std::make_unique<StackComponent> (horizontal, readPixels (node, "gap"), readPixels (node, "padding"));

            if (auto* children = node["children"].getArray())
            {
                for (auto& child : *children)
                {
                    const auto size = readPixels (child, "size");
                    const auto flex = child.hasProperty ("flex") ? (float) child["flex"] : 1.0f;

                    if (size > 0 && child.hasProperty ("flex"))
                        throw LayoutError ("Use either \"size\" or \"flex\", not both (id \"" + child["id"].toString() + "\")");

                    stack->addItem (factory.create (child), size, flex);
                }
            }
            else if (node.hasProperty ("children"))
            {
                throw LayoutError ("\"children\" must be an array (id \"" + node["id"].toString() + "\")");
            }

            return stack;
        };
    }
}

//==============================================================================
TextLabel::TextLabel (ThemeManager& tm, juce::String t, bool m, juce::Justification j)
    : themeManager (tm), text (std::move (t)), muted (m), justification (j)
{
    setInterceptsMouseClicks (false, false);
}

void TextLabel::setText (const juce::String& newText)
{
    if (newText != text)
    {
        text = newText;
        repaint();
    }
}

void TextLabel::paint (juce::Graphics& g)
{
    auto& t = themeManager.getTheme();
    g.setColour (muted ? t.mutedText : t.text);
    g.setFont (themeManager.getFont());
    g.drawFittedText (text, getLocalBounds().reduced (themeManager.getMetrics().textPadding, 0), justification, 1);
}

//==============================================================================
void registerPrimitives (ComponentFactory& factory, CommandRegistry& commands, ThemeManager& themeManager)
{
    factory.registerType ("vstack", stackCreator (false));
    factory.registerType ("hstack", stackCreator (true));

    factory.registerType ("panel", [&themeManager] (const juce::var&, ComponentFactory&) -> std::unique_ptr<juce::Component>
    {
        return std::make_unique<PanelComponent> (themeManager);
    });

    factory.registerType ("label", [&themeManager] (const juce::var& node, ComponentFactory&) -> std::unique_ptr<juce::Component>
    {
        auto style = node["style"].toString();

        if (style.isNotEmpty() && style != "muted")
            throw LayoutError ("Unknown label style \"" + style + "\"");

        return std::make_unique<TextLabel> (themeManager, node["text"].toString(), style == "muted", readJustification (node));
    });

    factory.registerType ("button", [&commands] (const juce::var& node, ComponentFactory&) -> std::unique_ptr<juce::Component>
    {
        auto commandId = requireString (node, "command");

        if (! commands.contains (commandId))
            throw LayoutError ("Button \"" + node["id"].toString() + "\" refers to unknown Command \"" + commandId + "\"");

        // A button invokes its Command without args, so it may only name one that takes none.
        if (commands.find (commandId)->getArgsType() != typeid (void))
            throw LayoutError ("Button \"" + node["id"].toString() + "\" refers to Command \"" + commandId
                               + "\", which needs args a button can't give");

        auto button = std::make_unique<juce::TextButton> (requireString (node, "text"));
        button->setTooltip (commands.find (commandId)->getName());

        // Keyboard shortcuts (e.g. space for play) must reach the Command table,
        // not trigger whichever button was clicked last.
        button->setWantsKeyboardFocus (false);
        button->onClick = [&commands, commandId] { commands.invokeById (commandId); };
        return button;
    });
}

} // namespace resamper
