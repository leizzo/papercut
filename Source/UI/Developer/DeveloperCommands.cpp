#include "DeveloperCommands.h"
#include "Commands/CommandRegistry.h"
#include "UI/Layout/LayoutManager.h"
#include "UI/Theme/ThemeManager.h"

namespace resamper
{

void registerDeveloperCommands (CommandRegistry& registry, LayoutManager& layouts, ThemeManager& themes,
                                std::function<void (const juce::String&)> reportError)
{
    registry.add ({ "dev.reloadLayout", "Reload Layout" }, [&layouts]
    {
        auto rebuilt = layouts.reloadChanged();
        DBG ("Reload Layout rebuilt: " << (rebuilt.isEmpty() ? juce::String ("nothing") : rebuilt.joinIntoString (", ")));
    });

    registry.add ({ "dev.reloadTheme", "Reload Theme" }, [&themes, onError = std::move (reportError)]
    {
        if (auto r = themes.reloadTheme(); r.failed() && onError)
            onError (r.getErrorMessage());
    });
}

} // namespace resamper
