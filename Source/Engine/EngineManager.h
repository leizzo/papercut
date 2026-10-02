#pragma once

#include <juce_core/juce_core.h>
#include <memory>

namespace tracktion::inline engine { class Engine; }
namespace juce { class AudioIODeviceCallback; }

namespace resamper
{

class PluginHosting;

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

    /** Where each plug-in of the engine's Edits runs, in its Sandbox or in-process. */
    PluginHosting& getPluginHosting() const noexcept;

    /** Human-readable name of the active output device, or a note that none is open. */
    juce::String describeActiveAudioDevice() const;

private:
    // Declared before the engine so it outlives the thread it may still be queued on.
    std::unique_ptr<juce::TimeSliceClient> thumbnailPriority;

    // Before the engine, which creates plug-ins through it until it goes.
    std::unique_ptr<PluginHosting> hosting;

    // Tells the sandbox when the device has finished a block; on the engine's device while both exist.
    std::unique_ptr<juce::AudioIODeviceCallback> blockClock;

    std::unique_ptr<tracktion::Engine> engine;

    JUCE_DECLARE_NON_COPYABLE (EngineManager)
};

} // namespace resamper
