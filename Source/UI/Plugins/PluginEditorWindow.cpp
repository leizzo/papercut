#include "PluginEditorWindow.h"

#include "Engine/PluginRack.h"

namespace resamper
{

namespace
{

    class EmptyPluginEditor : public juce::Component
    {
    public:
        explicit EmptyPluginEditor (ThemeManager& theme) : themeManager (theme) {}

        void paint (juce::Graphics& g) override
        {
            auto& theme = themeManager.getTheme();
            auto& metrics = themeManager.getMetrics();
            g.fillAll (theme.background);
            g.setColour (theme.mutedText);
            g.setFont (themeManager.getFont());
            g.drawFittedText ("This plug-in has no editor",
                              getLocalBounds().reduced (metrics.textPadding),
                              juce::Justification::centred, 2);
        }

    private:
        ThemeManager& themeManager;
    };

    juce::Rectangle<int> editorSize (ThemeManager& themeManager)
    {
        const auto& metrics = themeManager.getMetrics();
        return { metrics.trackHeaderWidth, metrics.trackHeight + metrics.timelineHeight + metrics.velocityLaneHeight };
    }
}

PluginEditorWindow::PluginEditorWindow (PluginRack& rack, ThemeManager& theme, const juce::String& id)
    : FloatingWindow (theme, "Plug-in", "PluginEditorWindow"), pluginId (id)
{
    setResizable (true, false);

    auto content = rack.createEditor (id);
    const auto fallback = editorSize (themeManager);

    if (content == nullptr)
        content = std::make_unique<EmptyPluginEditor> (themeManager);

    if (content->getWidth() <= 0 || content->getHeight() <= 0)
        content->setSize (fallback.getWidth(), fallback.getHeight());

    show (std::move (content));
}

} // namespace resamper
