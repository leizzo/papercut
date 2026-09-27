#include "ChannelStrip.h"
#include "Commands/AppCommands.h"
#include "Commands/MixerCommands.h"
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

    void prepareSlider (juce::Slider& slider)
    {
        slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        slider.setPopupDisplayEnabled (true, true, nullptr);
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

ChannelStrip::SendRow::SendRow (ThemeManager& tm) : themeManager (tm)
{
    mute.setTooltip ("Mute send");
    gain.setTitle ("Send");
    gain.setTooltip ("Send level");
    gain.setSliderStyle (juce::Slider::LinearHorizontal);
    gain.setRange (ApplicationModel::minVolumeDb, ApplicationModel::maxVolumeDb);
    gain.setSkewFactorFromMidPoint (volumeSkewMidPointDb);
    gain.setDoubleClickReturnValue (true, 0.0);
    gain.textFromValueFunction = volumeText;
    prepareSlider (gain);
    addAndMakeVisible (mute);
    addAndMakeVisible (gain);
}

void ChannelStrip::SendRow::resized()
{
    auto& metrics = themeManager.getMetrics();
    auto r = getLocalBounds();
    mute.setBounds (r.removeFromLeft (metrics.trackButtonWidth));
    r.removeFromLeft (metrics.inset);
    gain.setBounds (r);
}

void ChannelStrip::SendRow::applyTheme()
{
    auto& theme = themeManager.getTheme();
    mute.setColour (juce::TextButton::buttonOnColourId, theme.mute);
    mute.setColour (juce::TextButton::textColourOnId, theme.background);
    mute.setColour (juce::TextButton::textColourOffId, theme.text);
    colourSlider (gain, theme);
}

ChannelStrip::ChannelStrip (CommandRegistry& c, ThemeManager& tm, const TrackInfo& info,
                            const std::vector<SendInfo>& sendList, const std::vector<PluginInfo>& inserts)
    : commands (c), themeManager (tm)
{
    muteButton.setTooltip ("Mute");
    soloButton.setTooltip ("Solo");
    muteButton.onClick = [this] { commands.invoke ("track.toggleMute", trackArgs (track.id)); };
    soloButton.onClick = [this] { commands.invoke ("track.toggleSolo", trackArgs (track.id)); };

    volume.setTitle ("Volume");
    volume.setSliderStyle (juce::Slider::LinearVertical);
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

    for (auto* slider : { static_cast<juce::Slider*> (&volume), static_cast<juce::Slider*> (&pan) })
        prepareSlider (*slider);

    for (auto* child : std::initializer_list<juce::Component*> { &muteButton, &soloButton, &pan, &volume })
        addAndMakeVisible (child);

    setState (info, sendList, inserts);
}

void ChannelStrip::rebuildSends (const std::vector<SendInfo>& next)
{
    auto same = sendRows.size() == next.size();

    for (size_t i = 0; same && i < next.size(); ++i)
        same = sendRows[i]->sendId == next[i].id;

    if (! same)
    {
        sendRows.clear();

        for (auto& send : next)
        {
            auto row = std::make_unique<SendRow> (themeManager);
            row->sendId = send.id;
            row->gain.onGestureValue = [this, id = send.id] (double db, bool continues)
            {
                commands.invoke ("mixer.setSendGain", sendGainArgs (track.id, id, db, continues));
            };
            row->mute.onClick = [this, id = send.id]
            {
                for (auto& current : sends)
                    if (current.id == id)
                        commands.invoke ("mixer.setSendMuted", sendMutedArgs (track.id, id, ! current.muted));
            };
            addAndMakeVisible (*row);
            sendRows.push_back (std::move (row));
        }
    }

    for (size_t i = 0; i < next.size(); ++i)
    {
        sendRows[i]->muted = next[i].muted;
        sendRows[i]->mute.setToggleState (next[i].muted, juce::dontSendNotification);
        sendRows[i]->mute.setTooltip ("Mute send, bus " + juce::String (next[i].bus));

        if (! sendRows[i]->gain.dragging)
            sendRows[i]->gain.setValue (next[i].gainDb, juce::dontSendNotification);
    }
}

void ChannelStrip::rebuildInserts (const std::vector<PluginInfo>& inserts)
{
    auto same = insertLabels.size() == inserts.size();

    for (size_t i = 0; same && i < inserts.size(); ++i)
        same = insertLabels[i]->getText() == inserts[i].name;

    if (same)
        return;

    insertLabels.clear();

    for (auto& insert : inserts)
    {
        auto label = std::make_unique<juce::Label>();
        label->setText (insert.name, juce::dontSendNotification);
        label->setJustificationType (juce::Justification::centredLeft);
        label->setInterceptsMouseClicks (false, false);
        label->setTooltip (insert.name);
        addAndMakeVisible (*label);
        insertLabels.push_back (std::move (label));
    }
}

void ChannelStrip::setState (const TrackInfo& info, const std::vector<SendInfo>& sendList,
                             const std::vector<PluginInfo>& inserts)
{
    track = info;
    sends = sendList;
    muteButton.setToggleState (track.muted, juce::dontSendNotification);
    soloButton.setToggleState (track.solo, juce::dontSendNotification);

    if (! volume.dragging)
        volume.setValue (track.volumeDb, juce::dontSendNotification);

    if (! pan.dragging)
        pan.setValue (track.pan, juce::dontSendNotification);

    rebuildSends (sends);
    rebuildInserts (inserts);
    applyTheme();
    resized();
}

void ChannelStrip::setLevelDb (float db)
{
    if (std::abs (levelDb - db) < 0.25f)
        return;

    levelDb = db;
    repaint (meterBounds);
}

void ChannelStrip::applyTheme()
{
    auto& theme = themeManager.getTheme();

    for (auto [button, colour] : { std::pair (&muteButton, theme.mute), std::pair (&soloButton, theme.solo) })
    {
        button->setColour (juce::TextButton::buttonOnColourId, colour);
        button->setColour (juce::TextButton::textColourOnId, theme.background);
        button->setColour (juce::TextButton::textColourOffId, theme.text);
    }

    colourSlider (volume, theme);
    colourSlider (pan, theme);

    for (auto& row : sendRows)
        row->applyTheme();

    for (auto& label : insertLabels)
    {
        label->setColour (juce::Label::textColourId, theme.mutedText);
        label->setFont (themeManager.getFont());
    }

    repaint();
}

void ChannelStrip::paint (juce::Graphics& g)
{
    auto& theme = themeManager.getTheme();
    auto& metrics = themeManager.getMetrics();

    g.setColour (track.selected ? theme.trackHeaderSelected : theme.trackHeader);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), theme.cornerRadius);

    g.setFont (themeManager.getFont());
    auto row = getLocalBounds().reduced (metrics.inset).removeFromTop (metrics.trackControlHeight);
    const auto kind = track.kind == TrackKind::midi ? juce::String ("MIDI") : juce::String ("Audio");
    auto kindArea = row.removeFromRight (juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), kind) + metrics.inset);

    g.setColour (track.selected ? theme.text : theme.mutedText);
    g.drawText (track.name, row, juce::Justification::centredLeft, true);
    g.setColour (theme.accent);
    g.drawText (kind, kindArea, juce::Justification::centredRight, true);

    if (! meterBounds.isEmpty())
    {
        g.setColour (theme.laneB);
        g.fillRect (meterBounds);

        const auto span = (float) (ApplicationModel::maxVolumeDb - ApplicationModel::minVolumeDb);
        const auto norm = span <= 0.0f ? 0.0f
                                       : juce::jlimit (0.0f, 1.0f, (levelDb - (float) ApplicationModel::minVolumeDb) / span);
        auto filled = meterBounds;
        filled.removeFromTop (juce::roundToInt ((1.0f - norm) * (float) filled.getHeight()));
        g.setColour (theme.accent);
        g.fillRect (filled);
    }
}

void ChannelStrip::resized()
{
    auto& metrics = themeManager.getMetrics();
    auto r = getLocalBounds().reduced (metrics.inset);
    r.removeFromTop (metrics.trackControlHeight);   // the name

    r.removeFromTop (metrics.inset);
    auto buttons = r.removeFromTop (metrics.trackControlHeight);
    muteButton.setBounds (buttons.removeFromLeft (metrics.trackButtonWidth));
    buttons.removeFromLeft (metrics.inset);
    soloButton.setBounds (buttons.removeFromLeft (metrics.trackButtonWidth));

    r.removeFromTop (metrics.inset);
    pan.setBounds (r.removeFromTop (metrics.trackControlHeight).removeFromLeft (metrics.trackControlHeight));

    for (int i = (int) insertLabels.size(); --i >= 0;)
    {
        r.removeFromBottom (metrics.inset);
        insertLabels[(size_t) i]->setBounds (r.removeFromBottom (metrics.trackControlHeight));
    }

    for (int i = (int) sendRows.size(); --i >= 0;)
    {
        r.removeFromBottom (metrics.inset);
        sendRows[(size_t) i]->setBounds (r.removeFromBottom (metrics.trackControlHeight));
    }

    r.removeFromTop (metrics.inset);
    meterBounds = r.removeFromRight (metrics.trackButtonWidth);
    r.removeFromRight (metrics.inset);
    volume.setBounds (r);
}

} // namespace papercut
