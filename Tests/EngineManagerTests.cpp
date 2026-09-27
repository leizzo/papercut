#include "TestFixture.h"

#include <tracktion_engine/tracktion_engine.h>

namespace papercut::test
{

struct EngineManagerTests : juce::UnitTest
{
    EngineManagerTests() : juce::UnitTest ("EngineManager", "Papercut") {}

    void runTest() override
    {
        beginTest ("vends an initialized engine that can host an Edit, without an audio device");
        {
            auto& engine = getEngineManager().getEngine();

            expect (engine.getDeviceManager().deviceManager.getCurrentAudioDevice() == nullptr);
            expectEquals (getEngineManager().describeActiveAudioDevice(), juce::String ("No audio device"));

            auto edit = tracktion::createEmptyEdit (engine, juce::File());
            expect (edit != nullptr);
            expect (&edit->engine == &engine);
        }
    }
};

static EngineManagerTests engineManagerTests;

} // namespace papercut::test
