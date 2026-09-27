#include "TestFixture.h"

#include "UI/Layout/LayoutSource.h"
#include "UI/Layout/Primitives.h"
#include "UI/Theme/ThemeManager.h"

namespace papercut::test
{

/** The declarative layer's failure behaviour: broken layouts must fail loudly.
    (Rendering correctness is deliberately not covered.) */
struct ComponentFactoryTests : juce::UnitTest
{
    ComponentFactoryTests() : juce::UnitTest ("ComponentFactory", "Papercut") {}

    struct Setup
    {
        Setup()
        {
            commands.add (std::make_unique<Probe> (invocations));
            registerPrimitives (factory, commands, themes);
        }

        struct Probe : Command
        {
            explicit Probe (int& n) : Command ("test.probe", "Probe"), count (n) {}
            void execute() override   { ++count; }
            int& count;
        };

        int invocations = 0;
        LayoutSource source;
        ThemeManager themes { source, "themes/dark.json" };
        CommandRegistry commands;
        ComponentFactory factory;
    };

    template <typename Fn>
    void expectLayoutError (Fn&& fn, const juce::String& messageFragment)
    {
        try
        {
            fn();
            expect (false, "expected a LayoutError mentioning: " + messageFragment);
        }
        catch (const LayoutError& e)
        {
            expect (juce::String (e.what()).contains (messageFragment), e.what());
        }
    }

    void runTest() override
    {
        beginTest ("An unknown type fails loudly, naming the type");
        {
            Setup s;
            expectLayoutError ([&] { s.factory.createFromJson (R"({ "type": "knob", "id": "volume" })"); }, "knob");
        }

        beginTest ("An unknown type deep in a stack fails the whole layout");
        {
            Setup s;
            expectLayoutError ([&] { s.factory.createFromJson (R"({ "type": "hstack", "children": [
                                         { "type": "label", "text": "ok" },
                                         { "type": "vstack", "children": [ { "type": "slider" } ] } ] })"); },
                               "slider");
        }

        beginTest ("A button naming an unregistered Command fails loudly");
        {
            Setup s;
            expectLayoutError ([&] { s.factory.createFromJson (R"({ "type": "button", "id": "b", "text": "Go", "command": "no.such" })"); },
                               "no.such");
        }

        beginTest ("Malformed JSON fails loudly");
        {
            Setup s;
            expectLayoutError ([&] { s.factory.createFromJson ("{ \"type\": "); }, "JSON");
        }

        beginTest ("A stack child may not set both size and flex");
        {
            Setup s;
            expectLayoutError ([&] { s.factory.createFromJson (R"({ "type": "hstack", "children": [
                                         { "type": "panel", "id": "p", "size": 10, "flex": 1 } ] })"); },
                               "either");
        }

        beginTest ("A valid layout builds components with their IDs, and buttons invoke their Command");
        {
            Setup s;
            auto root = s.factory.createFromJson (R"({ "type": "vstack", "id": "root", "children": [
                                                      { "type": "button", "id": "go", "text": "Go", "command": "test.probe", "size": 30 },
                                                      { "type": "label", "id": "status", "text": "Ready", "style": "muted" } ] })");

            expectEquals (root->getComponentID(), juce::String ("root"));
            expectEquals (root->getNumChildComponents(), 2);

            auto* button = dynamic_cast<juce::Button*> (root->getChildComponent (0));
            expect (button != nullptr && button->getComponentID() == "go");

            if (button != nullptr)
                button->triggerClick();

            juce::MessageManager::getInstance()->runDispatchLoopUntil (50);   // triggerClick is async
            expectEquals (s.invocations, 1);

            auto* label = dynamic_cast<TextLabel*> (root->getChildComponent (1));
            expect (label != nullptr && label->getText() == "Ready");
        }
    }
};

static ComponentFactoryTests componentFactoryTests;

} // namespace papercut::test
