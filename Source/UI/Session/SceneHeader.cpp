#include "SceneHeader.h"
#include "Commands/SessionCommands.h"
#include "UI/Theme/ThemeManager.h"

#include <algorithm>

namespace papercut
{

SceneHeader::SceneHeader (CommandRegistry& c, ThemeManager& tm, const SceneInfo& info)
    : commands (c), themeManager (tm), scene (info)
{
    name.setEditable (false, true, false);
    name.setJustificationType (juce::Justification::centredLeft);
    name.setTooltip ("Double-click to rename");
    name.onEditorHide = [this] { commitName(); };

    launch.setTooltip ("Launch scene");
    launch.onClick = [this]
    {
        commands.invoke ("session.launchScene", sessionSceneArgs (scene.index));
    };

    addAndMakeVisible (name);
    addAndMakeVisible (launch);
    applyTheme();
    setScene (info);
}

void SceneHeader::setScene (const SceneInfo& info)
{
    scene = info;

    if (! name.isBeingEdited())
        name.setText (displayName(), juce::dontSendNotification);

    repaint();
}

juce::String SceneHeader::displayName() const
{
    return scene.name.isEmpty() ? "Scene " + juce::String (scene.index + 1) : scene.name;
}

void SceneHeader::commitName()
{
    const auto text = name.getText().trim();
    const auto fallback = "Scene " + juce::String (scene.index + 1);

    if (text == scene.name || (scene.name.isEmpty() && (text.isEmpty() || text == fallback)))
    {
        name.setText (displayName(), juce::dontSendNotification);
        return;
    }

    commands.invoke ("session.renameScene", sessionRenameSceneArgs (scene.index, text));
}

void SceneHeader::applyTheme()
{
    auto& theme = themeManager.getTheme();
    name.setFont (themeManager.getFont());
    name.setColour (juce::Label::textColourId, theme.text);
    name.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    name.setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    name.setColour (juce::Label::backgroundWhenEditingColourId, theme.panel);
    name.setColour (juce::Label::textWhenEditingColourId, theme.text);
    name.setColour (juce::Label::outlineWhenEditingColourId, theme.accent);

    launch.setColour (juce::TextButton::buttonColourId, theme.accent);
    launch.setColour (juce::TextButton::textColourOffId, theme.background);
    repaint();
}

void SceneHeader::paint (juce::Graphics& g)
{
    auto& theme = themeManager.getTheme();
    g.setColour (theme.trackHeader);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), theme.cornerRadius);
}

void SceneHeader::resized()
{
    auto& metrics = themeManager.getMetrics();
    auto r = getLocalBounds().reduced (metrics.textPadding, metrics.inset);
    const auto button = std::max (metrics.trackButtonWidth, metrics.trackControlHeight);
    launch.setBounds (r.removeFromRight (button));
    r.removeFromRight (metrics.inset);
    name.setBounds (r);
}

} // namespace papercut
