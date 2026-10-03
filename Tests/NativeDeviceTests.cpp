#include "TestFixture.h"
#include "Commands/ClipCommands.h"
#include "Commands/EditCommands.h"
#include "Commands/PluginCommands.h"
#include "Commands/TrackCommands.h"
#include "Engine/NativeDeviceDsp.h"
#include "UI/Detail/CompressorDevice.h"
#include "UI/Detail/DetailView.h"
#include "UI/Detail/EqEightDevice.h"
#include "UI/State/ShellState.h"

#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion;

namespace resamper::test
{

namespace
{
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

    juce::Component* findId (juce::Component& root, const juce::String& id)
    {
        for (auto* child : root.getChildren())
        {
            if (child->getComponentID() == id)
                return child;

            if (auto* match = findId (*child, id))
                return match;
        }

        return nullptr;
    }

    /** Presses at from, drags through each point, and lets go: one gesture. */
    void drag (juce::Component& c, juce::Point<int> from, std::initializer_list<juce::Point<int>> path)
    {
        c.mouseDown (mouseEvent (c, from, from, false));
        auto last = from;

        for (auto p : path)
        {
            c.mouseDrag (mouseEvent (c, p, from, true));
            last = p;
        }

        c.mouseUp (mouseEvent (c, last, from, true));
    }

    void wheel (juce::Component& c, juce::Point<int> at, float deltaY)
    {
        juce::MouseWheelDetails details;
        details.deltaX = 0;
        details.deltaY = deltaY;
        details.isReversed = false;
        details.isSmooth = false;
        details.isInertial = false;
        c.mouseWheelMove (mouseEvent (c, at, at, false), details);
    }
}

/** PRD §9.2.1a (#67): the v2 native devices, EQ Eight and Compressor v2.
    Their graphs are controllers, and their meters and spectra reach the UI
    from the audio thread without locks. */
struct NativeDeviceTests : juce::UnitTest
{
    NativeDeviceTests() : juce::UnitTest ("Native Devices v2", "Resamper") {}

    struct Devices : Fixture
    {
        Devices()
        {
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

        /** A 440 Hz sine at -6 dBFS on the track. */
        void addTone()
        {
            audioFileToChoose = writeSineWav (scratchDir().getChildFile ("tone.wav"), 1.0);
            invoke (cmd::clipAdd);
        }

        float value (const juce::String& pluginId, const juce::String& parameterId)
        {
            for (auto& p : plugins.getParameters (pluginId))
                if (p.id == parameterId)
                    return p.value;

            return std::numeric_limits<float>::quiet_NaN();
        }

        void set (const juce::String& pluginId, const juce::String& parameterId, float v)
        {
            invoke (cmd::pluginSetParameter, { pluginId, parameterId, v });
        }

        NativeDevices& natives()   { return plugins.getNativeDevices(); }

        /** Lets the model tell the cards what changed: it notifies asynchronously. */
        void settle()   { juce::MessageManager::getInstance()->runDispatchLoopUntil (50); }

        /** Undoes the last step, as a card on screen sees it. */
        void undo()
        {
            invoke (cmd::editUndo);
            settle();
        }

        std::unique_ptr<DetailView> view()
        {
            theme.load();
            auto v = std::make_unique<DetailView> (model, plugins, app.engine.getPluginHosting(), commands, theme, shell, uiState);
            v->setSize (1400, 240);
            return v;
        }

        juce::ValueTree uiState { "detail" };
        ShellState shell { juce::ValueTree ("shell") };
    };

    static juce::String band (int b, const char* field)   { return NativeDevices::bandParameter (b, field); }

    float responseAt (Devices& f, const juce::String& id, float hz, int bandIndex = -1, int channel = 0)
    {
        std::vector<float> db;
        f.natives().getEqResponse (id, { hz }, db, bandIndex, channel);
        return db.empty() ? 0.0f : db.front();
    }

    void runTest() override
    {
        const juce::String eq (NativeDevices::eqEightType), compressor (NativeDevices::compressorType);

        beginTest ("The catalogue's EQ and Compressor are the v2 devices, built into Resamper");
        {
            Devices f;
            juce::StringArray paths;

            for (auto& info : f.plugins.getCatalogue())
            {
                paths.add (info.path);

                if (info.path == eq || info.path == compressor)
                    expectEquals (info.manufacturer, juce::String ("Resamper"));
            }

            expect (paths.contains (eq) && paths.contains (compressor));
            expect (! paths.contains (te::EqualiserPlugin::xmlTypeName), "v2 replaces the v1 equaliser");
            expect (! paths.contains (te::CompressorPlugin::xmlTypeName), "v2 replaces the v1 compressor");

            const auto id = f.insert (eq);
            expect (f.errors.isEmpty(), f.errors.joinIntoString ("; "));
            expectEquals ((int) f.plugins.getParameters (id).size(), NativeDevices::numEqBands * 6 + 4);
            expectEquals (f.plugins.getChain (f.trackId(), PluginChain::device).back().name, juce::String ("EQ Eight"));

            // Scale and Out (EQ), Makeup, Mix and Out (Compressor) are the Output zone.
            juce::StringArray outputs;

            for (auto* type : { eq.toRawUTF8(), compressor.toRawUTF8() })
                for (auto& p : f.plugins.getParameters (f.insert (type)))
                    if (p.output)
                        outputs.add (p.id);

            expectEquals (outputs.joinIntoString (","), juce::String ("scale,output,makeupAuto,makeup,mix,output"));

            // A project or Command naming the v1 types still gets them.
            expect (f.insert (te::CompressorPlugin::xmlTypeName).isNotEmpty());
            expect (f.errors.isEmpty(), f.errors.joinIntoString ("; "));
        }

        beginTest ("EQ Eight's curve is its bands: gain, Scale, Adaptive Q and on/off");
        {
            Devices f;
            const auto id = f.insert (eq);
            expectWithinAbsoluteError (responseAt (f, id, 1000.0f), 0.0f, 0.01f, "a new EQ Eight is transparent");

            f.set (id, band (2, "Type"), (float) dsp::EqBandType::bell);
            f.set (id, band (2, "Freq"), 1000.0f);
            f.set (id, band (2, "Gain"), 6.0f);
            expectWithinAbsoluteError (responseAt (f, id, 1000.0f), 6.0f, 0.1f);
            expectWithinAbsoluteError (responseAt (f, id, 1000.0f, 2), 6.0f, 0.1f, "one band alone");
            expectWithinAbsoluteError (responseAt (f, id, 40.0f), 0.0f, 0.2f);

            const auto wide = responseAt (f, id, 2000.0f);
            f.set (id, "adaptiveQ", 1.0f);
            expectLessThan (responseAt (f, id, 2000.0f), wide - 0.3f, "Adaptive Q narrows a boost");
            f.set (id, "adaptiveQ", 0.0f);

            f.set (id, "scale", 0.5f);
            expectWithinAbsoluteError (responseAt (f, id, 1000.0f), 3.0f, 0.1f, "Scale halves every gain");

            f.set (id, band (2, "On"), 0.0f);
            expectWithinAbsoluteError (responseAt (f, id, 1000.0f), 0.0f, 0.01f, "a band that is off is not on the curve");

            // A choice snaps to a legal value.
            f.set (id, band (2, "Type"), 2.4f);
            expectEquals (f.value (id, band (2, "Type")), 2.0f);
        }

        beginTest ("EQ Eight reaches the audio; in M/S a Side band leaves a centred tone alone");
        {
            Devices f;
            f.addTone();
            const auto flat = renderPeak (f);
            const auto id = f.insert (eq);

            f.set (id, band (2, "Type"), (float) dsp::EqBandType::bell);
            f.set (id, band (2, "Freq"), 440.0f);
            f.set (id, band (2, "Gain"), 6.0f);
            expectWithinAbsoluteError (renderPeak (f), flat * 2.0f, flat * 0.1f);

            f.set (id, "mode", 2.0f);
            f.set (id, band (2, "Channel"), 2.0f);
            expectWithinAbsoluteError (renderPeak (f), flat, flat * 0.05f);
            expectWithinAbsoluteError (responseAt (f, id, 440.0f, -1, 0), 0.0f, 0.01f, "Mid hears no Side band");
            expectWithinAbsoluteError (responseAt (f, id, 440.0f, -1, 1), 6.0f, 0.1f);

            f.set (id, "output", -6.0f);
            expectWithinAbsoluteError (renderPeak (f), flat * 0.5f, flat * 0.05f);
        }

        beginTest ("Dragging a node moves frequency and gain in one undo step");
        {
            Devices f;
            const auto id = f.insert (eq);
            const auto freq = band (3, "Freq"), gain = band (3, "Gain");
            const auto freqBefore = f.value (id, freq);

            for (int i = 1; i <= 5; ++i)
                f.invoke (cmd::pluginSetParameters, { id, { { freq, freqBefore * (1.0f + 0.1f * (float) i) },
                                                            { gain, (float) i } }, i > 1 });

            expectWithinAbsoluteError (f.value (id, gain), 5.0f, 1.0e-4f);
            f.invoke (cmd::editUndo);
            expectWithinAbsoluteError (f.value (id, gain), 0.0f, 1.0e-4f);
            expectWithinAbsoluteError (f.value (id, freq), freqBefore, 0.01f);
        }

        beginTest ("Compressor v2's curve: threshold, ratio, knee and expand");
        {
            Devices f;
            const auto id = f.insert (compressor);
            f.set (id, "threshold", -20.0f);
            f.set (id, "ratio", 4.0f);
            f.set (id, "knee", 0.0f);
            expectWithinAbsoluteError (f.natives().getTransferDb (id, -10.0f), -17.5f, 0.01f);
            expectWithinAbsoluteError (f.natives().getTransferDb (id, -30.0f), -30.0f, 0.01f);

            f.set (id, "knee", 6.0f);
            expectLessThan (f.natives().getTransferDb (id, -20.0f), -20.0f, "the soft knee bends at the threshold");

            f.set (id, "detect", 2.0f);
            expectWithinAbsoluteError (f.natives().getTransferDb (id, -40.0f), -100.0f, 0.01f, "Expand pushes down below");

            f.set (id, "detect", 0.0f);
            expectGreaterThan (f.natives().getMakeupDb (id), 0.0f, "auto makeup is on by default");
            f.set (id, "makeupAuto", 0.0f);
            expectWithinAbsoluteError (f.natives().getMakeupDb (id), 0.0f, 1.0e-4f);
        }

        beginTest ("Compressor v2 turns a loud tone down, and its meters read it");
        {
            Devices f;
            f.addTone();
            const auto flat = renderPeak (f);
            const auto id = f.insert (compressor);
            f.set (id, "threshold", -30.0f);
            f.set (id, "ratio", 10.0f);
            f.set (id, "makeupAuto", 0.0f);
            f.set (id, "attack", 0.1f);
            f.set (id, "lookahead", 1.0f);   // the gain is down before the tone's first peak
            f.natives().readDynamics (id);

            expectLessThan (renderPeak (f), flat * 0.3f);
            const auto reading = f.natives().readDynamics (id);
            expectWithinAbsoluteError (reading.inputLeftDb, -6.0f, 0.5f);
            expectWithinAbsoluteError (reading.inputRightDb, -6.0f, 0.5f);
            expectGreaterThan (reading.reductionDb, 15.0f);

            // A read takes the peaks: the next one starts again from silence.
            expectLessThan (f.natives().readDynamics (id).inputLeftDb, -100.0f);

            f.set (id, "lookahead", 2.0f);
            expectWithinAbsoluteError (f.plugins.getChain (f.trackId(), PluginChain::device).back().latencySamples,
                                       juce::roundToInt (0.010 * f.projects.getEdit().engine.getDeviceManager().getSampleRate()), 1,
                                       "10 ms of lookahead is the device's latency");
        }

        beginTest ("The spectrum comes from the audio thread: the tone shows at 440 Hz");
        {
            Devices f;
            f.addTone();
            const auto id = f.insert (eq);
            f.set (id, band (0, "On"), 1.0f);
            f.set (id, band (0, "Freq"), 2000.0f);   // a low cut well above the tone: post loses it
            renderPeak (f);

            std::vector<float> pre, post;
            expect (f.natives().getSpectrum (id, false, { 440.0f, 5000.0f }, pre));
            f.natives().getSpectrum (id, true, { 440.0f, 5000.0f }, post);
            expectGreaterThan (pre[0], -20.0f);
            expectGreaterThan (pre[0], pre[1] + 30.0f);
            expectLessThan (post[0], pre[0] - 12.0f);

            std::vector<float> none;
            expect (! f.natives().getSpectrum (f.insert (compressor), true, { 440.0f }, none), "a compressor has no spectrum");
        }

        beginTest ("A modulated parameter reports its range; an unmodulated one none");
        {
            Devices f;
            const auto id = f.insert (eq);
            const auto freq = band (2, "Freq");
            expect (! f.natives().getModulationRange (id, freq).has_value());

            expect (f.shaper.add (f.trackId(), "plugin:" + id + ":" + freq, ShaperMode::loop).wasOk());
            const auto range = f.natives().getModulationRange (id, freq);
            expect (range.has_value() && ! range->isEmpty());
        }

        beginTest ("The EQ Eight card: Display, then the band panel and Output; the strip mirrors the bands");
        {
            Devices f;
            const auto id = f.insert (eq);
            auto view = f.view();
            auto* card = findId (*view, "DeviceCard/Native");
            auto* graph = findType<EqEightGraph> (*view);
            auto* device = findType<EqEightDevice> (*view);
            expect (card != nullptr && graph != nullptr && device != nullptr);

            if (card == nullptr || graph == nullptr || device == nullptr)
                return;

            expectEquals (card->getWidth(), EqEightDevice::compactWidth);
            expectEquals (card->getHeight(), 164);
            expect (graph->getRight() < findId (*card, "freq")->getX(), "the Display comes before the band panel");

            for (int b = 0; b < NativeDevices::numEqBands; ++b)
            {
                auto* button = dynamic_cast<juce::Button*> (findId (*card, "band" + juce::String (b + 1)));
                expect (button != nullptr && button->getToggleState() == (f.value (id, band (b, "On")) >= 0.5f));
            }

            // A band switched off elsewhere (a Command) goes dim on the strip.
            f.set (id, band (3, "On"), 0.0f);
            f.settle();
            auto* fourth = dynamic_cast<juce::Button*> (findId (*card, "band4"));
            expect (fourth != nullptr && ! fourth->getToggleState());

            // The panel follows the selected band.
            device->selectBand (2);
            auto* freq = dynamic_cast<ParameterRow*> (findId (*card, "freq"));
            expect (freq != nullptr && freq->getModel().getText() == "250 Hz", freq != nullptr ? freq->getModel().getText() : "no row");

            // St / L-R / M-S.
            auto* mode = dynamic_cast<Segmented*> (findId (*card, "mode"));
            expect (mode != nullptr);
            mode->setSelectedIndex (2);
            expectEquals (f.value (id, "mode"), 2.0f);
            expect (findId (*card, "channel")->isVisible(), "in M/S a band picks its side");
        }

        beginTest ("EQ graph: drag a node = freq and gain, wheel = Q, double-click = band on/off; each one undo step");
        {
            Devices f;
            const auto id = f.insert (eq);
            auto view = f.view();
            auto* graph = findType<EqEightGraph> (*view);
            expect (graph != nullptr);

            if (graph == nullptr)
                return;

            const auto freq = band (2, "Freq"), gain = band (2, "Gain"), q = band (2, "Q"), on = band (2, "On");
            const auto freqBefore = f.value (id, freq), qBefore = f.value (id, q);
            const auto node = graph->nodePosition (2).toInt();
            expectEquals (graph->nodeAt (node.toFloat()), 2);

            drag (*graph, node, { node + juce::Point<int> (10, -5), node + juce::Point<int> (30, -15), node + juce::Point<int> (40, -20) });
            expectGreaterThan (f.value (id, freq), freqBefore * 1.2f);
            expectGreaterThan (f.value (id, gain), 3.0f);
            expectWithinAbsoluteError (graph->nodePosition (2).x, (float) node.x + 40.0f, 1.0f, "the node follows the mouse");

            f.undo();
            expectWithinAbsoluteError (f.value (id, freq), freqBefore, 0.01f);
            expectWithinAbsoluteError (f.value (id, gain), 0.0f, 1.0e-4f);

            wheel (*graph, node, 1.0f);
            wheel (*graph, node, 1.0f);
            expectGreaterThan (f.value (id, q), qBefore * 1.2f);
            f.undo();
            expectWithinAbsoluteError (f.value (id, q), qBefore, 1.0e-4f, "two quick notches are one step");

            graph->mouseDoubleClick (mouseEvent (*graph, node, node, false));
            expectEquals (f.value (id, on), 0.0f);
            auto* strip = dynamic_cast<juce::Button*> (findId (*view, "band3"));
            expect (strip != nullptr && ! strip->getToggleState(), "the strip mirrors the graph");
            f.undo();
            expectEquals (f.value (id, on), 1.0f);

            // Double-click empty space: the first band that is off becomes a bell there.
            const juce::Point<int> empty (graph->getWidth() / 2, graph->getHeight() / 4);
            expectEquals (graph->nodeAt (empty.toFloat()), -1);
            graph->mouseDoubleClick (mouseEvent (*graph, empty, empty, false));
            expectEquals (f.value (id, band (0, "On")), 1.0f);
            expectEquals (f.value (id, band (0, "Type")), (float) dsp::EqBandType::bell);
            expectGreaterThan (f.value (id, band (0, "Gain")), 3.0f);
            f.undo();
            expectEquals (f.value (id, band (0, "On")), 0.0f);
            expectEquals (f.value (id, band (0, "Type")), (float) dsp::EqBandType::lowCut);
        }

        beginTest ("Compressor card: drag the threshold line; Transfer / Activity; IN / GR meters and the live dot");
        {
            Devices f;
            f.addTone();
            const auto id = f.insert (compressor);
            auto view = f.view();
            auto* card = findId (*view, "DeviceCard/Native");
            auto* graph = findType<CompressorGraph> (*view);
            auto* device = findType<CompressorDevice> (*view);
            auto* meters = findType<DynamicsMeters> (*view);
            expect (card != nullptr && graph != nullptr && device != nullptr && meters != nullptr);

            if (card == nullptr || graph == nullptr || device == nullptr || meters == nullptr)
                return;

            expectEquals (card->getWidth(), CompressorDevice::compactWidth);
            expect (meters->getRight() < graph->getX() && graph->getRight() < findId (*card, "ratio")->getX()
                        && findId (*card, "ratio")->getRight() < findId (*card, "mix")->getX(),
                    "Input, Display, Controls, Output from left to right");

            const auto before = f.value (id, "threshold");
            const auto x = juce::roundToInt (graph->thresholdPosition()), y = graph->getHeight() / 2;
            drag (*graph, { x, y }, { { x + 10, y }, { x + 20, y }, { x + 30, y } });
            expectGreaterThan (f.value (id, "threshold"), before + 5.0f);
            f.undo();
            expectWithinAbsoluteError (f.value (id, "threshold"), before, 1.0e-4f, "the drag is one undo step");

            // A drag away from the line moves nothing.
            drag (*graph, { 2, y }, { { 40, y } });
            expectWithinAbsoluteError (f.value (id, "threshold"), before, 1.0e-4f);

            auto* switcher = dynamic_cast<Segmented*> (findId (*card, "view"));
            expect (switcher != nullptr && ! graph->showsActivity());
            switcher->setSelectedIndex (1);
            expect (graph->showsActivity());
            const auto lineY = juce::roundToInt (graph->thresholdPosition());
            drag (*graph, { graph->getWidth() / 2, lineY }, { { graph->getWidth() / 2, lineY - 15 } });
            expectGreaterThan (f.value (id, "threshold"), before + 3.0f, "in Activity the line drags up and down");

            // Meters: what the audio thread sent, read at UI rate.
            f.set (id, "threshold", -30.0f);
            renderPeak (f);
            device->refreshReadings();
            expectGreaterThan (meters->getInputDb (0), -10.0f);
            expectGreaterThan (meters->getReductionDb(), 3.0f);
            expect (graph->getLiveLevel().has_value(), "the live level dot shows while signal flows");
        }

        beginTest ("The v2 devices' settings survive save and load");
        {
            Devices f;
            const auto eqId = f.insert (eq);
            const auto compId = f.insert (compressor);
            f.set (eqId, band (5, "Freq"), 3300.0f);
            f.set (eqId, "mode", 2.0f);
            f.set (compId, "threshold", -27.0f);
            f.set (compId, "lookahead", 2.0f);

            f.projectSaveLocation = f.scratchDir().getChildFile ("NativeDevices");
            f.invoke (cmd::projectSaveAs);

            Fixture reopened;
            reopened.projectToOpen = f.projectSaveLocation;
            reopened.invoke (cmd::projectOpen);
            expect (reopened.errors.isEmpty(), reopened.errors.joinIntoString ("; "));

            const auto chain = reopened.plugins.getChain (reopened.model.getTracks().front().id, PluginChain::device);
            expectEquals ((int) chain.size(), 2);

            auto valueOf = [&] (const juce::String& pluginId, const juce::String& parameterId)
            {
                for (auto& p : reopened.plugins.getParameters (pluginId))
                    if (p.id == parameterId)
                        return p.value;

                return std::numeric_limits<float>::quiet_NaN();
            };

            if (chain.size() == 2)
            {
                expectWithinAbsoluteError (valueOf (chain[0].id, band (5, "Freq")), 3300.0f, 0.5f);
                expectEquals (valueOf (chain[0].id, "mode"), 2.0f);
                expectWithinAbsoluteError (valueOf (chain[1].id, "threshold"), -27.0f, 1.0e-4f);
                expectEquals (valueOf (chain[1].id, "lookahead"), 2.0f);
            }
        }

        beginTest ("Audition plays one band and is never an undo step");
        {
            Devices f;
            const auto id = f.insert (eq);
            const auto undoName = f.projects.getEdit().getUndoManager().getUndoDescription();
            f.invoke (cmd::pluginAudition, { id, 3 });
            expectEquals (f.natives().getAudition (id), 3);
            expectEquals (f.projects.getEdit().getUndoManager().getUndoDescription(), undoName);
            f.invoke (cmd::pluginAudition, { id, -1 });
            expectEquals (f.natives().getAudition (id), -1);
        }
    }
};

static NativeDeviceTests nativeDeviceTests;

} // namespace resamper::test
