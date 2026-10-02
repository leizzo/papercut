#include "Preferences.h"

namespace resamper
{

namespace
{
    const juce::Identifier autoOpenPluginWindows ("autoOpenPluginWindows");
    const juce::Identifier pluginWindowsForSelectedTrackOnly ("pluginWindowsForSelectedTrackOnly");
}

Preferences::Preferences()
{
    state.addListener (this);
}

Preferences::~Preferences()
{
    state.removeListener (this);
}

void Preferences::setFile (const juce::File& f)
{
    file = f;

    if (auto xml = juce::XmlDocument::parse (file))
    {
        // Copied property by property, so the tree (and its listeners) stays the same.
        const auto loaded = juce::ValueTree::fromXml (*xml);

        if (loaded.hasType (state.getType()))
            state.copyPropertiesFrom (loaded, nullptr);
    }
}

bool Preferences::getAutoOpenPluginWindows() const
{
    return state.getProperty (autoOpenPluginWindows, true);
}

void Preferences::setAutoOpenPluginWindows (bool on)
{
    state.setProperty (autoOpenPluginWindows, on, nullptr);
}

bool Preferences::getPluginWindowsForSelectedTrackOnly() const
{
    return state.getProperty (pluginWindowsForSelectedTrackOnly, true);
}

void Preferences::setPluginWindowsForSelectedTrackOnly (bool on)
{
    state.setProperty (pluginWindowsForSelectedTrackOnly, on, nullptr);
}

void Preferences::save() const
{
    if (file == juce::File())
        return;

    // A preference that can't be written still holds for this session; nothing else depends on the file.
    auto xml = state.createXml();
    const auto written = xml != nullptr && file.getParentDirectory().createDirectory().wasOk() && xml->writeTo (file);
    juce::ignoreUnused (written);
    jassert (written);
}

void Preferences::valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&)
{
    save();
}

} // namespace resamper
