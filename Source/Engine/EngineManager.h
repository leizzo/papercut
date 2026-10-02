#pragma once

#include <juce_core/juce_core.h>
#include <memory>

namespace tracktion::inline engine { class Engine; }

namespace resamper
{

class PluginSandbox;

/** The single owner and creation point of the Tracktion Engine.

    Nothing else in the application creates a tracktion::Engine. The header only
    forward-declares the engine so it can be included from code that cannot see
    Tracktion headers.
*/
class EngineManager
{
public:
    enum class AudioDevice
    {
        initialise,  ///< Open the default audio device (the app)
        none         ///< Headless: never touch an audio device (tests, CI)
    };

    EngineManager (const juce::String& applicationName, AudioDevice);
    ~EngineManager();

    tracktion::Engine& getEngine() const noexcept;

    /** Where the engine's plug-ins run out of process (each plug-in of an
        Edit that the sandbox can host, unless it runs in-process). */
    PluginSandbox& getPluginSandbox() const noexcept;

    /** Human-readable name of the active output device, or a note that none is open. */
    juce::String describeActiveAudioDevice() const;

private:
    // Declared before the engine so it outlives the thread it may still be queued on.
    std::unique_ptr<juce::TimeSliceClient> thumbnailPriority;

    // Before the engine, which creates plug-ins through it until it goes.
    std::unique_ptr<PluginSandbox> sandbox;
    std::unique_ptr<tracktion::Engine> engine;

    JUCE_DECLARE_NON_COPYABLE (EngineManager)
};

} // namespace resamper
