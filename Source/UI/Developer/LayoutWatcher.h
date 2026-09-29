#pragma once

#include <juce_core/juce_core.h>
#include <functional>
#include <memory>

namespace resamper
{

/** Watches a folder for .json updates via gin::FileSystemWatcher.

    MainComponent watches the dev-mode layouts and themes folders and invokes
    dev.reloadLayout or dev.reloadTheme. Callbacks arrive on the message thread.
*/
class LayoutWatcher
{
public:
    explicit LayoutWatcher (const juce::File& folder);
    ~LayoutWatcher();

    /** Called when a watched .json file is updated. */
    std::function<void (const juce::File&)> onJsonUpdated;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LayoutWatcher)
};

} // namespace resamper
