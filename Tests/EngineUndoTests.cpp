#include "TestFixture.h"
#include "Commands/MixerCommands.h"
#include "Commands/PluginCommands.h"
#include "Engine/Mixer.h"
#include "Engine/PluginRack.h"

#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion;

namespace resamper::test
{

/** Engine Undo across facades: a gesture (a drag) is one undo step, and it
    never joins a step another facade started in the middle of it. */
struct EngineUndoTests : juce::UnitTest
{
    EngineUndoTests() : juce::UnitTest ("Engine Undo", "Resamper") {}

    /** One track with a reverb on its device chain; the Mixer and Plug-in
        Commands registered next to the app's. */
    struct UndoFixture : Fixture
    {
        PluginRack rack { projects };
        Mixer mixer { projects };
        juce::String trackId, pluginId;

        UndoFixture()
        {
            registerPluginCommands (commands, rack, host);
            registerMixerCommands (commands, mixer, host);

            invoke ("track.add");
            trackId = model.getTracks()[0].id;
            invoke ("plugin.insert", pluginInsertArgs (trackId, te::ReverbPlugin::xmlTypeName));
            pluginId = rack.getChain (trackId, PluginChain::device)[0].id;
        }

        bool bypassed() const   { return ! rack.getChain (trackId, PluginChain::device)[0].enabled; }
    };

    void runTest() override
    {
        beginTest ("A track volume drag never joins a plug-in step made during it");
        {
            UndoFixture f;
            f.invoke ("track.setVolume", trackVolumeArgs (f.trackId, -3.0));
            f.invoke ("plugin.setBypassed", pluginBypassArgs (f.trackId, f.pluginId, true));
            f.invoke ("track.setVolume", trackVolumeArgs (f.trackId, -6.0, true));

            f.invoke ("edit.undo");
            expectWithinAbsoluteError (f.model.getTracks()[0].volumeDb, -3.0, 1e-3);
            expect (f.bypassed(), "the bypass step survives");
        }

        beginTest ("A plug-in parameter drag never joins a track step made during it");
        {
            UndoFixture f;
            const auto param = f.rack.getParameters (f.pluginId)[0];
            const auto low = param.minimum + (param.maximum - param.minimum) * 0.6f;
            const auto high = param.minimum + (param.maximum - param.minimum) * 0.9f;

            f.invoke ("plugin.setParameter", pluginParameterArgs (f.pluginId, param.id, low));
            f.invoke ("track.add");
            f.invoke ("plugin.setParameter", pluginParameterArgs (f.pluginId, param.id, high, true));

            f.invoke ("edit.undo");
            expectWithinAbsoluteError (f.rack.getParameters (f.pluginId)[0].value, low, 1.0e-4f);
            expectEquals (f.numTracks(), 2, "the track step survives");
        }

        beginTest ("A master volume drag never joins a plug-in step made during it");
        {
            UndoFixture f;
            f.invoke ("mixer.setMasterVolume", masterVolumeArgs (-3.0));
            f.invoke ("plugin.setBypassed", pluginBypassArgs (f.trackId, f.pluginId, true));
            f.invoke ("mixer.setMasterVolume", masterVolumeArgs (-6.0, true));

            f.invoke ("edit.undo");
            expectWithinAbsoluteError (f.mixer.getMaster().volumeDb, -3.0, 1e-3);
            expect (f.bypassed(), "the bypass step survives");
        }

        beginTest ("A drag that continues after an undo is its own step");
        {
            UndoFixture f;
            const auto param = f.rack.getParameters (f.pluginId)[0];
            const auto low = param.minimum + (param.maximum - param.minimum) * 0.6f;
            const auto high = param.minimum + (param.maximum - param.minimum) * 0.9f;

            f.invoke ("plugin.setBypassed", pluginBypassArgs (f.trackId, f.pluginId, true));
            f.invoke ("plugin.setParameter", pluginParameterArgs (f.pluginId, param.id, low));
            f.invoke ("edit.undo");
            f.invoke ("plugin.setParameter", pluginParameterArgs (f.pluginId, param.id, high, true));

            f.invoke ("edit.undo");
            expectWithinAbsoluteError (f.rack.getParameters (f.pluginId)[0].value, param.value, 1.0e-4f);
            expect (f.bypassed(), "the bypass step survives");
        }
    }
};

static EngineUndoTests engineUndoTests;

} // namespace resamper::test
