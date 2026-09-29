#include "TestFixture.h"
#include "Commands/MixerCommands.h"
#include "Commands/PluginCommands.h"
#include "Engine/Mixer.h"
#include "Engine/PluginRack.h"

#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion;

namespace resamper::test
{

/** A track's two chains (PRD §4.1–4.2): the device chain (sound design) and
    the mixer inserts (console processing, effects only, at most 8). */
struct DeviceChainTests : juce::UnitTest
{
    DeviceChainTests() : juce::UnitTest ("Device Chain", "Resamper") {}

    struct Chains : Fixture
    {
        PluginRack rack { projects };
        Mixer mixer { projects };

        Chains()
        {
            registerPluginCommands (commands, rack, host);
            registerMixerCommands (commands, mixer, host);
        }

        juce::String trackId (int index = 0) const   { return model.getTracks()[(size_t) index].id; }

        /** The chain's plug-in types, joined with " > ". */
        juce::String paths (const juce::String& id, PluginChain chain) const
        {
            juce::StringArray result;

            for (auto& p : rack.getChain (id, chain))
                result.add (p.path);

            return result.joinIntoString (" > ");
        }

        /** Every plug-in on the track, in signal order, by type name. */
        juce::StringArray signalOrder (const juce::String& id)
        {
            juce::StringArray result;

            if (auto* track = te::findAudioTrackForID (projects.getEdit(), te::EditItemID::fromString (id)))
                for (auto* plugin : track->pluginList)
                    result.add (plugin->getPluginType());

            return result;
        }

        static juce::String joined (juce::StringArray types)   { return types.joinIntoString (" > "); }

        bool insert (const juce::String& id, const char* type, PluginChain chain = PluginChain::device)
        {
            const auto before = errors.size();
            invoke ("plugin.insert", pluginInsertArgs (id, type, chain));
            return errors.size() == before;
        }
    };

    void runTest() override
    {
        const juce::String reverb (te::ReverbPlugin::xmlTypeName), delay (te::DelayPlugin::xmlTypeName),
                           compressor (te::CompressorPlugin::xmlTypeName), eq (te::EqualiserPlugin::xmlTypeName),
                           synth (te::FourOscPlugin::xmlTypeName), midiFx (te::MidiModifierPlugin::xmlTypeName);

        beginTest ("A device goes on the device chain and never shows as a mixer insert");
        {
            Chains f;
            f.invoke ("track.add");
            expect (f.insert (f.trackId(), reverb.toRawUTF8()));

            expectEquals (f.paths (f.trackId(), PluginChain::device), Chains::joined ({ reverb }));
            expect (f.rack.getChain (f.trackId(), PluginChain::mixer).empty());
        }

        beginTest ("A mixer insert shows only on the mixer chain");
        {
            Chains f;
            f.invoke ("track.add");
            expect (f.insert (f.trackId(), compressor.toRawUTF8(), PluginChain::mixer));

            expectEquals (f.paths (f.trackId(), PluginChain::mixer), Chains::joined ({ compressor }));
            expect (f.rack.getChain (f.trackId(), PluginChain::device).empty());
            expect (f.rack.getChain (f.trackId(), PluginChain::mixer).front().chain == PluginChain::mixer);
        }

        beginTest ("Signal order: device chain, then mixer inserts, then sends, then the fader");
        {
            Chains f;
            f.invoke ("track.add");
            f.invoke ("mixer.addReturn");
            const auto id = f.trackId();
            f.invoke ("mixer.addSend", sendArgs (id, f.mixer.getReturns().front().bus));

            expect (f.insert (id, reverb.toRawUTF8()));
            expect (f.insert (id, compressor.toRawUTF8(), PluginChain::mixer));
            expect (f.insert (id, delay.toRawUTF8()));
            expect (f.insert (id, eq.toRawUTF8(), PluginChain::mixer));

            auto order = f.signalOrder (id);
            const juce::StringArray expected { reverb, delay, compressor, eq, te::AuxSendPlugin::xmlTypeName,
                                               te::VolumeAndPanPlugin::xmlTypeName };

            for (int i = 0; i < expected.size(); ++i)
                expectEquals (order[i], expected[i]);
        }

        beginTest ("Mixer inserts take effects only: an instrument or a MIDI effect is refused");
        {
            Chains f;
            f.invoke ("track.addMidi");
            const auto id = f.trackId();
            const auto before = f.signalOrder (id);

            expect (! f.insert (id, synth.toRawUTF8(), PluginChain::mixer));
            expect (f.errors[f.errors.size() - 1].containsIgnoreCase ("effects only"), f.errors[f.errors.size() - 1]);
            expect (! f.insert (id, midiFx.toRawUTF8(), PluginChain::mixer));
            expectEquals (Chains::joined (f.signalOrder (id)), Chains::joined (before));
            expect (! f.model.canUndo() || f.rack.getChain (id, PluginChain::mixer).empty());
        }

        beginTest ("A track holds at most 8 mixer inserts");
        {
            Chains f;
            f.invoke ("track.add");
            const auto id = f.trackId();

            for (int i = 0; i < PluginRack::maxMixerInserts; ++i)
                expect (f.insert (id, eq.toRawUTF8(), PluginChain::mixer));

            expect (! f.insert (id, eq.toRawUTF8(), PluginChain::mixer));
            expectEquals ((int) f.rack.getChain (id, PluginChain::mixer).size(), PluginRack::maxMixerInserts);

            // The device chain has no such limit.
            expect (f.insert (id, reverb.toRawUTF8()));
        }

        beginTest ("A MIDI track's instrument is in its device chain; a new one replaces the built-in synth");
        {
            Chains f;
            f.invoke ("track.addMidi");
            const auto id = f.trackId();
            expectEquals (f.paths (id, PluginChain::device), Chains::joined ({ synth }));

            expect (f.insert (id, te::SamplerPlugin::xmlTypeName));
            expectEquals (f.paths (id, PluginChain::device), Chains::joined ({ te::SamplerPlugin::xmlTypeName }));
            expect (f.model.getTracks()[0].kind == TrackKind::midi);
        }

        beginTest ("plugin.move reorders within one chain only");
        {
            Chains f;
            f.invoke ("track.add");
            const auto id = f.trackId();
            f.insert (id, reverb.toRawUTF8());
            f.insert (id, compressor.toRawUTF8(), PluginChain::mixer);
            f.insert (id, eq.toRawUTF8(), PluginChain::mixer);

            const auto eqId = f.rack.getChain (id, PluginChain::mixer)[1].id;
            f.invoke ("plugin.move", pluginMoveArgs (id, eqId, 0));
            expectEquals (f.paths (id, PluginChain::mixer), Chains::joined ({ eq, compressor }));
            expectEquals (f.paths (id, PluginChain::device), Chains::joined ({ reverb }));

            // Index 1 of the mixer chain doesn't reach into the device chain.
            f.invoke ("plugin.move", pluginMoveArgs (id, eqId, 5));
            expectEquals (f.paths (id, PluginChain::mixer), Chains::joined ({ eq, compressor }));
        }

        beginTest ("plugin.setBypassed toggles a plug-in, one undo step each");
        {
            Chains f;
            f.invoke ("track.add");
            const auto id = f.trackId();
            f.insert (id, compressor.toRawUTF8(), PluginChain::mixer);
            const auto pluginId = f.rack.getChain (id, PluginChain::mixer)[0].id;
            expect (f.rack.getChain (id, PluginChain::mixer)[0].enabled);

            f.invoke ("plugin.setBypassed", pluginBypassArgs (id, pluginId, true));
            expect (! f.rack.getChain (id, PluginChain::mixer)[0].enabled);

            f.invoke ("edit.undo");
            expect (f.rack.getChain (id, PluginChain::mixer)[0].enabled);
        }

        beginTest ("Move to track chain puts a mixer insert at the end of the device chain, one undo step");
        {
            Chains f;
            f.invoke ("track.add");
            const auto id = f.trackId();
            f.insert (id, reverb.toRawUTF8());
            f.insert (id, compressor.toRawUTF8(), PluginChain::mixer);
            f.insert (id, eq.toRawUTF8(), PluginChain::mixer);
            const auto compressorId = f.rack.getChain (id, PluginChain::mixer)[0].id;

            f.invoke ("plugin.moveToDeviceChain", pluginArgs (id, compressorId));
            expectEquals (f.paths (id, PluginChain::device), Chains::joined ({ reverb, compressor }));
            expectEquals (f.paths (id, PluginChain::mixer), Chains::joined ({ eq }));
            expectEquals (f.rack.getChain (id, PluginChain::device)[1].id, compressorId);

            f.invoke ("edit.undo");
            expectEquals (f.paths (id, PluginChain::device), Chains::joined ({ reverb }));
            expectEquals (f.paths (id, PluginChain::mixer), Chains::joined ({ compressor, eq }));
        }

        beginTest ("Opening a project saved before the split keeps its sound: its inserts become the device chain");
        {
            Chains f;
            f.invoke ("track.add");
            f.audioFileToChoose = writeSineWav (f.scratchDir().getChildFile ("tone.wav"), 1.0);
            f.invoke ("clip.add");
            const auto id = f.trackId();

            // What the old single insert chain wrote: plug-ins ahead of the fader, untagged.
            auto* track = te::findAudioTrackForID (f.projects.getEdit(), te::EditItemID::fromString (id));
            auto& edit = f.projects.getEdit();

            for (auto* type : { te::CompressorPlugin::xmlTypeName, te::LowPassPlugin::xmlTypeName })
                track->pluginList.insertPlugin (edit.getPluginCache().createNewPlugin (type, {}),
                                                track->pluginList.indexOf (track->getVolumePlugin()), nullptr);

            const auto order = f.signalOrder (id);
            const auto peak = renderPeak (f);
            expectGreaterThan (peak, 0.05f);

            f.projectSaveLocation = f.scratchDir().getChildFile ("Old Project");
            f.invoke ("project.saveAs");

            Chains reopened;
            reopened.projectToOpen = f.projectSaveLocation;
            reopened.invoke ("project.open");
            const auto reopenedId = reopened.trackId();

            expectEquals (Chains::joined (reopened.signalOrder (reopenedId)), Chains::joined (order));
            expectEquals (reopened.paths (reopenedId, PluginChain::device),
                          Chains::joined ({ te::CompressorPlugin::xmlTypeName, te::LowPassPlugin::xmlTypeName }));
            expect (reopened.rack.getChain (reopenedId, PluginChain::mixer).empty());
            expectWithinAbsoluteError (renderPeak (reopened), peak, 1.0e-4f);
        }

        beginTest ("copyInsert puts a copy of a mixer insert on another track's mixer chain");
        {
            Chains f;
            f.invoke ("track.add");
            f.invoke ("track.add");
            const auto from = f.trackId (0), to = f.trackId (1);
            f.insert (from, compressor.toRawUTF8(), PluginChain::mixer);
            f.insert (to, eq.toRawUTF8(), PluginChain::mixer);
            const auto sourceId = f.rack.getChain (from, PluginChain::mixer)[0].id;

            f.invoke ("plugin.copyInsert", pluginCopyArgs (from, sourceId, to, 0));
            expectEquals (f.paths (to, PluginChain::mixer), Chains::joined ({ compressor, eq }));
            expect (f.rack.getChain (to, PluginChain::mixer)[0].id != sourceId);
            expectEquals (f.paths (from, PluginChain::mixer), Chains::joined ({ compressor }));

            f.invoke ("edit.undo");
            expectEquals (f.paths (to, PluginChain::mixer), Chains::joined ({ eq }));
        }

        beginTest ("A device's parameters are listed with names, ranges and text");
        {
            Chains f;
            f.invoke ("track.add");
            const auto id = f.trackId();
            f.insert (id, reverb.toRawUTF8());
            const auto pluginId = f.rack.getChain (id, PluginChain::device)[0].id;

            const auto params = f.rack.getParameters (pluginId);
            expectGreaterThan ((int) params.size(), 2);

            for (auto& p : params)
            {
                expect (p.id.isNotEmpty() && p.name.isNotEmpty());
                expect (p.maximum > p.minimum);
                expect (p.value >= p.minimum && p.value <= p.maximum);
                expect (f.rack.getParameterText (pluginId, p.id, p.value).isNotEmpty());
            }

            expect (f.rack.getParameters ("no-such-plugin").empty());
        }

        beginTest ("plugin.setParameter changes a device knob; a drag is one undo step");
        {
            Chains f;
            f.invoke ("track.add");
            const auto id = f.trackId();
            f.insert (id, reverb.toRawUTF8());
            const auto pluginId = f.rack.getChain (id, PluginChain::device)[0].id;
            const auto param = f.rack.getParameters (pluginId)[0];
            const auto mid = param.minimum + (param.maximum - param.minimum) * 0.5f;
            const auto high = param.minimum + (param.maximum - param.minimum) * 0.8f;

            f.invoke ("plugin.setParameter", pluginParameterArgs (pluginId, param.id, param.minimum + (param.maximum - param.minimum) * 0.3f));
            f.invoke ("plugin.setParameter", pluginParameterArgs (pluginId, param.id, mid, true));
            f.invoke ("plugin.setParameter", pluginParameterArgs (pluginId, param.id, high, true));
            expectWithinAbsoluteError (f.rack.getParameters (pluginId)[0].value, high, 1.0e-4f);

            auto noValue = pluginParameterArgs (pluginId, param.id, high);
            noValue.getDynamicObject()->removeProperty ("value");
            f.invoke ("plugin.setParameter", noValue);
            expectWithinAbsoluteError (f.rack.getParameters (pluginId)[0].value, high, 1.0e-4f);

            f.invoke ("edit.undo");
            expectWithinAbsoluteError (f.rack.getParameters (pluginId)[0].value, param.value, 1.0e-4f);
            expectEquals ((int) f.rack.getChain (id, PluginChain::device).size(), 1);
        }

        beginTest ("plugin.replace swaps a mixer insert in place, one undo step; an instrument is refused");
        {
            Chains f;
            f.invoke ("track.add");
            const auto id = f.trackId();
            f.insert (id, eq.toRawUTF8(), PluginChain::mixer);
            f.insert (id, compressor.toRawUTF8(), PluginChain::mixer);
            const auto eqId = f.rack.getChain (id, PluginChain::mixer)[0].id;

            f.invoke ("plugin.replace", pluginReplaceArgs (id, eqId, delay));
            expectEquals (f.paths (id, PluginChain::mixer), Chains::joined ({ delay, compressor }));

            f.invoke ("plugin.replace", pluginReplaceArgs (id, f.rack.getChain (id, PluginChain::mixer)[0].id, synth));
            expectEquals (f.paths (id, PluginChain::mixer), Chains::joined ({ delay, compressor }));

            f.invoke ("edit.undo");
            expectEquals (f.paths (id, PluginChain::mixer), Chains::joined ({ eq, compressor }));
        }

        beginTest ("copyInsert refuses a full mixer chain");
        {
            Chains f;
            f.invoke ("track.add");
            f.invoke ("track.add");
            const auto from = f.trackId (0), to = f.trackId (1);
            f.insert (from, compressor.toRawUTF8(), PluginChain::mixer);

            for (int i = 0; i < PluginRack::maxMixerInserts; ++i)
                f.insert (to, eq.toRawUTF8(), PluginChain::mixer);

            f.invoke ("plugin.copyInsert", pluginCopyArgs (from, f.rack.getChain (from, PluginChain::mixer)[0].id, to, 0));
            expectEquals ((int) f.rack.getChain (to, PluginChain::mixer).size(), PluginRack::maxMixerInserts);
            expect (! f.errors.isEmpty());
        }
    }
};

static DeviceChainTests deviceChainTests;

} // namespace resamper::test
