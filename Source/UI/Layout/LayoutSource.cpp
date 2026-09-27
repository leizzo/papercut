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
    if (isDevMode())
    {
        auto file = devDirectory.getChildFile (relativePath);

        if (! file.existsAsFile())
            return juce::Result::fail ("UI file not found: " + file.getFullPathName());

        text = file.loadFileAsString();
        return juce::Result::ok();
    }

    const auto fileName = relativePath.fromLastOccurrenceOf ("/", false, false);

    for (int i = 0; i < PapercutResources::namedResourceListSize; ++i)
    {
        auto* name = PapercutResources::namedResourceList[i];

        if (fileName == PapercutResources::getNamedResourceOriginalFilename (name))
        {
            int size = 0;
            auto* data = PapercutResources::getNamedResource (name, size);
            text = juce::String::fromUTF8 (data, size);
            return juce::Result::ok();
        }
    }

    return juce::Result::fail ("UI file not embedded: " + relativePath);
}

} // namespace papercut
