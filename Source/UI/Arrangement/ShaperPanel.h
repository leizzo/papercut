#pragma once

#include "Engine/ApplicationModel.h"
#include "Engine/Shaper.h"
#include "UI/Theme/ThemeManager.h"

#include <optional>

namespace papercut
{

class CommandRegistry;

/** Controls for the selected shaper: mode, depth, and either loop length or
    the audio-trigger envelope. Changes go through Commands.

    The mode row is LayoutMetrics::timelineHeight. The panel is at least one
    trackHeight tall.
*/
class ShaperPanel : public juce::Component,
                    private ApplicationModel::Listener,
                    private ThemeManager::Listener
{
public:
    ShaperPanel (ApplicationModel&, Shaper&, CommandRegistry&, ThemeManager&);
    ~ShaperPanel() override;

    /** parameterKey is what Add assigns. An empty key adds on volume. */
    void setTrack (const juce::String& trackId, const juce::String& parameterKey = "volume");

    /** Changes the parameter Add assigns, without resetting the selected shaper. */
    void setParameterKey (const juce::String& parameterKey);

    int getPreferredHeight() const;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    ApplicationModel& model;
    Shaper& shapers;
    CommandRegistry& commands;
    ThemeManager& themeManager;

    juce::String trackId, parameterKey, shaperId;
    bool updating = false;

    juce::ComboBox shaperList;
    juce::TextButton addButton { "Add" }, loopButton { "Loop" }, audioButton { "Audio Trigger" };
    juce::Label depthLabel { {}, "Depth" }, lengthLabel { {}, "Length" },
                attackLabel { {}, "Attack" }, holdLabel { {}, "Hold" },
                releaseLabel { {}, "Release" }, thresholdLabel { {}, "Threshold" };
    juce::Slider depth, length, attack, hold, release, threshold;

    void refresh();
    void loadControls (const ShaperInfo*);
    void commit();
    void applyTheme();
    std::optional<ShaperInfo> selected() const;
    juce::StringArray idsOnTrack() const;

    void modelChanged() override { refresh(); }
    void themeChanged() override { applyTheme(); }
};

} // namespace papercut
