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

/** One ChannelStrip per Application Model track (returns included) and a
    MasterStrip. Add Return and Add Bus invoke mixer Commands. */
class MixerView : public juce::Component,
                  private ApplicationModel::Listener,
                  private ThemeManager::Listener
{
public:
    static constexpr const char* componentId = "mixer";

    MixerView (ApplicationModel&, Mixer&, CommandRegistry&, ThemeManager&);
    ~MixerView() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    ApplicationModel& model;
    Mixer& mixer;
    CommandRegistry& commands;
    ThemeManager& themeManager;

    juce::TextButton addReturnButton { "Add Return" }, addBusButton { "Add Bus" };
    std::vector<TrackInfo> tracks;
    std::map<juce::String, std::unique_ptr<ChannelStrip>> strips;
    MasterStrip master;

    void refresh();
    void applyTheme();

    void modelChanged() override   { refresh(); }
    void themeChanged() override;
};

} // namespace papercut
