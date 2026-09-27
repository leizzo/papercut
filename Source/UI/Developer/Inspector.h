#pragma once

#include "UI/Theme/ThemeManager.h"

namespace papercut
{

/** Shows one component's name, ID, bounds, and parent ID. Theme colours only. */
class Inspector : public juce::Component,
                  private ThemeManager::Listener
{
public:
    explicit Inspector (ThemeManager&);
    ~Inspector() override;

    /** nullptr clears the readout. Does not require the component to be showing. */
    void setInspected (juce::Component*);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    ThemeManager& themes;
    juce::Label nameLabel, idLabel, boundsLabel, parentLabel;

    void themeChanged() override;
    void applyTheme();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Inspector)
};

} // namespace papercut
