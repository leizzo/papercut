#include "EngineManager.h"

#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion;

namespace papercut
{

namespace
{
    class PapercutEngineBehaviour : public te::EngineBehaviour
    {
    public:
        explicit PapercutEngineBehaviour (EngineManager::AudioDevice d) : audioDevice (d) {}

        bool autoInitialiseDeviceManager() override    { return audioDevice == EngineManager::AudioDevice::initialise; }

        // Recording is out of scope; opening inputs would only trigger a microphone prompt.
        bool shouldOpenAudioInputByDefault() override   { return false; }

    private:
        EngineManager::AudioDevice audioDevice;
    };

    /** Runs engine background tasks (e.g. offline renders) to completion without
        a progress window, dispatching the message loop while waiting. */
    class PapercutUIBehaviour : public te::UIBehaviour
    {
    public:
        void runTaskWithProgressBar (te::ThreadPoolJobWithProgress& job) override
        {
            while (job.runJob() != juce::ThreadPoolJob::jobHasFinished)
                juce::MessageManager::getInstance()->runDispatchLoopUntil (1);
        }
    };
}

EngineManager::EngineManager (const juce::String& applicationName, AudioDevice audioDevice)
    : engine (std::make_unique<te::Engine> (applicationName,
                                            std::make_unique<PapercutUIBehaviour>(),
                                            std::make_unique<PapercutEngineBehaviour> (audioDevice)))
{
}

EngineManager::~EngineManager() = default;

te::Engine& EngineManager::getEngine() const noexcept
{
    return *engine;
}

juce::String EngineManager::describeActiveAudioDevice() const
{
    if (auto* device = engine->getDeviceManager().deviceManager.getCurrentAudioDevice())
        return device->getTypeName() + ": " + device->getName()
                + " (" + juce::String (juce::roundToInt (device->getCurrentSampleRate())) + " Hz, "
                + juce::String (device->getCurrentBufferSizeSamples()) + " samples)";

    return "No audio device";
}

} // namespace papercut
