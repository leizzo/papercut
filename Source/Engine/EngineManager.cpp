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
    /** Whether the engine creates this plug-in asynchronously (AUv3), past the
        createPluginInstance hook, where no sandboxed stand-in can take its place. */
    bool createsAsynchronously (te::Engine& engine, const juce::PluginDescription& desc)
    {
        for (auto* format : engine.getPluginManager().pluginFormatManager.getFormats())
            if (format->getName() == desc.pluginFormatName && format->fileMightContainThisPluginType (desc.fileOrIdentifier)
                && format->requiresUnblockedMessageThreadDuringCreation (desc))
                return true;

        return false;
    }

    /** Whether the plug-in runs in its sandbox: one the sandbox can host, not set to run
        in-process, and not one the engine creates asynchronously. */
    bool runsSandboxed (PluginSandbox& sandbox, te::ExternalPlugin& plugin)
    {
        return ! (bool) plugin.state[PluginSandbox::inProcessProperty] && sandbox.canHost (plugin.desc)
            && ! createsAsynchronously (plugin.engine, plugin.desc);
    }

    /** Creates the plug-in's instance anew, now, from the state saved on it; its old one goes first. */
    void createAgain (te::ExternalPlugin& plugin)
    {
        const auto hadInstance = plugin.getAudioPluginInstance() != nullptr;

        // Processing off deletes the instance; neither change is an undo step.
        if (hadInstance)
        {
            plugin.state.setProperty (te::IDs::process, false, nullptr);
            plugin.processingChanged();
        }

        // The engine still counts a deleted instance as prepared, and would then read
        // the new one without checking it could be created. Initialised with none, it doesn't.
        auto& devices = plugin.engine.getDeviceManager();
        plugin.initialise ({ {}, devices.getSampleRate(), devices.getBlockSize() });

        if (hadInstance)
        {
            plugin.state.setProperty (te::IDs::process, true, nullptr);
            plugin.processingChanged();
        }
        else
        {
            plugin.forceFullReinitialise();
        }
    }

    /** Has the sandbox load the plug-in in the background, unless it has: true then.
        Once it has, the plug-in is created again with it (if it still runs sandboxed). */
    bool loadInSandbox (PluginSandbox& sandbox, te::ExternalPlugin& plugin)
    {
        auto& devices = plugin.engine.getDeviceManager();

        return sandbox.loadInBackground (plugin.desc, plugin.itemID.toString(), devices.getSampleRate(),
                                         devices.getBlockSize(), [&sandbox, ref = te::makeSafeRef (plugin)]
        {
            if (ref != nullptr && runsSandboxed (sandbox, *ref))
                createAgain (*ref);
        });
    }

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
            instance it is, and whether it runs sandboxed (see createPluginInstance below).
            A sandboxed one isn't created until its host has loaded it in the background:
            till then it has no instance (it is loading), and the message thread goes on. */
        bool shouldLoadPlugin (te::ExternalPlugin& plugin) override
        {
            if (! te::EngineBehaviour::shouldLoadPlugin (plugin))
                return false;

            const auto sandboxed = runsSandboxed (sandbox, plugin);

            if (sandboxed && ! loadInSandbox (sandbox, plugin))
                return false;

            sandbox.willLoad (plugin.desc.createIdentifierString(), plugin.itemID.toString(), sandboxed);
            return true;
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
      blockClock (std::make_unique<BlockClock>()),
      engine (std::make_unique<te::Engine> (applicationName,
                                            std::make_unique<ResamperUIBehaviour>(),
                                            std::make_unique<ResamperEngineBehaviour> (audioDevice, *sandbox)))
{
    // A plug-in of an Edit runs in its sandbox when shouldLoadPlugin said so (its
    // host has loaded it by now); anything else, as the engine would.
    auto& plugins = engine->getPluginManager();
    plugins.createPluginInstance = [&s = *sandbox, inProcess = plugins.createPluginInstance]
                                   (const juce::PluginDescription& desc, double rate, int blockSize, juce::String& error)
    {
        juce::String pluginId;
        bool sandboxed = false;

        if (s.takeLoading (desc.createIdentifierString(), pluginId, sandboxed) && sandboxed)
            return s.createInstance (desc, pluginId, error);

        return inProcess (desc, rate, blockSize, error);
    };

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

PluginSandbox& EngineManager::getPluginSandbox() const noexcept
{
    return *sandbox;
}

void EngineManager::recreatePlugin (te::ExternalPlugin& plugin)
{
    if (runsSandboxed (*sandbox, plugin) && ! loadInSandbox (*sandbox, plugin))
        return;

    createAgain (plugin);
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
