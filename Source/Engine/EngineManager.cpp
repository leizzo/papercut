#include "EngineManager.h"
#include "ProjectManager.h"

#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion;

namespace resamper
{

namespace
{
    class ResamperEngineBehaviour : public te::EngineBehaviour
    {
    public:
        explicit ResamperEngineBehaviour (EngineManager::AudioDevice d) : audioDevice (d) {}

        bool autoInitialiseDeviceManager() override    { return audioDevice == EngineManager::AudioDevice::initialise; }
        bool shouldOpenAudioInputByDefault() override   { return true; }

        /** Recordings go into the Project's audio folder, named after their track. */
        juce::File getFileForNewAudioRecording (te::Track& track, const juce::String& fileExtension) override
        {
            const auto folder = ProjectManager::getAudioFolder (track.edit.editFileRetriever().getParentDirectory());
            const auto name = juce::File::createLegalFileName (track.getName()) + " Recording ";

            for (int n = 1;; ++n)
                if (auto file = folder.getChildFile (name + juce::String (n) + fileExtension); ! file.exists())
                    return file;
        }

    private:
        EngineManager::AudioDevice audioDevice;
    };

    /** Runs engine background tasks (e.g. offline renders) to completion without
        a progress window, dispatching the message loop while waiting. */
    class ResamperUIBehaviour : public te::UIBehaviour
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
                                            std::make_unique<ResamperUIBehaviour>(),
                                            std::make_unique<ResamperEngineBehaviour> (audioDevice)))
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

} // namespace resamper
