#pragma once

#include <functional>

namespace juce { class String; }

namespace resamper
{

class CommandRegistry;
class LayoutManager;
class ThemeManager;

/** Registers the Developer Mode reload Commands:

    dev.reloadLayout   rebuild the layout hosts whose file changed
    dev.reloadTheme    re-read the Theme and re-style in place (geometry untouched)

    dev.toggleOverlay (show the status bar and developer overlay) belongs to
    MainComponent, which owns both.
*/
void registerDeveloperCommands (CommandRegistry&, LayoutManager&, ThemeManager&,
                                std::function<void (const juce::String&)> reportError);

} // namespace resamper
