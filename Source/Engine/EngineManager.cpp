#include "EngineManager.h"
#include "PluginSandbox.h"
#include "NativeDevicePlugins.h"
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
        ResamperEngineBehaviour (EngineManager::AudioDevice d, PluginSandbox& s) : audioDevice (d), sandbox (s) {}

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

        /** Called just before a plug-in of an Edit is created: tells the sandbox which
            instance it is, and whether it runs sandboxed (see createInstance below). */
        bool shouldLoadPlugin (te::ExternalPlugin& plugin) override
        {
            const auto load = te::EngineBehaviour::shouldLoadPlugin (plugin);

            if (load)
                sandbox.willLoad (plugin.desc.createIdentifierString(), plugin.itemID.toString(),
                                  ! (bool) plugin.state[PluginSandbox::inProcessProperty]);

            return load;
        }

    private:
        EngineManager::AudioDevice audioDevice;
        PluginSandbox& sandbox;
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
      sandbox (std::make_unique<PluginSandbox>()),
      engine (std::make_unique<te::Engine> (applicationName,
                                            std::make_unique<ResamperUIBehaviour>(),
                                            std::make_unique<ResamperEngineBehaviour> (audioDevice, *sandbox)))
{
    // A plug-in of an Edit runs in its sandbox when the sandbox can host it and
    // the instance isn't set to run in-process; anything else, as the engine would.
    auto& plugins = engine->getPluginManager();
    plugins.createPluginInstance = [&s = *sandbox, inProcess = plugins.createPluginInstance]
                                   (const juce::PluginDescription& desc, double rate, int blockSize, juce::String& error)
    {
        juce::String pluginId;
        bool sandboxed = false;

        if (s.takeLoading (desc.createIdentifierString(), pluginId, sandboxed) && sandboxed && s.canHost (desc))
            return s.createInstance (desc, rate, blockSize, pluginId, error);

        return inProcess (desc, rate, blockSize, error);
    };

    // JUCE runs the shared thumbnail thread at low priority, which on macOS
    // (utility QoS) reads waveforms about 3x slower than normal (#87).
    engine->getAudioFileManager().getAudioThumbnailCache().getTimeSliceThread().addTimeSliceClient (thumbnailPriority.get());

    registerNativeDevices (engine->getPluginManager());
}

EngineManager::~EngineManager() = default;

te::Engine& EngineManager::getEngine() const noexcept
{
    return *engine;
}

PluginSandbox& EngineManager::getPluginSandbox() const noexcept
{
    return *sandbox;
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
