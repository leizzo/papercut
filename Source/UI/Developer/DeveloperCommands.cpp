#include "DeveloperCommands.h"
#include "Commands/CommandRegistry.h"
#include "UI/Layout/LayoutManager.h"
#include "UI/Theme/ThemeManager.h"

namespace papercut
{

namespace
{
    struct ReloadLayoutCommand : Command
    {
        explicit ReloadLayoutCommand (LayoutManager& lm) : Command ("dev.reloadLayout", "Reload Layout"), layouts (lm) {}

        void execute (const juce::var&) override
        {
            auto rebuilt = layouts.reloadChanged();
            DBG ("Reload Layout rebuilt: " << (rebuilt.isEmpty() ? juce::String ("nothing") : rebuilt.joinIntoString (", ")));
        }

        LayoutManager& layouts;
    };

    struct ReloadThemeCommand : Command
    {
        ReloadThemeCommand (ThemeManager& tm, std::function<void (const juce::String&)> onError)
            : Command ("dev.reloadTheme", "Reload Theme"), themes (tm), reportError (std::move (onError)) {}

        void execute (const juce::var&) override
        {
            if (auto r = themes.reloadTheme(); r.failed() && reportError)
                reportError (r.getErrorMessage());
        }

        ThemeManager& themes;
        std::function<void (const juce::String&)> reportError;
    };
}

void registerDeveloperCommands (CommandRegistry& registry, LayoutManager& layouts, ThemeManager& themes,
                                std::function<void (const juce::String&)> reportError)
{
    registry.add (std::make_unique<ReloadLayoutCommand> (layouts));
    registry.add (std::make_unique<ReloadThemeCommand> (themes, std::move (reportError)));
}

} // namespace papercut
