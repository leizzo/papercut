#include "EngineManager.h"
#include "PluginHostingImpl.h"
#include "NativeDevicePlugins.h"
#include "ProjectManager.h"

#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion;

namespace resamper
{

namespace
{
    /** On the audio device, beside the engine: each block it has finished, the sandbox hears. */
    class BlockClock final : public juce::AudioIODeviceCallback
    {
    public:
        void audioDeviceIOCallbackWithContext (const float* const*, int, float* const* outputs, int numOutputs,
                                               int numSamples, const juce::AudioIODeviceCallbackContext&) override
        {
            // What a callback writes is mixed into the device's output: nothing, here.
            for (int i = 0; i < numOutputs; ++i)
                if (outputs[i] != nullptr)
                    juce::FloatVectorOperations::clear (outputs[i], numSamples);

            PluginSandbox::audioBlockFinished();
        }

        void audioDeviceAboutToStart (juce::AudioIODevice*) override {}
        void audioDeviceStopped() override {}
    };

    class ResamperEngineBehaviour : public te::EngineBehaviour
    {
    public:
        ResamperEngineBehaviour (EngineManager::AudioDevice d, PluginHosting& h) : audioDevice (d), hosting (h) {}

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

        /** Called just before a plug-in of an Edit is created: Plug-in Hosting decides where
            it runs. A sandboxed one isn't created until its host has loaded it in the
            background: till then it has no instance (it is loading), and the message thread goes on. */
        bool shouldLoadPlugin (te::ExternalPlugin& plugin) override
        {
            return te::EngineBehaviour::shouldLoadPlugin (plugin) && hosting.getImpl().shouldLoad (plugin);
        }

    private:
        EngineManager::AudioDevice audioDevice;
        PluginHosting& hosting;
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
      hosting (std::make_unique<PluginHosting>()),
      blockClock (std::make_unique<BlockClock>()),
      engine (std::make_unique<te::Engine> (applicationName,
                                            std::make_unique<ResamperUIBehaviour>(),
                                            std::make_unique<ResamperEngineBehaviour> (audioDevice, *hosting)))
{
    hosting->getImpl().attachTo (*engine);

    // JUCE runs the shared thumbnail thread at low priority, which on macOS
    // (utility QoS) reads waveforms about 3x slower than normal (#87).
    engine->getAudioFileManager().getAudioThumbnailCache().getTimeSliceThread().addTimeSliceClient (thumbnailPriority.get());

    registerNativeDevices (engine->getPluginManager());

    // Added after the engine's, so it runs once the engine has processed the block.
    engine->getDeviceManager().deviceManager.addAudioCallback (blockClock.get());
}

EngineManager::~EngineManager()
{
    engine->getDeviceManager().deviceManager.removeAudioCallback (blockClock.get());
}

te::Engine& EngineManager::getEngine() const noexcept
{
    return *engine;
}

PluginHosting& EngineManager::getPluginHosting() const noexcept
{
    return *hosting;
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
