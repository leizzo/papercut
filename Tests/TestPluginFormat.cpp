#include "TestPluginFormat.h"

#include <cstdlib>

namespace resamper::test
{

TestPluginFormat::TestPluginFormat (juce::File f) : folder (std::move (f)) {}

void TestPluginFormat::findAllTypesForFile (juce::OwnedArray<juce::PluginDescription>& results, const juce::String& fileOrIdentifier)
{
    const juce::File file (fileOrIdentifier);
    const auto text = file.loadFileAsString().trim();

    // Dies as a plug-in crashing the scanner does (without leaving a crash report).
    if (text == "crash")
        std::_Exit (134);

    if (text == "hang")
        for (;;)
            juce::Thread::sleep (100);

    if (! text.startsWith ("ok "))
        return;

    auto desc = std::make_unique<juce::PluginDescription>();
    desc->name = desc->descriptiveName = text.fromFirstOccurrenceOf ("ok ", false, false);
    desc->pluginFormatName = formatName;
    desc->manufacturerName = "Resamper Tests";
    desc->category = "Effect";
    desc->fileOrIdentifier = file.getFullPathName();
    desc->uniqueId = desc->deprecatedUid = file.getFileName().hashCode();
    desc->lastFileModTime = file.getLastModificationTime();
    desc->numInputChannels = desc->numOutputChannels = 2;
    results.add (desc.release());
}

bool TestPluginFormat::fileMightContainThisPluginType (const juce::String& fileOrIdentifier)
{
    return fileOrIdentifier.endsWithIgnoreCase (fileExtension);
}

juce::String TestPluginFormat::getNameOfPluginFromIdentifier (const juce::String& fileOrIdentifier)
{
    return juce::File (fileOrIdentifier).getFileNameWithoutExtension();
}

bool TestPluginFormat::doesPluginStillExist (const juce::PluginDescription& desc)
{
    return juce::File (desc.fileOrIdentifier).existsAsFile();
}

juce::StringArray TestPluginFormat::searchPathsForPlugins (const juce::FileSearchPath& paths, bool recursive, bool)
{
    juce::StringArray found;

    for (int i = 0; i < paths.getNumPaths(); ++i)
        for (const auto& entry : juce::RangedDirectoryIterator (paths.getRawString (i), recursive,
                                                                juce::String ("*") + fileExtension))
            found.add (entry.getFile().getFullPathName());

    found.sort (true);
    return found;
}

void TestPluginFormat::createPluginInstance (const juce::PluginDescription&, double, int, PluginCreationCallback callback)
{
    callback (nullptr, "Test plug-ins can't be created");
}

} // namespace resamper::test
