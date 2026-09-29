#pragma once

#include "Commands/CommandRegistry.h"

#include <functional>

namespace juce { class String; }

namespace resamper
{

class LayoutManager;
class ThemeManager;

namespace cmd
{
    inline constexpr CommandRef<> devReloadLayout { "dev.reloadLayout" };   ///< rebuild the layout hosts whose file changed
    inline constexpr CommandRef<> devReloadTheme { "dev.reloadTheme" };     ///< re-read the Theme and re-style in place
}

/** Registers the Developer Mode reload Commands above.

    dev.toggleOverlay (show the status bar and developer overlay) belongs to
    MainComponent, which owns both.
*/
void registerDeveloperCommands (CommandRegistry&, LayoutManager&, ThemeManager&,
                                std::function<void (const juce::String&)> reportError);

} // namespace resamper
