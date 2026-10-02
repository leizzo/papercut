#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace resamper::test
{

/** A plug-in format for scan tests: a "plug-in" is a file whose text says how
    its scan goes. "ok <name>" finds one effect called name; "crash" kills the
    scanning process, as a crashing plug-in does; "hang" never returns. Files
    end in fileExtension and are found in the folder given to the constructor.
    Nothing it finds can be created. */
class TestPluginFormat : public juce::AudioPluginFormat
{
public:
    explicit TestPluginFormat (juce::File folder = {});

    static constexpr const char* formatName = "ResamperTest";
    static constexpr const char* fileExtension = ".resampertest";

    juce::String getName() const override   { return formatName; }
    void findAllTypesForFile (juce::OwnedArray<juce::PluginDescription>&, const juce::String& fileOrIdentifier) override;
    bool fileMightContainThisPluginType (const juce::String& fileOrIdentifier) override;
    juce::String getNameOfPluginFromIdentifier (const juce::String& fileOrIdentifier) override;
    bool pluginNeedsRescanning (const juce::PluginDescription&) override   { return false; }
    bool doesPluginStillExist (const juce::PluginDescription&) override;
    bool canScanForPlugins() const override   { return true; }
    bool isTrivialToScan() const override   { return false; }
    juce::StringArray searchPathsForPlugins (const juce::FileSearchPath&, bool recursive, bool allowAsync) override;
    juce::FileSearchPath getDefaultLocationsToSearch() override   { return juce::FileSearchPath (folder.getFullPathName()); }
    bool requiresUnblockedMessageThreadDuringCreation (const juce::PluginDescription&) const override   { return false; }

private:
    juce::File folder;

    void createPluginInstance (const juce::PluginDescription&, double, int, PluginCreationCallback) override;
};

} // namespace resamper::test
