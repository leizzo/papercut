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

PluginEditorWindow::PluginEditorWindow (PluginRack& rack, ThemeManager& theme, const juce::String& pluginId)
    : juce::DocumentWindow ("Plug-in", theme.getTheme().background, juce::DocumentWindow::closeButton),
      themeManager (theme)
{
    setComponentID ("PluginEditorWindow");
    setLookAndFeel (&themeManager.getLookAndFeel());
    setTitleBarHeight (themeManager.getMetrics().trackControlHeight);
    setResizable (true, false);

    auto content = rack.createEditor (pluginId);
    const auto fallback = editorSize (themeManager);

    if (content == nullptr)
        content = std::make_unique<EmptyPluginEditor> (themeManager);

    if (content->getWidth() <= 0 || content->getHeight() <= 0)
        content->setSize (fallback.getWidth(), fallback.getHeight());

    setContentOwned (content.release(), true);
    centreWithSize (getWidth(), getHeight());
    themeManager.addListener (this);
    setVisible (true);
}

PluginEditorWindow::~PluginEditorWindow()
{
    themeManager.removeListener (this);
    setLookAndFeel (nullptr);
}

void PluginEditorWindow::closeButtonPressed()
{
    setVisible (false);
}

void PluginEditorWindow::themeChanged()
{
    setBackgroundColour (themeManager.getTheme().background);

    if (auto* content = getContentComponent())
        content->repaint();
}

} // namespace resamper
