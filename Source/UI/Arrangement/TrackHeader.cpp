#include "TrackHeader.h"
#include "Commands/AppCommands.h"
#include "UI/Theme/ThemeManager.h"

namespace papercut
{

namespace
{
    constexpr double volumeSkewMidPointDb = -12.0;   // half-way along the fader

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
}

TrackHeader::TrackHeader (CommandRegistry& c, ThemeManager& tm, const TrackInfo& info)
    : commands (c), themeManager (tm), track (info)
{
    setInterceptsMouseClicks (false, true);

    muteButton.setTooltip ("Mute");
    soloButton.setTooltip ("Solo");
    muteButton.onClick = [this] { commands.invoke ("track.toggleMute", trackArgs (track.id)); };
    soloButton.onClick = [this] { commands.invoke ("track.toggleSolo", trackArgs (track.id)); };

    volume.setTitle ("Volume");
    volume.setSliderStyle (juce::Slider::LinearHorizontal);
    volume.setRange (ApplicationModel::minVolumeDb, ApplicationModel::maxVolumeDb);
    volume.setSkewFactorFromMidPoint (volumeSkewMidPointDb);
    volume.setDoubleClickReturnValue (true, 0.0);
    volume.textFromValueFunction = volumeText;
    volume.onGestureValue = [this] (double db, bool continues)
    {
        commands.invoke ("track.setVolume", trackVolumeArgs (track.id, db, continues));
    };

    pan.setTitle ("Pan");
    pan.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    pan.setRange (-1.0, 1.0);
    pan.setDoubleClickReturnValue (true, 0.0);
    pan.textFromValueFunction = panText;
    pan.onGestureValue = [this] (double value, bool continues)
    {
        commands.invoke ("track.setPan", trackPanArgs (track.id, value, continues));
    };

    for (auto* s : { &volume, &pan })
    {
        s->setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        s->setPopupDisplayEnabled (true, true, nullptr);
    }

    for (auto* child : std::initializer_list<juce::Component*> { &muteButton, &soloButton, &pan, &volume })
        addAndMakeVisible (child);

    applyTheme();
    setTrack (info);
}

void TrackHeader::setTrack (const TrackInfo& info)
{
    track = info;
    muteButton.setToggleState (track.muted, juce::dontSendNotification);
    soloButton.setToggleState (track.solo, juce::dontSendNotification);

    // A drag in progress already shows the value it is sending.
    if (! volume.dragging)
        volume.setValue (track.volumeDb, juce::dontSendNotification);

    if (! pan.dragging)
        pan.setValue (track.pan, juce::dontSendNotification);

    repaint();
}

void TrackHeader::applyTheme()
{
    auto& theme = themeManager.getTheme();
    muteButton.setColour (juce::TextButton::buttonOnColourId, theme.mute);
    soloButton.setColour (juce::TextButton::buttonOnColourId, theme.solo);
    muteButton.setColour (juce::TextButton::textColourOnId, theme.background);
    soloButton.setColour (juce::TextButton::textColourOnId, theme.background);
}

void TrackHeader::paint (juce::Graphics& g)
{
    auto& theme = themeManager.getTheme();
    auto& metrics = themeManager.getMetrics();

    g.setColour (track.selected ? theme.trackHeaderSelected : theme.trackHeader);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), theme.cornerRadius);

    g.setFont (themeManager.getFont());
    g.setColour (track.selected ? theme.text : theme.mutedText);
    g.drawText (track.name,
                getLocalBounds().reduced (metrics.textPadding).removeFromTop (metrics.trackControlHeight),
                juce::Justification::centredLeft, true);
}

void TrackHeader::resized()
{
    auto& metrics = themeManager.getMetrics();
    auto r = getLocalBounds().reduced (metrics.textPadding);
    r.removeFromTop (metrics.trackControlHeight);   // the name

    r.removeFromTop (metrics.inset);
    auto buttons = r.removeFromTop (metrics.trackControlHeight);
    muteButton.setBounds (buttons.removeFromLeft (metrics.trackButtonWidth));
    buttons.removeFromLeft (metrics.inset);
    soloButton.setBounds (buttons.removeFromLeft (metrics.trackButtonWidth));
    pan.setBounds (buttons.removeFromRight (metrics.trackControlHeight));

    r.removeFromTop (metrics.inset);
    volume.setBounds (r.removeFromTop (metrics.trackControlHeight));
}

} // namespace papercut
