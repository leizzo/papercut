#pragma once

#include "Engine/Session.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace papercut
{

class CommandRegistry;
class ThemeManager;

/** One scene row's header: its name, and a button that launches the row.
    Double-click the name to rename. Both go through Session commands. */
class SceneHeader : public juce::Component
{
public:
    SceneHeader (CommandRegistry&, ThemeManager&, const SceneInfo&);

    void setScene (const SceneInfo&);
    const SceneInfo& getScene() const noexcept   { return scene; }

    void paint (juce::Graphics&) override;
    void resized() override;
    void applyTheme();

private:
    CommandRegistry& commands;
    ThemeManager& themeManager;
    SceneInfo scene;

    juce::Label name;
    juce::TextButton launch { ">" };

    juce::String displayName() const;
    void commitName();
};

} // namespace papercut
