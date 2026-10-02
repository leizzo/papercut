#include "ComponentSearch.h"
#include "FakePlugin.h"
#include "TestFixture.h"
#include "Commands/PluginCommands.h"
#include "Commands/ProjectCommands.h"
#include "Commands/TrackCommands.h"
#include "UI/Browser/Library.h"
#include "UI/Detail/DetailView.h"
#include "UI/Detail/PluginDeviceCard.h"
#include "UI/MainWindow/MainComponent.h"
#include "UI/State/ShellState.h"

#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion;

namespace resamper::test
{

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

            view->setOpenWindows ({ id });
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

        beginTest ("A native device's size is saved with the device, not as an undo step");
        {
            Cards f;
            const auto id = f.insert (reverb);
            auto view = f.view();
            click (findOne (*findOne (*view, "DeviceCard/Native"), "fold"));
            expect (f.plugins.getChain (f.trackId(), PluginChain::device).front().size == DeviceSize::folded);

            // One undo takes back the insert itself: folding was no step of its own.
            f.invoke (cmd::editUndo);
            expect (f.plugins.getChain (f.trackId(), PluginChain::device).empty(), "folding made an undo step");
            f.invoke (cmd::editRedo);

            f.invoke (cmd::pluginSetSize, { id, DeviceSize::expanded });
            f.projectSaveLocation = f.scratchDir().getChildFile ("Sizes");
            f.invoke (cmd::projectSaveAs);

            Fixture reopened;
            reopened.projectToOpen = f.projectSaveLocation;
            reopened.invoke (cmd::projectOpen);
            auto chain = reopened.plugins.getChain (reopened.model.getTracks().front().id, PluginChain::device);
            expect (chain.size() == 1 && chain[0].size == DeviceSize::expanded);
        }

        beginTest ("A device declares its Output parameters; an instrument's effect mixes aren't its output");
        {
            Cards f;
            f.invoke (cmd::trackAddMidi);
            const auto midi = f.model.getTracks()[1].id;
            const auto compressor = f.insert (te::CompressorPlugin::xmlTypeName);
            f.invoke (cmd::pluginInsert, { midi, te::FourOscPlugin::xmlTypeName });
            const auto synth = f.plugins.getChain (midi, PluginChain::device).back().id;

            auto outputs = [&] (const juce::String& id)
            {
                juce::StringArray ids;

                for (auto& p : f.plugins.getParameters (id))
                    if (p.output)
                        ids.add (p.id);

                return ids.joinIntoString (",");
            };

            expectEquals (outputs (compressor), juce::String ("output gain"));
            expectEquals (outputs (synth), juce::String ("masterLevel"));
        }

        beginTest ("Open in Window floats the device expanded; it follows the device and closes when it goes");
        {
            Cards f;
            const auto id = f.insert (reverb);
            auto view = f.view();
            auto* card = dynamic_cast<DeviceCard*> (findOne (*view, "DeviceCard/Native"));
            expect (card != nullptr && card->onFloat != nullptr);

            if (card == nullptr || card->onFloat == nullptr)
                return;

            auto deviceWindows = [] () -> juce::Component*
            {
                for (int i = 0; i < juce::Desktop::getInstance().getNumComponents(); ++i)
                    if (auto* c = juce::Desktop::getInstance().getComponent (i); c->getComponentID() == "DeviceWindow" && c->isVisible())
                        return c;

                return nullptr;
            };

            card->onFloat();
            auto* window = deviceWindows();
            expect (window != nullptr);

            if (window == nullptr)
                return;

            for (auto& p : f.plugins.getParameters (id))
                expect (findOne (*window, p.id) != nullptr && findOne (*window, p.id)->isVisible(), p.name + " isn't in the window");

            expect (findOne (*window, "fold") == nullptr || ! findOne (*window, "fold")->isVisible());
            expect (f.plugins.getChain (f.trackId(), PluginChain::device).front().size == DeviceSize::compact,
                    "floating it changed the docked card's size");

            f.invoke (cmd::pluginRemove, { f.trackId(), id });
            juce::MessageManager::getInstance()->runDispatchLoopUntil (50);
            expect (deviceWindows() == nullptr, "the removed device's window stayed open");
        }

        beginTest ("Locate loads a missing plug-in from the file the user points at");
        {
            Cards f;
            f.projectSaveLocation = f.scratchDir().getChildFile ("Locate");

            {
                ScannedPlugin scanned (f);
                f.insert (pinboard);
                f.invoke (cmd::projectSaveAs);
            }

            auto& manager = f.projects.getEdit().engine.getPluginManager();
            FakeFormat::registerWith (manager);

            Cards reopened;
            reopened.projectToOpen = f.projectSaveLocation;
            reopened.invoke (cmd::projectOpen);
            reopened.invoke (cmd::trackSelect, { reopened.trackId() });
            expect (reopened.plugins.getChain (reopened.trackId(), PluginChain::device).front().missing);

            // A file that holds no plug-in is refused.
            reopened.pluginFileToChoose = reopened.scratchDir().getChildFile ("Readme.txt");
            auto view = reopened.view();
            click (findOne (*findOne (*view, "DeviceCard/Plugin"), "locate"));
            expect (reopened.errors.size() == 1 && reopened.errors[0].contains ("No plug-in"), reopened.errors.joinIntoString ("; "));

            reopened.pluginFileToChoose = reopened.scratchDir().getChildFile (juce::String ("Moved/Pinboard") + FakeFormat::extension);
            click (findOne (*findOne (*view, "DeviceCard/Plugin"), "locate"));
            expectEquals (reopened.errors.size(), 1, reopened.errors.joinIntoString ("; "));

            auto chain = reopened.plugins.getChain (reopened.trackId(), PluginChain::device);
            expect (chain.size() == 1 && ! chain[0].missing && chain[0].name == "Pinboard");

            reopened.invoke (cmd::projectNew);
            FakeFormat::forgetFound (manager);
        }

        beginTest ("Deleting a plug-in closes its window");
        {
            Cards f;
            const auto id = f.insert (reverb);
            juce::ApplicationCommandManager commandManager;
            MainComponent main (f.app, commandManager);
            main.setSize (1400, 900);

            auto* detail = findType<DetailView> (main);
            expect (detail != nullptr && detail->onOpenEditor != nullptr);

            if (detail == nullptr || detail->onOpenEditor == nullptr)
                return;

            detail->onOpenEditor (id);
            expectEquals (visiblePluginWindows(), 1);

            f.invoke (cmd::pluginRemove, { f.trackId(), id });
            expect (f.plugins.getChain (f.trackId(), PluginChain::device).empty());
            expect (dispatchUntil ([] { return visiblePluginWindows() == 0; }), "the deleted plug-in's window stayed open");

            // Any other way the plug-in goes: its insert undone, its track deleted.
            const auto again = f.insert (reverb);
            detail->onOpenEditor (again);
            expectEquals (visiblePluginWindows(), 1);
            f.invoke (cmd::editUndo);
            expect (! f.plugins.contains (again));
            // The model notifies asynchronously.
            expect (dispatchUntil ([] { return visiblePluginWindows() == 0; }), "undoing the insert left its window open");

            const auto onTrack = f.insert (reverb);
            detail->onOpenEditor (onTrack);
            f.invoke (cmd::trackRemove);
            expect (dispatchUntil ([] { return visiblePluginWindows() == 0; }), "deleting the track left its plug-in's window open");
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

        beginTest ("The chain ends in a drop zone; a drop inserts through plugin.insert, where the opening rule hears it");
        {
            Cards f;
            ScannedPlugin scanned (f);
            f.insert (reverb);
            auto view = f.view();
            juce::StringArray opened, added;
            view->onOpenEditor = [&] (const juce::String& plugin) { opened.add (plugin); };
            f.host.pluginAdded = [&] (const juce::String&, const juce::String& plugin) { added.add (plugin); };

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
            expectEquals (added.joinIntoString (","), chain.back().id);

            // The window is the opening rule's (PluginWindows), not the chain's: it opens it once.
            expect (opened.isEmpty(), "the chain opened the window itself");

            item.name = "Reverb";
            item.pluginPath = reverb;
            target->itemDropped ({ dragDescription (item), nullptr, { 2000, 50 } });
            expectEquals (added.size(), 2);
        }
    }
};

static DeviceCardTests deviceCardTests;

} // namespace resamper::test
