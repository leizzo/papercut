#pragma once

#include "Inspector.h"

namespace papercut
{

/** Optional debug strip. The parent sets the status text (tracks, clips, playhead);
    this component does not query the engine and does no audio-thread work. */
class DeveloperOverlay : public juce::Component,
                         private ThemeManager::Listener
{
public:
    explicit DeveloperOverlay (ThemeManager&);
    ~DeveloperOverlay() override;

    void setStatusText (const juce::String&);
    Inspector& getInspector() noexcept   { return inspector; }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    ThemeManager& themes;
    juce::Label status;
    Inspector inspector;

    void themeChanged() override;
    void applyTheme();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeveloperOverlay)
};

} // namespace papercut
