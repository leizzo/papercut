#pragma once

#include "Engine/PluginRack.h"
#include "UI/Theme/Interaction.h"

#include <optional>

namespace resamper
{

/** `InsertSlot/Filled · Plugin · Empty` (PRD §10.2, §10.6, §9.2.3): one
    mixer insert slot, 19 high. A native device is Filled: a power LED (accent
    when on) and its name. A plug-in adds a plug icon and its format (VST3,
    AU, CLAP) after the name, so the two are told apart without colour. Empty:
    a bg-slot well that stays visible as a drop target. A missing plug-in keeps
    its name, gets a red dashed outline and a MISSING badge. The strip and the
    mixer give it behaviour through the callbacks. */
class InsertSlot : public juce::Component,
                   public juce::SettableTooltipClient
{
public:
    InsertSlot (ThemeManager&, int index);

    int getIndex() const noexcept                              { return index; }
    const std::optional<PluginInfo>& getPlugin() const noexcept  { return plugin; }
    void setPlugin (std::optional<PluginInfo>);

    /** Which design component the slot draws: empty, a native device, a plug-in. */
    enum class Look { empty, filled, plugin };
    Look getLook() const noexcept;

    /** Drop-target feedback: valid (accent-dim outline) or not (not-allowed cursor). */
    void setDropHighlight (std::optional<bool> valid);

    std::function<void (const juce::MouseEvent&)> onClick, onPowerClick, onMenu, onDrag;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

    juce::Rectangle<int> powerBounds() const;

private:
    ThemeManager& themeManager;
    int index;
    std::optional<PluginInfo> plugin;
    std::optional<bool> dropHighlight;
    bool dragStarted = false;
};

} // namespace resamper
