#pragma once

#include <functional>

namespace juce { class String; }

namespace papercut
{

class CommandRegistry;
class LayoutManager;
class ThemeManager;

/** Registers the Developer Mode Commands (the entire slice scope):

    dev.reloadLayout   rebuild the layout hosts whose file changed
    dev.reloadTheme    re-read the Theme and re-style in place (geometry untouched)
*/
void registerDeveloperCommands (CommandRegistry&, LayoutManager&, ThemeManager&,
                                std::function<void (const juce::String&)> reportError);

} // namespace papercut
