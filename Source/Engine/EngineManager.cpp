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

    /** Runs once on the thread it is added to and raises that thread to normal
        priority. Thread::setPriority is protected and only acts on the calling
        thread, so it has to run there. */
    class NormalPriority : public juce::TimeSliceClient
    {
    public:
        int useTimeSlice() override
        {
           #if JUCE_MAC
            pthread_set_qos_class_self_np (QOS_CLASS_DEFAULT, 0);
           #endif
            return -1;
        }
    };
}

EngineManager::EngineManager (const juce::String& applicationName, AudioDevice audioDevice)
    : thumbnailPriority (std::make_unique<NormalPriority>()),
      engine (std::make_unique<te::Engine> (applicationName,
                                            std::make_unique<ResamperUIBehaviour>(),
                                            std::make_unique<ResamperEngineBehaviour> (audioDevice)))
{
    // JUCE runs the shared thumbnail thread at low priority, which on macOS
    // (utility QoS) reads waveforms about 3x slower than normal (#87).
    engine->getAudioFileManager().getAudioThumbnailCache().getTimeSliceThread().addTimeSliceClient (thumbnailPriority.get());
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
