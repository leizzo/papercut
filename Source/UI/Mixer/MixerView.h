#pragma once

#include "ChannelStrip.h"
#include "MasterStrip.h"
#include "Engine/ApplicationModel.h"
#include "UI/Theme/ThemeManager.h"

#include <map>
#include <memory>
#include <vector>

namespace papercut
{

class CommandRegistry;
class Mixer;

/** The Mixer view (PRD §10.1): the 40 px toolbar, then the strips area — the
    track strips, the return strips, and the master strip — scrolling
    sideways when they don't fit. Meters refresh at 30 Hz while the mixer
    shows, and pause while it's hidden (§19). */
class MixerView : public juce::Component,
                  private ApplicationModel::Listener,
                  private ThemeManager::Listener,
                  private juce::Timer
{
public:
    static constexpr const char* componentId = "mixer";

    MixerView (ApplicationModel&, Mixer&, PluginRack&, CommandRegistry&, ThemeManager&);
    ~MixerView() override;

    /** A strip's Track chain row was clicked. */
    std::function<void (const juce::String& trackId)> onShowDeviceChain;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct StripsArea : juce::Component {};

    ApplicationModel& model;
    Mixer& mixer;
    PluginRack& plugins;
    CommandRegistry& commands;
    ThemeManager& themeManager;

    Button addReturnButton, addBusButton, addSendButton, toBusButton;
    juce::Viewport viewport;
    StripsArea stripsArea;
    std::vector<juce::String> trackOrder, returnOrder;
    std::map<juce::String, std::unique_ptr<ChannelStrip>> strips;
    MasterStrip master;
    double lastMeterTime = 0;

    void refresh();
    void layoutStrips();
    void timerCallback() override;
    juce::String targetTrackId() const;

    void modelChanged() override   { refresh(); }
    void themeChanged() override   { repaint(); }
};

} // namespace papercut
