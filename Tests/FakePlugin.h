#pragma once

#include "TestFixture.h"
#include "Engine/EngineManager.h"
#include "Engine/PluginSandbox.h"

#include <tracktion_engine/tracktion_engine.h>

namespace resamper::test
{

/** A plug-in with six parameters, standing in for a scanned VST3. Its state
    is its parameter values; it has a fixed-size editor unless built without. */
struct FakePlugin : juce::AudioPluginInstance
{
    static constexpr int editorWidth = 320, editorHeight = 180;

    static juce::PluginDescription description()
    {
        juce::PluginDescription d;
        d.name = "Pinboard";
        d.descriptiveName = "Pinboard";
        d.manufacturerName = "Resamper Tests";
        d.version = "1.2.0";
        d.pluginFormatName = "VST3";
        d.category = "Fx";
        d.fileOrIdentifier = "/Library/Audio/Plug-Ins/VST3/Pinboard.vst3";
        d.uniqueId = d.deprecatedUid = 0x50696e62;
        d.numInputChannels = d.numOutputChannels = 2;
        return d;
    }

    FakePlugin()
        : juce::AudioPluginInstance (BusesProperties().withInput ("In", juce::AudioChannelSet::stereo())
                                                      .withOutput ("Out", juce::AudioChannelSet::stereo()))
    {
        for (int i = 0; i < 6; ++i)
            juce::AudioProcessor::addParameter (new juce::AudioParameterFloat (juce::ParameterID { "p" + juce::String (i), 1 },
                                                                               "Param " + juce::String (i + 1), 0.0f, 1.0f, 0.5f));
    }

    /** The plug-in's own window: a plain fixed-size component. */
    struct Editor : juce::AudioProcessorEditor
    {
        explicit Editor (FakePlugin& p) : juce::AudioProcessorEditor (p)
        {
            setComponentID ("vendorEditor");
            setSize (editorWidth, editorHeight);
        }

        ~Editor() override   { processor.editorBeingDeleted (this); }

        void paint (juce::Graphics& g) override   { g.fillAll (juce::Colours::darkred); }
    };

    void fillInPluginDescription (juce::PluginDescription& d) const override   { d = description(); }
    const juce::String getName() const override                    { return "Pinboard"; }
    void prepareToPlay (double, int) override                     {}
    void releaseResources() override                              {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    double getTailLengthSeconds() const override                  { return 0; }
    bool acceptsMidi() const override                             { return false; }
    bool producesMidi() const override                            { return false; }
    juce::AudioProcessorEditor* createEditor() override           { return new Editor (*this); }
    bool hasEditor() const override                               { return true; }
    int getNumPrograms() override                                 { return 1; }
    int getCurrentProgram() override                              { return 0; }
    void setCurrentProgram (int) override                         {}
    const juce::String getProgramName (int) override              { return {}; }
    void changeProgramName (int, const juce::String&) override    {}

    void getStateInformation (juce::MemoryBlock& block) override
    {
        juce::MemoryOutputStream out (block, false);

        for (auto* parameter : getParameters())
            out.writeFloat (parameter->getValue());
    }

    void setStateInformation (const void* data, int size) override
    {
        juce::MemoryInputStream in (data, (size_t) size, false);

        for (auto* parameter : getParameters())
            if (! in.isExhausted())
                parameter->setValueNotifyingHost (in.readFloat());
    }
};

/** A plug-in format whose ".fakeplugin" files hold FakePlugin: what Locate scans. */
struct FakeFormat : juce::AudioPluginFormat
{
    static constexpr const char* extension = ".fakeplugin";

    juce::String getName() const override   { return "Fake"; }

    void findAllTypesForFile (juce::OwnedArray<juce::PluginDescription>& results, const juce::String& path) override
    {
        if (! fileMightContainThisPluginType (path))
            return;

        auto d = FakePlugin::description();
        d.pluginFormatName = getName();
        d.fileOrIdentifier = path;
        results.add (new juce::PluginDescription (d));
    }

    bool fileMightContainThisPluginType (const juce::String& path) override   { return path.endsWith (extension); }
    juce::String getNameOfPluginFromIdentifier (const juce::String& path) override { return path; }
    bool pluginNeedsRescanning (const juce::PluginDescription&) override      { return false; }
    bool doesPluginStillExist (const juce::PluginDescription&) override       { return true; }
    bool canScanForPlugins() const override                                   { return false; }
    bool isTrivialToScan() const override                                     { return true; }
    juce::StringArray searchPathsForPlugins (const juce::FileSearchPath&, bool, bool) override { return {}; }
    juce::FileSearchPath getDefaultLocationsToSearch() override               { return {}; }
    bool requiresUnblockedMessageThreadDuringCreation (const juce::PluginDescription&) const override { return false; }

    void createPluginInstance (const juce::PluginDescription&, double, int, PluginCreationCallback callback) override
    {
        callback (std::make_unique<FakePlugin>(), {});
    }

    /** Registers the format with the engine once per run (a format can't be removed). */
    static void registerWith (tracktion::PluginManager& manager)
    {
        static bool registered = false;

        if (! std::exchange (registered, true))
            manager.pluginFormatManager.addFormat (std::make_unique<FakeFormat>());
    }

    /** Forgets what Locate found, so later tests see the plug-in missing again. */
    static void forgetFound (tracktion::PluginManager& manager)
    {
        for (auto& type : manager.knownPluginList.getTypes())
            if (type.pluginFormatName == "Fake")
                manager.knownPluginList.removeType (type);
    }
};

/** While alive, the engine knows FakePlugin as a scanned VST3 and can create it. */
struct ScannedPlugin
{
    explicit ScannedPlugin (Fixture& f)
        : manager (f.projects.getEdit().engine.getPluginManager()), sandbox (f.app.engine.getPluginSandbox())
    {
        // Created here, in-process: the sandbox can't load a VST3 that isn't there.
        sandbox.removeHostedFormat (FakePlugin::description().pluginFormatName);
        previous = manager.createPluginInstance;
        manager.createPluginInstance = [fallback = previous] (const juce::PluginDescription& d, double rate, int block,
                                                              juce::String& error) -> std::unique_ptr<juce::AudioPluginInstance>
        {
            if (d.fileOrIdentifier == FakePlugin::description().fileOrIdentifier)
                return std::make_unique<FakePlugin>();

            return fallback (d, rate, block, error);
        };
        manager.knownPluginList.addType (FakePlugin::description());
    }

    ~ScannedPlugin()
    {
        manager.knownPluginList.removeType (FakePlugin::description());
        manager.createPluginInstance = previous;
        sandbox.addHostedFormat (FakePlugin::description().pluginFormatName);
    }

    tracktion::PluginManager& manager;
    PluginSandbox& sandbox;
    decltype (tracktion::PluginManager::createPluginInstance) previous;
};

/** The first external plug-in instance in the Edit, or nullptr. */
inline juce::AudioPluginInstance* firstExternalInstance (Fixture& f)
{
    for (auto* plugin : tracktion::getAllPlugins (f.projects.getEdit(), false))
        if (auto* external = dynamic_cast<tracktion::ExternalPlugin*> (plugin))
            return external->getAudioPluginInstance();

    return nullptr;
}

} // namespace resamper::test
