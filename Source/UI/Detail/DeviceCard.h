#pragma once

#include "Engine/PluginRack.h"
#include "UI/Controls/ContinuousControl.h"

#include <memory>
#include <vector>

namespace papercut
{

class CommandRegistry;

/** One device of a track's device chain (PRD §9.2): 164 high, a title bar in
    the device colour with a power button and the name, and knobs with label
    and value. Power toggles bypass (the card at 50 %, arcs grey); dragging the
    title bar reorders; double-clicking it collapses the card to 28 px wide.
    A knob drag edits that parameter, one undo step per gesture. A missing
    plug-in shows a red dashed outline and a MISSING badge. */
class DeviceCard : public juce::Component
{
public:
    static constexpr int collapsedWidth = 28;
    static constexpr int maxKnobs = 4;

    DeviceCard (CommandRegistry&, PluginRack&, ThemeManager&, const juce::String& trackId, const PluginInfo&);

    const PluginInfo& getPlugin() const noexcept   { return plugin; }

    /** New state from the model; rebuilds the knobs only if the parameters changed. */
    void setState (const PluginInfo&, juce::Colour deviceColour, bool collapsed);

    int getPreferredWidth() const;

    std::function<void()> onToggleCollapsed;
    std::function<void()> onOpenEditor;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    CommandRegistry& commands;
    PluginRack& rack;
    ThemeManager& themeManager;
    juce::String trackId;
    PluginInfo plugin;
    juce::Colour colour;
    bool collapsed = false;

    std::vector<juce::String> parameterIds;
    std::vector<std::unique_ptr<Knob>> knobs;

    juce::Rectangle<int> titleBar() const;
    juce::Rectangle<int> powerButton() const;
    void rebuildKnobs (const std::vector<PluginParameter>&);
    void showMenu();
};

} // namespace papercut
