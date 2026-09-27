#include "MasterStrip.h"
#include "Commands/MixerCommands.h"
#include "Engine/ApplicationModel.h"
#include "UI/Theme/ThemeManager.h"

#include <cmath>

namespace papercut
{

namespace
{
    constexpr double volumeSkewMidPointDb = -12.0;

    juce::String volumeText (double db)
    {
        return db <= ApplicationModel::minVolumeDb ? juce::String ("-inf dB")
                                                   : juce::String (db, 1) + " dB";
    }

    juce::String panText (double pan)
    {
        const auto percent = juce::roundToInt (std::abs (pan) * 100.0);
        return percent == 0 ? juce::String ("C") : (pan < 0 ? "L" : "R") + juce::String (percent);
    }

    void colourSlider (juce::Slider& slider, const Theme& theme)
    {
        slider.setColour (juce::Slider::thumbColourId, theme.accent);
        slider.setColour (juce::Slider::trackColourId, theme.text);
        slider.setColour (juce::Slider::backgroundColourId, theme.laneB);
        slider.setColour (juce::Slider::rotarySliderFillColourId, theme.accent);
        slider.setColour (juce::Slider::rotarySliderOutlineColourId, theme.laneB);
    }
}

MasterStrip::MasterStrip (CommandRegistry& c, ThemeManager& tm)
    : commands (c), themeManager (tm)
{
    volume.setTitle ("Master Volume");
    volume.setSliderStyle (juce::Slider::LinearVertical);
    volume.setRange (ApplicationModel::minVolumeDb, ApplicationModel::maxVolumeDb);
    volume.setSkewFactorFromMidPoint (volumeSkewMidPointDb);
    volume.setDoubleClickReturnValue (true, 0.0);
    volume.textFromValueFunction = volumeText;
    volume.onGestureValue = [this] (double db, bool continues)
    {
        commands.invoke ("mixer.setMasterVolume", masterVolumeArgs (db, continues));
    };

    pan.setTitle ("Master Pan");
    pan.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    pan.setRange (-1.0, 1.0);
    pan.setDoubleClickReturnValue (true, 0.0);
    pan.textFromValueFunction = panText;
    pan.onGestureValue = [this] (double value, bool continues)
    {
        commands.invoke ("mixer.setMasterPan", masterPanArgs (value, continues));
    };

    for (auto* slider : { static_cast<juce::Slider*> (&volume), static_cast<juce::Slider*> (&pan) })
    {
        slider->setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        slider->setPopupDisplayEnabled (true, true, nullptr);
        addAndMakeVisible (slider);
    }

    applyTheme();
    setMaster ({});
}

void MasterStrip::setMaster (const MasterInfo& info)
{
    master = info;

    if (! volume.dragging)
        volume.setValue (master.volumeDb, juce::dontSendNotification);

    if (! pan.dragging)
        pan.setValue (master.pan, juce::dontSendNotification);

    repaint();
}

void MasterStrip::setLevelDb (float db)
{
    if (std::abs (levelDb - db) < 0.25f)
        return;

    levelDb = db;
    repaint (meterBounds);
}

void MasterStrip::applyTheme()
{
    auto& theme = themeManager.getTheme();
    colourSlider (volume, theme);
    colourSlider (pan, theme);
    repaint();
}

void MasterStrip::paint (juce::Graphics& g)
{
    auto& theme = themeManager.getTheme();
    auto& metrics = themeManager.getMetrics();

    g.setColour (theme.panel);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), theme.cornerRadius);

    g.setFont (themeManager.getFont());
    g.setColour (theme.accent);
    g.drawText ("Master", getLocalBounds().reduced (metrics.inset).removeFromTop (metrics.trackControlHeight),
                juce::Justification::centredLeft, true);

    if (! meterBounds.isEmpty())
    {
        g.setColour (theme.laneB);
        g.fillRect (meterBounds);

        const auto span = (float) (ApplicationModel::maxVolumeDb - ApplicationModel::minVolumeDb);
        const auto norm = span <= 0.0f ? 0.0f
                                       : juce::jlimit (0.0f, 1.0f, (levelDb - (float) ApplicationModel::minVolumeDb) / span);
        auto filled = meterBounds;
        filled.removeFromTop (juce::roundToInt ((1.0f - norm) * (float) filled.getHeight()));
        g.setColour (theme.recording);
        g.fillRect (filled);
    }
}

void MasterStrip::resized()
{
    auto& metrics = themeManager.getMetrics();
    auto r = getLocalBounds().reduced (metrics.inset);
    r.removeFromTop (metrics.trackControlHeight);   // the name
    r.removeFromTop (metrics.inset);
    pan.setBounds (r.removeFromTop (metrics.trackControlHeight).removeFromLeft (metrics.trackControlHeight));
    r.removeFromTop (metrics.inset);
    meterBounds = r.removeFromRight (metrics.trackButtonWidth);
    r.removeFromRight (metrics.inset);
    volume.setBounds (r);
}

} // namespace papercut
