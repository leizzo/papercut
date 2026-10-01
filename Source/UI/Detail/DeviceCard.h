#pragma once

#include "Engine/PluginRack.h"
#include "UI/Controls/Controls.h"

#include <functional>
#include <memory>
#include <optional>

namespace resamper
{

class CommandRegistry;

/** How much of a native device shows (PRD §9.2.1a): a 28 px strip, the 164 px
    card that never scrolls, or every parameter docked across the detail view.
    A plug-in card has one size. */
enum class DeviceSize { folded, compact, expanded };

/** One device of a track's device chain (PRD §9.2). A chain mixes two
    contracts, told apart at a glance: a NativeDeviceCard for a built-in, a
    PluginDeviceCard for a scanned (VST3 / AU / CLAP) plug-in. Both are 164
    high; dragging the title bar reorders the chain and right-clicking it opens
    the card's menu. A bypassed device's card drops to 50 %. */
class DeviceCard : public juce::Component
{
public:
    static constexpr int height = 164;

    /** The card for info's contract. */
    static std::unique_ptr<DeviceCard> create (CommandRegistry&, PluginRack&, ThemeManager&, const juce::String& trackId,
                                               const PluginInfo&);

    const PluginInfo& getPlugin() const noexcept   { return plugin; }

    /** New state from the model. A plug-in card ignores the size. */
    virtual void setState (const PluginInfo&, DeviceSize) = 0;

    /** Width in the chain. dockedWidth is what an expanded device fills at least. */
    virtual int getPreferredWidth (int dockedWidth) const = 0;

    /** Whether the plug-in's window is open. */
    virtual void setWindowOpen (bool) {}

    /** Gives the first control keyboard focus (a native device just dropped in). */
    virtual void focusFirstControl() {}

    std::function<void (DeviceSize)> onSizeChange;
    std::function<void()> onOpenEditor;

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

protected:
    DeviceCard (CommandRegistry&, PluginRack&, ThemeManager&, const juce::String& trackId, const PluginInfo&);

    CommandRegistry& commands;
    PluginRack& rack;
    ThemeManager& themeManager;
    juce::String trackId;
    PluginInfo plugin;

    /** Where a drag reorders and a right-click opens the menu. */
    virtual juce::Rectangle<int> getTitleBar() const = 0;

    /** The card's own menu items; Bypass and Delete follow them. */
    virtual void addMenuItems (juce::PopupMenu&) = 0;

    void toggleBypass();
    void showMenu();
};

/** A device's power button, on while the device is enabled: a dark disc with
    a dot in the device colour (native), or an accent ring and dot (plug-in). */
class DevicePowerButton : public ThemedButton
{
public:
    enum class Style { native, plugin };

    DevicePowerButton (ThemeManager&, Style);

    /** The native dot's colour. */
    void setDotColour (juce::Colour);

    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

private:
    Style style;
    juce::Colour dot;
};

/** A button in a native device's coloured header: an icon or a short label in
    text-on-accent, with a darker wash on hover. */
class DeviceHeaderButton : public ThemedButton
{
public:
    DeviceHeaderButton (ThemeManager&, const juce::String& name, std::optional<Icon>, const juce::String& text = {});

    void setIcon (Icon i)   { icon = i; repaint(); }

    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

private:
    std::optional<Icon> icon;
};

} // namespace resamper
