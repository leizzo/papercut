#include "Inspector.h"

namespace papercut
{

namespace
{
    void styleLabel (juce::Label& label, ThemeManager& themes)
    {
        label.setFont (themes.getFont());
        label.setColour (juce::Label::textColourId, themes.getTheme().text);
        label.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        label.setJustificationType (juce::Justification::centredLeft);
    }
}

Inspector::Inspector (ThemeManager& tm)
    : themes (tm)
{
    for (auto* label : { &nameLabel, &idLabel, &boundsLabel, &parentLabel })
        addAndMakeVisible (label);

    nameLabel.setComponentID ("inspector.name");
    idLabel.setComponentID ("inspector.id");
    boundsLabel.setComponentID ("inspector.bounds");
    parentLabel.setComponentID ("inspector.parent");

    themes.addListener (this);
    applyTheme();
}

Inspector::~Inspector()
{
    themes.removeListener (this);
}

void Inspector::setInspected (juce::Component* component)
{
    if (component == nullptr)
    {
        nameLabel.setText ({}, juce::dontSendNotification);
        idLabel.setText ({}, juce::dontSendNotification);
        boundsLabel.setText ({}, juce::dontSendNotification);
        parentLabel.setText ({}, juce::dontSendNotification);
        return;
    }

    nameLabel.setText (component->getName(), juce::dontSendNotification);
    idLabel.setText (component->getComponentID(), juce::dontSendNotification);
    boundsLabel.setText (component->getBounds().toString(), juce::dontSendNotification);

    auto* parent = component->getParentComponent();
    parentLabel.setText (parent != nullptr ? parent->getComponentID() : juce::String(), juce::dontSendNotification);
}

void Inspector::paint (juce::Graphics& g)
{
    g.fillAll (themes.getTheme().panel);
}

void Inspector::resized()
{
    auto& metrics = themes.getMetrics();
    auto area = getLocalBounds().reduced (metrics.inset);
    const auto row = metrics.trackControlHeight;
    nameLabel.setBounds (area.removeFromTop (row));
    idLabel.setBounds (area.removeFromTop (row));
    boundsLabel.setBounds (area.removeFromTop (row));
    parentLabel.setBounds (area.removeFromTop (row));
}

void Inspector::themeChanged()
{
    applyTheme();
    repaint();
}

void Inspector::applyTheme()
{
    styleLabel (nameLabel, themes);
    styleLabel (idLabel, themes);
    styleLabel (boundsLabel, themes);
    styleLabel (parentLabel, themes);
}

} // namespace papercut
