#include "TestFixture.h"
#include "Commands/PluginCommands.h"
#include "Commands/ProjectCommands.h"
#include "Commands/TrackCommands.h"
#include "UI/Browser/Library.h"
#include "UI/Detail/DetailView.h"
#include "UI/Detail/PluginDeviceCard.h"
#include "UI/State/ShellState.h"

#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion;

namespace resamper::test
{

namespace
{
    /** A plug-in with six parameters and no editor, standing in for a scanned VST3. */
    struct FakePlugin : juce::AudioPluginInstance
    {
        static juce::PluginDescription description()
        {
            juce::PluginDescription d;
            d.name = "Pinboard";
            d.descriptiveName = "Pinboard";
            d.manufacturerName = "Resamper Tests";
            d.version = "1.2.0";
            d.pluginFormatName = "VST3";
            d.category = "Fx";
            d.fileOrIdentifier = "/Library/Audio/Plug-Ins/VST3/Pinboard.vst3";
            d.uniqueId = d.deprecatedUid = 0x50696e62;
            d.numInputChannels = d.numOutputChannels = 2;
            return d;
        }

        FakePlugin()
            : juce::AudioPluginInstance (BusesProperties().withInput ("In", juce::AudioChannelSet::stereo())
                                                          .withOutput ("Out", juce::AudioChannelSet::stereo()))
        {
            for (int i = 0; i < 6; ++i)
                juce::AudioProcessor::addParameter (new juce::AudioParameterFloat (juce::ParameterID { "p" + juce::String (i), 1 },
                                                             "Param " + juce::String (i + 1), 0.0f, 1.0f, 0.5f));
        }

        void fillInPluginDescription (juce::PluginDescription& d) const override   { d = description(); }
        const juce::String getName() const override                    { return "Pinboard"; }
        void prepareToPlay (double, int) override                     {}
        void releaseResources() override                              {}
        void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
        double getTailLengthSeconds() const override                  { return 0; }
        bool acceptsMidi() const override                             { return false; }
        bool producesMidi() const override                            { return false; }
        juce::AudioProcessorEditor* createEditor() override           { return nullptr; }
        bool hasEditor() const override                               { return false; }
        int getNumPrograms() override                                 { return 1; }
        int getCurrentProgram() override                              { return 0; }
        void setCurrentProgram (int) override                         {}
        const juce::String getProgramName (int) override              { return {}; }
        void changeProgramName (int, const juce::String&) override    {}
        void getStateInformation (juce::MemoryBlock&) override        {}
        void setStateInformation (const void*, int) override          {}
    };

    /** While alive, the engine knows FakePlugin as a scanned VST3 and can create it. */
    struct ScannedPlugin
    {
        explicit ScannedPlugin (Fixture& f) : manager (f.projects.getEdit().engine.getPluginManager())
        {
            previous = manager.createPluginInstance;
            manager.createPluginInstance = [fallback = previous] (const juce::PluginDescription& d, double rate, int block,
                                                                  juce::String& error) -> std::unique_ptr<juce::AudioPluginInstance>
            {
                if (d.fileOrIdentifier == FakePlugin::description().fileOrIdentifier)
                    return std::make_unique<FakePlugin>();

                return fallback (d, rate, block, error);
            };
            manager.knownPluginList.addType (FakePlugin::description());
        }

        ~ScannedPlugin()
        {
            manager.knownPluginList.removeType (FakePlugin::description());
            manager.createPluginInstance = previous;
        }

        te::PluginManager& manager;
        decltype (te::PluginManager::createPluginInstance) previous;
    };

    void findAll (juce::Component& root, const juce::String& id, std::vector<juce::Component*>& out)
    {
        for (auto* child : root.getChildren())
        {
            if (child->getComponentID() == id)
                out.push_back (child);

            findAll (*child, id, out);
        }
    }

    std::vector<juce::Component*> findAll (juce::Component& root, const juce::String& id)
    {
        std::vector<juce::Component*> out;
        findAll (root, id, out);
        return out;
    }

    juce::Component* findOne (juce::Component& root, const juce::String& id)
    {
        auto all = findAll (root, id);
        return all.size() == 1 ? all.front() : nullptr;
    }

    template <typename Type>
    Type* findType (juce::Component& root)
    {
        for (auto* child : root.getChildren())
        {
            if (auto* match = dynamic_cast<Type*> (child))
                return match;

            if (auto* match = findType<Type> (*child))
                return match;
        }

        return nullptr;
    }

    void click (juce::Component* c)
    {
        if (auto* button = dynamic_cast<juce::Button*> (c))
            button->triggerClick();

        juce::MessageManager::getInstance()->runDispatchLoopUntil (50);   // triggerClick is async
    }
}

/** PRD §9.2 (#66): native devices and plug-ins share the chain but follow
    different card contracts, told apart at a glance. */
struct DeviceCardTests : juce::UnitTest
{
    DeviceCardTests() : juce::UnitTest ("Device Cards", "Resamper") {}

    struct Cards : Fixture
    {
        Cards()
        {
            theme.load();
            invoke (cmd::trackAdd);
            invoke (cmd::trackSelect, { trackId() });
        }

        juce::String trackId() const   { return model.getTracks().front().id; }

        juce::String insert (const juce::String& type)
        {
            invoke (cmd::pluginInsert, { trackId(), type });
            auto chain = plugins.getChain (trackId(), PluginChain::device);
            return chain.empty() ? juce::String() : chain.back().id;
        }

        std::unique_ptr<DetailView> view()
        {
            auto v = std::make_unique<DetailView> (model, plugins, commands, theme, shell, uiState);
            v->setSize (1400, 240);
            return v;
        }

        juce::ValueTree uiState { "detail" };
        ShellState shell { juce::ValueTree ("shell") };
    };

    void runTest() override
    {
        const juce::String reverb (te::ReverbPlugin::xmlTypeName), pinboard (FakePlugin::description().fileOrIdentifier);

        beginTest ("A built-in gets the native contract, a scanned plug-in the plug-in contract");
        {
            Cards f;
            ScannedPlugin scanned (f);
            f.insert (reverb);
            f.insert (pinboard);
            expect (f.errors.isEmpty(), f.errors.joinIntoString ("; "));

            auto chain = f.plugins.getChain (f.trackId(), PluginChain::device);
            expectEquals ((int) chain.size(), 2);
            expect (! chain[0].external && chain[1].external);
            expectEquals (chain[1].manufacturer, juce::String ("Resamper Tests"));
            expectEquals (chain[1].version, juce::String ("1.2.0"));
            expect (! chain[1].sandboxed);

            auto view = f.view();
            expectEquals ((int) findAll (*view, "DeviceCard/Native").size(), 1);
            expectEquals ((int) findAll (*view, "DeviceCard/Plugin").size(), 1);

            // Never colour-only (§18): a plug-in names its vendor and format.
            auto* card = findOne (*view, "DeviceCard/Plugin");
            expect (card != nullptr && card->getDescription().contains ("Resamper Tests"));
            expect (card != nullptr && card->getDescription().contains ("VST3"));
            expect (card != nullptr && card->getWidth() == 214 && card->getHeight() == 164);
        }

        beginTest ("The native header runs power, name, preset, A/B, Mods, fold, expand, options; Mods waits for its drawer");
        {
            Cards f;
            f.insert (reverb);
            auto view = f.view();
            auto* card = findOne (*view, "DeviceCard/Native");
            expect (card != nullptr);

            if (card == nullptr)
                return;

            int lastX = -1;

            for (auto* id : { "power", "preset", "ab", "mods", "fold", "expand", "options" })
            {
                auto* part = findOne (*card, id);
                expect (part != nullptr, id);

                if (part == nullptr)
                    continue;

                expect (part->getX() > lastX, juce::String (id) + " is out of order");
                expect (part->getBottom() <= 28, juce::String (id) + " is outside the 28 px header");
                lastX = part->getX();
            }

            auto* mods = findOne (*card, "mods");
            expect (mods != nullptr && ! mods->isEnabled());
            expect (mods != nullptr && dynamic_cast<juce::Button*> (mods)->getButtonText().contains ("0"));
        }

        beginTest ("Mix and Out are the last zone of a native device");
        {
            Cards f;
            const auto id = f.insert (reverb);
            auto view = f.view();
            auto* card = findOne (*view, "DeviceCard/Native");
            expect (card != nullptr);

            if (card == nullptr)
                return;

            int lastControl = -1, firstOutput = std::numeric_limits<int>::max();

            for (auto& p : f.plugins.getParameters (id))
                if (auto* knob = findOne (*card, p.id); knob != nullptr && knob->isVisible())
                {
                    const auto output = p.name.containsIgnoreCase ("wet") || p.name.containsIgnoreCase ("dry");
                    (output ? firstOutput : lastControl) = output ? juce::jmin (firstOutput, knob->getX())
                                                                  : juce::jmax (lastControl, knob->getRight());
                }

            expect (lastControl > 0 && firstOutput < std::numeric_limits<int>::max(), "the reverb shows controls and outputs");
            expect (lastControl < firstOutput, "an output knob sits among the controls");
        }

        beginTest ("Folded is a 28 px strip; Expanded edits every parameter inline");
        {
            Cards f;
            const auto id = f.insert (reverb);
            auto view = f.view();
            auto* card = findOne (*view, "DeviceCard/Native");
            expect (card != nullptr);

            if (card == nullptr)
                return;

            const auto compactWidth = card->getWidth();
            click (findOne (*card, "fold"));
            card = findOne (*view, "DeviceCard/Native");
            expectEquals (card->getWidth(), 28);
            expect (findOne (*card, "mods") != nullptr && findOne (*card, "mods")->isVisible(), "the folded strip shows Mods");
            expect (! findOne (*card, "fold")->isVisible() && ! findOne (*card, "preset")->isVisible());

            // Clicking the strip unfolds it.
            card->mouseUp (mouseEvent (*card, { 14, 100 }, { 14, 100 }, false));
            card = findOne (*view, "DeviceCard/Native");
            expectEquals (card->getWidth(), compactWidth);

            click (findOne (*card, "expand"));
            card = findOne (*view, "DeviceCard/Native");
            expect (card->getWidth() > compactWidth);

            for (auto& p : f.plugins.getParameters (id))
                expect (findOne (*card, p.id) != nullptr && findOne (*card, p.id)->isVisible(), p.name + " isn't on the expanded card");
        }

        beginTest ("Pinning: at most 4, only on a plug-in, one undo step each");
        {
            Cards f;
            ScannedPlugin scanned (f);
            const auto native = f.insert (reverb);
            const auto id = f.insert (pinboard);
            auto parameters = f.plugins.getParameters (id);
            expect (parameters.size() >= 6, "the fake plug-in lists its parameters");

            for (int i = 0; i < 4; ++i)
                expect (f.invoke (cmd::pluginSetPinned, { id, parameters[(size_t) i].id, true }));

            expect (f.errors.isEmpty(), f.errors.joinIntoString ("; "));
            f.invoke (cmd::pluginSetPinned, { id, parameters[4].id, true });
            expect (f.errors.size() == 1 && f.errors[0].contains ("4"), f.errors.joinIntoString ("; "));
            expectEquals (f.plugins.getChain (f.trackId(), PluginChain::device).back().pinnedParameters.size(), 4);

            f.invoke (cmd::pluginSetPinned, { native, f.plugins.getParameters (native).front().id, true });
            expectEquals (f.errors.size(), 2);

            f.invoke (cmd::editUndo);
            expectEquals (f.plugins.getChain (f.trackId(), PluginChain::device).back().pinnedParameters.size(), 3);

            f.invoke (cmd::pluginSetPinned, { id, parameters[0].id, false });
            const auto pins = f.plugins.getChain (f.trackId(), PluginChain::device).back().pinnedParameters;
            expectEquals (pins.joinIntoString (","), parameters[1].id + "," + parameters[2].id);
        }

        beginTest ("Pins survive save and load");
        {
            Cards f;
            ScannedPlugin scanned (f);
            const auto id = f.insert (pinboard);
            auto parameters = f.plugins.getParameters (id);
            f.invoke (cmd::pluginSetPinned, { id, parameters[3].id, true });
            f.invoke (cmd::pluginSetPinned, { id, parameters[1].id, true });

            f.projectSaveLocation = f.scratchDir().getChildFile ("Pins");
            f.invoke (cmd::projectSaveAs);

            Fixture reopened;
            reopened.projectToOpen = f.projectSaveLocation;
            reopened.invoke (cmd::projectOpen);
            expect (reopened.errors.isEmpty(), reopened.errors.joinIntoString ("; "));

            auto chain = reopened.plugins.getChain (reopened.model.getTracks().front().id, PluginChain::device);
            expectEquals ((int) chain.size(), 1);
            expect (! chain.empty() && chain[0].external && ! chain[0].missing);
            expectEquals (chain.empty() ? juce::String() : chain[0].pinnedParameters.joinIntoString (","),
                          parameters[3].id + "," + parameters[1].id);
        }

        beginTest ("The plug-in card shows its pins and the window's state, and opens the window");
        {
            Cards f;
            ScannedPlugin scanned (f);
            const auto id = f.insert (pinboard);
            auto parameters = f.plugins.getParameters (id);
            f.invoke (cmd::pluginSetPinned, { id, parameters[0].id, true });
            f.invoke (cmd::pluginSetPinned, { id, parameters[2].id, true });

            auto view = f.view();
            juce::StringArray opened;
            view->onOpenEditor = [&] (const juce::String& plugin) { opened.add (plugin); };

            auto* card = findOne (*view, "DeviceCard/Plugin");
            expect (card != nullptr);

            if (card == nullptr)
                return;

            expectEquals ((int) findAll (*card, "pinned").size(), 2);
            expect (findOne (*card, "knob") == nullptr, "the host drew the plug-in's own controls");

            auto* open = dynamic_cast<juce::Button*> (findOne (*card, "openWindow"));
            expect (open != nullptr && open->getButtonText() == "Open plug-in window");
            click (open);
            expectEquals (opened.joinIntoString (","), id);

            view->setOpenEditor (id);
            expect (open != nullptr && open->getButtonText().startsWith ("Window open"));

            card->mouseDoubleClick (mouseEvent (*card, { 60, 10 }, { 60, 10 }, false));
            expectEquals (opened.size(), 2);
        }

        beginTest ("The pin button learns: each parameter touched in the plug-in's window is pinned, up to 4");
        {
            Cards f;
            ScannedPlugin scanned (f);
            const auto id = f.insert (pinboard);
            auto view = f.view();
            juce::StringArray opened;
            view->onOpenEditor = [&] (const juce::String& plugin) { opened.add (plugin); };

            auto* card = dynamic_cast<PluginDeviceCard*> (findOne (*view, "DeviceCard/Plugin"));
            auto* instance = [&]() -> juce::AudioPluginInstance*
            {
                for (auto* plugin : te::getAllPlugins (f.projects.getEdit(), false))
                    if (auto* external = dynamic_cast<te::ExternalPlugin*> (plugin))
                        return external->getAudioPluginInstance();

                return nullptr;
            }();
            expect (card != nullptr && instance != nullptr);

            if (card == nullptr || instance == nullptr)
                return;

            auto touch = [&] (int index)
            {
                auto* parameter = instance->getParameters()[index];
                parameter->beginChangeGesture();
                parameter->endChangeGesture();
                juce::MessageManager::getInstance()->runDispatchLoopUntil (50);   // the pin lands after the touch
            };

            auto pinnedNames = [&]
            {
                juce::StringArray names;
                const auto parameters = f.plugins.getParameters (id);
                const auto chain = f.plugins.getChain (f.trackId(), PluginChain::device);

                for (auto& pin : chain.back().pinnedParameters)
                    for (auto& p : parameters)
                        if (p.id == pin)
                            names.add (p.name);

                return names.joinIntoString (",");
            };

            touch (0);
            expectEquals (pinnedNames(), juce::String(), "a touch pinned without learning");

            click (findOne (*card, "pinLearn"));
            expect (card->isLearningPins());
            expectEquals (opened.joinIntoString (","), id, "learning opens the window to touch in");

            touch (3);
            touch (1);
            touch (3);
            expectEquals (pinnedNames(), juce::String ("Param 4,Param 2"));

            touch (5);
            touch (0);
            expectEquals (pinnedNames(), juce::String ("Param 4,Param 2,Param 6,Param 1"));
            card = dynamic_cast<PluginDeviceCard*> (findOne (*view, "DeviceCard/Plugin"));
            expect (card != nullptr && ! card->isLearningPins(), "learning goes on past 4 pins");

            touch (2);
            expect (f.errors.isEmpty(), f.errors.joinIntoString ("; "));

            // Clicking the pin again stops learning.
            f.invoke (cmd::pluginSetPinned, { id, f.plugins.getChain (f.trackId(), PluginChain::device).back().pinnedParameters[0], false });
            click (findOne (*card, "pinLearn"));
            expect (card->isLearningPins());
            click (findOne (*card, "pinLearn"));
            expect (! card->isLearningPins());
            touch (4);
            expectEquals (f.plugins.getChain (f.trackId(), PluginChain::device).back().pinnedParameters.size(), 3);
        }

        beginTest ("A missing plug-in offers Locate and Replace, never its window");
        {
            Cards f;
            f.projectSaveLocation = f.scratchDir().getChildFile ("Missing");

            {
                ScannedPlugin scanned (f);
                f.insert (pinboard);
                f.invoke (cmd::projectSaveAs);
            }

            Cards reopened;
            reopened.projectToOpen = f.projectSaveLocation;
            reopened.invoke (cmd::projectOpen);
            reopened.invoke (cmd::trackSelect, { reopened.trackId() });

            auto chain = reopened.plugins.getChain (reopened.trackId(), PluginChain::device);
            expect (chain.size() == 1 && chain[0].missing && chain[0].name == "Pinboard");

            auto view = reopened.view();
            juce::StringArray opened;
            view->onOpenEditor = [&] (const juce::String& plugin) { opened.add (plugin); };
            auto* card = findOne (*view, "DeviceCard/Plugin");
            expect (card != nullptr);

            if (card == nullptr)
                return;

            expect (findOne (*card, "locate") != nullptr && findOne (*card, "replace") != nullptr);
            expect (findOne (*card, "openWindow") == nullptr || ! findOne (*card, "openWindow")->isVisible());
            card->mouseDoubleClick (mouseEvent (*card, { 60, 10 }, { 60, 10 }, false));
            expect (opened.isEmpty());
        }

        beginTest ("The chain ends in a drop zone; a dropped plug-in opens its window");
        {
            Cards f;
            ScannedPlugin scanned (f);
            f.insert (reverb);
            auto view = f.view();
            juce::StringArray opened;
            view->onOpenEditor = [&] (const juce::String& plugin) { opened.add (plugin); };

            auto* zone = findOne (*view, "dropZone");
            auto* card = findOne (*view, "DeviceCard/Native");
            expect (zone != nullptr && card != nullptr && zone->getX() > card->getRight());

            auto* target = findType<juce::DragAndDropTarget> (*view);
            expect (target != nullptr);

            if (target == nullptr)
                return;

            LibraryItem item;
            item.name = "Pinboard";
            item.pluginPath = pinboard;
            target->itemDropped ({ dragDescription (item), nullptr, { 2000, 50 } });

            auto chain = f.plugins.getChain (f.trackId(), PluginChain::device);
            expectEquals ((int) chain.size(), 2);
            expectEquals (opened.joinIntoString (","), chain.back().id);

            // A native device opens no window.
            item.name = "Reverb";
            item.pluginPath = reverb;
            target->itemDropped ({ dragDescription (item), nullptr, { 2000, 50 } });
            expectEquals (opened.size(), 1);
        }
    }
};

static DeviceCardTests deviceCardTests;

} // namespace resamper::test
