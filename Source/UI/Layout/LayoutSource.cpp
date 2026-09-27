#include "LayoutSource.h"

#include <PapercutResources.h>

namespace papercut
{

LayoutSource::LayoutSource()
{
   #ifdef PAPERCUT_DEV_UI_DIR
    if (juce::File dir (PAPERCUT_DEV_UI_DIR); dir.isDirectory())
        devDirectory = dir;
   #endif
}

juce::Result LayoutSource::read (const juce::String& relativePath, juce::String& text) const
{
    juce::MemoryBlock data;

    if (auto r = readData (relativePath, data); r.failed())
        return r;

    text = data.toString();
    return juce::Result::ok();
}

juce::Result LayoutSource::readData (const juce::String& relativePath, juce::MemoryBlock& data) const
{
    if (isDevMode())
    {
        auto file = devDirectory.getChildFile (relativePath);

        if (! file.existsAsFile() || ! file.loadFileAsData (data))
            return juce::Result::fail ("UI file not found: " + file.getFullPathName());

        return juce::Result::ok();
    }

    const auto fileName = relativePath.fromLastOccurrenceOf ("/", false, false);

    for (int i = 0; i < PapercutResources::namedResourceListSize; ++i)
    {
        auto* name = PapercutResources::namedResourceList[i];

        if (fileName == PapercutResources::getNamedResourceOriginalFilename (name))
        {
            int size = 0;
            auto* bytes = PapercutResources::getNamedResource (name, size);
            data.replaceAll (bytes, (size_t) size);
            return juce::Result::ok();
        }
    }

    return juce::Result::fail ("UI file not embedded: " + relativePath);
}

} // namespace papercut
