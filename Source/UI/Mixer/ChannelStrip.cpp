#include "ChannelStrip.h"
#include "Commands/AppCommands.h"
#include "Commands/MixerCommands.h"

namespace papercut
{

namespace
{
    constexpr int padX = 10, sectionPadY = 8, labelHeight = 11, rowGap = 4, slotHeight = 19, selectHeight = 20,
                  chainLinkHeight = 22, flowHeight = 10, sendHeight = 18, panHeight = 50, readoutHeight = 20,
                  buttonHeight = 20, meterWidth = 17;

    /** Signal-flow stages, as the mixer toolbar names them. */
    enum Stage { trackChainStage, insertsStage, sendsStage, faderStage };

    ContinuousValue::Spec panSpec()
    {
        ContinuousValue::Spec spec;
        spec.minimum = -1.0;
        spec.maximum = 1.0;
        spec.format = ValueFormat::pan();
        return spec;
    }

    ContinuousValue::Spec gainSpec()
    {
        ContinuousValue::Spec spec;
        spec.minimum = ApplicationModel::minVolumeDb;
        spec.maximum = ApplicationModel::maxVolumeDb;
        spec.format = ValueFormat::decibels (ApplicationModel::minVolumeDb);
        spec.wheelStep = 0.5;
        return spec;
    }

    juce::String twoDigits (int n)   { return n < 10 ? "0" + juce::String (n) : juce::String (n); }
}

//==============================================================================
/** A send: its return's letter (click: mute), a level bar, the level. */
struct ChannelStrip::SendRow : juce::Component
{
    SendRow (ThemeManager& tm, CommandRegistry& c) : themeManager (tm), commands (c), level (tm, sendSpec())
    {
        level.setTitle ("Send level");
        addAndMakeVisible (level);
    }

    static ContinuousValue::Spec sendSpec()
    {
        auto spec = gainSpec();
        spec.toProportion = [] (double db) { return 1.0 - FaderLaw::dbToTravel (db); };
        spec.fromProportion = [] (double p) { return FaderLaw::travelToDb (1.0 - p); };
        return spec;
    }

    void paint (juce::Graphics& g) override
    {
        auto& theme = themeManager.getTheme();
        auto badge = getLocalBounds().removeFromLeft (14).withSizeKeepingCentre (14, 14);
        g.setColour (send.muted || send.gainDb <= ApplicationModel::minVolumeDb ? theme.textDim : theme.returnColours[0]);
        g.fillRoundedRectangle (badge.toFloat(), theme.radiusSm);
        drawStyledText (g, themeManager, letter, TypeStyle { 8.0f, false, 700 }, badge, juce::Justification::centred,
                        theme.textOnAccent);
        drawNumber (g, themeManager, send.muted ? juce::String ("off") : juce::String (send.gainDb, 1),
                    TypeStyle { 9.0f, true, 400 }, getLocalBounds().removeFromRight (30), juce::Justification::centredRight,
                    theme.textSecondary);
    }

    void resized() override
    {
        level.setBounds (getLocalBounds().withTrimmedLeft (18).withTrimmedRight (34));
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.x < 16)
            commands.invoke ("mixer.setSendMuted", sendMutedArgs (trackId, send.id, ! send.muted));
    }

    ThemeManager& themeManager;
    CommandRegistry& commands;
    Slider level;
    SendInfo send;
    juce::String trackId, letter;
};

//==============================================================================
ChannelStrip::ChannelStrip (CommandRegistry& c, ThemeManager& tm)
    : commands (c), themeManager (tm),
      pan (tm, panSpec(), "Pan", true), gain (tm, gainSpec()), fader (tm), meter (tm),
      mute (tm, TrackButton::Kind::mute), solo (tm, TrackButton::Kind::solo), arm (tm, TrackButton::Kind::arm)
{
    input.setTitle ("Input");
    input.onChange = [this]
    {
        const auto index = input.getSelectedItemIndex();
        const auto chosen = index > 0 ? state.inputs[index - 1] : juce::String();

        if (chosen != state.track.input)
            commands.invoke ("track.setInput", trackInputArgs (state.track.id, chosen));
    };

    pan.setDialSize (22);
    pan.onChange = [this] (double v, bool continues) { commands.invoke ("track.setPan", trackPanArgs (state.track.id, v, continues)); };

    auto setVolume = [this] (double db, bool continues)
    {
        commands.invoke ("track.setVolume", trackVolumeArgs (state.track.id, db, continues));
    };
    fader.onChange = setVolume;
    gain.onChange = setVolume;
    gain.setTitle ("Gain");
    gain.setTooltip ("Gain: click to type");
    gain.setDoubleClickEdits (false);

    meter.onPeaksReset = [this] { repaint (peakReadout); };

    mute.onClick = [this] { commands.invoke ("track.toggleMute", trackArgs (state.track.id)); };
    solo.onClick = [this] { commands.invoke ("track.toggleSolo", trackArgs (state.track.id)); };
    arm.onClick = [this] { commands.invoke ("track.toggleArm", trackArgs (state.track.id)); };

    for (auto* child : std::initializer_list<juce::Component*> { &input, &pan, &gain, &fader, &meter, &mute, &solo, &arm })
        addAndMakeVisible (child);

    for (int i = 0; i < PluginRack::maxMixerInserts; ++i)
    {
        insertSlots.push_back (std::make_unique<InsertSlot> (themeManager, i));
        addChildComponent (*insertSlots.back());
    }
}

ChannelStrip::~ChannelStrip() = default;

void ChannelStrip::setState (const StripState& next)
{
    state = next;
    auto& palette = themeManager.getTheme().trackPalette;
    colour = state.isReturn ? themeManager.getTheme().returnColours[0]
                            : palette[(size_t) juce::jlimit (0, (int) palette.size() - 1, state.track.colourIndex)];

    setTitle (state.track.name);
    setTooltip (state.track.name);

    // Input choices: "No Input" then each audio input.
    input.clear (juce::dontSendNotification);
    input.addItem ("No Input", 1);

    for (int i = 0; i < state.inputs.size(); ++i)
        input.addItem (state.inputs[i], i + 2);

    input.setSelectedItemIndex (state.track.input.isEmpty() ? 0 : state.inputs.indexOf (state.track.input) + 1,
                                juce::dontSendNotification);
    input.setEnabled (state.track.kind == TrackKind::audio && ! state.isReturn);

    pan.setValue (state.track.pan);
    fader.setValue (state.track.volumeDb);
    fader.setColour (colour);
    gain.setValue (state.track.volumeDb);

    mute.setToggleState (state.track.muted, juce::dontSendNotification);
    solo.setToggleState (state.track.solo, juce::dontSendNotification);
    arm.setToggleState (state.track.armed, juce::dontSendNotification);
    arm.setVisible (! state.isReturn && state.track.kind == TrackKind::audio);

    rebuildSends();
    rebuildInsertSlots();
    resized();
    repaint();
}

void ChannelStrip::rebuildSends()
{
    const auto showSends = ! state.isReturn;

    if (sendRows.size() != (showSends ? state.sends.size() : 0))
    {
        sendRows.clear();

        for (size_t i = 0; showSends && i < state.sends.size(); ++i)
        {
            auto row = std::make_unique<SendRow> (themeManager, commands);
            row->level.onChange = [this, i] (double db, bool continues)
            {
                if (i < state.sends.size())
                    commands.invoke ("mixer.setSendGain", sendGainArgs (state.track.id, state.sends[i].id, db, continues));
            };
            addAndMakeVisible (*row);
            sendRows.push_back (std::move (row));
        }
    }

    for (size_t i = 0; i < sendRows.size(); ++i)
    {
        auto& row = *sendRows[i];
        row.send = state.sends[i];
        row.trackId = state.track.id;
        row.letter = juce::String::charToString ((juce::juce_wchar) ('A' + juce::jlimit (0, 25, state.sends[i].bus)));
        row.level.setValue (state.sends[i].gainDb);
        row.repaint();
    }
}

void ChannelStrip::rebuildInsertSlots()
{
    for (size_t i = 0; i < insertSlots.size(); ++i)
        insertSlots[i]->setPlugin (i < state.inserts.size() ? std::optional<PluginInfo> (state.inserts[i]) : std::nullopt);
}

juce::String ChannelStrip::chainSummary() const
{
    juce::StringArray names;

    for (auto& device : state.deviceChain)
        names.add (device.name);

    return names.isEmpty() ? juce::String ("Empty") : names.joinIntoString (juce::String (juce::CharPointer_UTF8 (" \xe2\x80\xba ")));
}

void ChannelStrip::setLevel (StereoLevel level, double elapsedSeconds)
{
    const auto peakBefore = meter.getPeakDb();
    meter.setLevel (level, elapsedSeconds);

    if (std::abs (meter.getPeakDb() - peakBefore) > 0.05)
        repaint (peakReadout);
}

void ChannelStrip::resetPeaks()
{
    meter.resetPeaks();
    repaint (peakReadout);
}

void ChannelStrip::setSectionVisible (Section section, bool visible)
{
    sectionShown[(size_t) section] = visible;
    resized();
    repaint();
}

//==============================================================================
void ChannelStrip::resized()
{
    auto r = getLocalBounds();
    headArea = r.removeFromTop (3 + 2 * sectionPadY + 14);

    auto section = [&] (bool visible, int contentHeight)
    {
        if (! visible)
            return juce::Rectangle<int>();

        return r.removeFromTop (2 * sectionPadY + labelHeight + rowGap + contentHeight);
    };

    auto content = [] (juce::Rectangle<int> area)
    {
        return area.reduced (padX, sectionPadY).withTrimmedTop (labelHeight + rowGap);
    };

    // I/O
    ioArea = section (shown (Section::io), 2 * selectHeight + rowGap);
    input.setVisible (shown (Section::io));

    if (shown (Section::io))
        input.setBounds (content (ioArea).removeFromTop (selectHeight));

    // Track chain (read-only) and the flow arrow into the inserts.
    chainArea = section (shown (Section::inserts), chainLinkHeight);
    chainLink = shown (Section::inserts) ? content (chainArea) : juce::Rectangle<int>();
    flowArea = shown (Section::inserts) ? r.removeFromTop (flowHeight) : juce::Rectangle<int>();

    // Mixer inserts: 4 slots show; a fuller chain grows the section up to 8.
    const auto slots = juce::jlimit (visibleInsertSlots, PluginRack::maxMixerInserts, (int) state.inserts.size() + 1);
    insertsArea = section (shown (Section::inserts), slots * slotHeight + (slots - 1) * rowGap);

    for (int i = 0; i < (int) insertSlots.size(); ++i)
    {
        const auto visible = shown (Section::inserts) && i < slots;
        insertSlots[(size_t) i]->setVisible (visible);

        if (visible)
            insertSlots[(size_t) i]->setBounds (content (insertsArea).withTrimmedTop (i * (slotHeight + rowGap)).withHeight (slotHeight));
    }

    // Sends
    const auto sendCount = (int) sendRows.size();
    sendsArea = section (shown (Section::sends) && sendCount > 0, sendCount * sendHeight + juce::jmax (0, sendCount - 1) * rowGap);

    for (int i = 0; i < sendCount; ++i)
    {
        sendRows[(size_t) i]->setVisible (shown (Section::sends));
        sendRows[(size_t) i]->setBounds (content (sendsArea).withTrimmedTop (i * (sendHeight + rowGap)).withHeight (sendHeight));
    }

    // Buttons at the bottom; pan and the fader section take the rest.
    buttonsArea = r.removeFromBottom (buttonHeight + padX);
    auto buttons = buttonsArea.reduced (padX, 0).withTrimmedBottom (padX);
    const auto buttonCount = arm.isVisible() ? 3 : 2;
    const auto buttonWidth = (buttons.getWidth() - (buttonCount - 1) * rowGap) / buttonCount;

    for (auto* b : { static_cast<juce::Component*> (&mute), static_cast<juce::Component*> (&solo), static_cast<juce::Component*> (&arm) })
    {
        if (! b->isVisible())
            continue;

        b->setBounds (buttons.removeFromLeft (buttonWidth));
        buttons.removeFromLeft (rowGap);
    }

    panArea = r.removeFromTop (panHeight + sectionPadY);
    pan.setBounds (panArea.reduced (padX, 4).removeFromLeft (80).withHeight (panHeight));

    faderArea = r;
    const auto showFader = shown (Section::fader);

    for (auto* c : std::initializer_list<juce::Component*> { &gain, &fader, &meter })
        c->setVisible (showFader);

    if (showFader)
    {
        auto f = faderArea.reduced (padX, sectionPadY);
        auto readouts = f.removeFromTop (readoutHeight);
        gain.setBounds (readouts.removeFromLeft ((readouts.getWidth() - rowGap) / 2));
        readouts.removeFromLeft (rowGap);
        peakReadout = readouts;
        f.removeFromTop (8);

        auto meterColumn = f.removeFromRight (meterWidth);
        f.removeFromRight (6);
        fader.setBounds (f);
        meter.setBounds (meterColumn.withY (fader.getY() + fader.getTravelBounds().getY())
                                    .withHeight (fader.getTravelBounds().getHeight()));
    }
}

void ChannelStrip::paintSectionHeader (juce::Graphics& g, juce::Rectangle<int> area, const juce::String& title,
                                       const juce::String& tag, juce::Colour tagColour) const
{
    auto& theme = themeManager.getTheme();
    auto row = area.reduced (padX, sectionPadY).removeFromTop (labelHeight);
    const auto labelStyle = TypeStyle { 8.5f, false, 600, true, 0.5f };
    drawStyledText (g, themeManager, title, labelStyle, row, juce::Justification::centredLeft, theme.textDim);

    if (tag.isNotEmpty())
    {
        const auto width = juce::GlyphArrangement::getStringWidthInt (themeManager.font (theme.micro), theme.micro.apply (tag)) + 8;
        auto badge = row.removeFromRight (width);
        g.setColour (tagColour.withAlpha (0.15f));
        g.fillRoundedRectangle (badge.toFloat(), theme.radiusSm);
        drawStyledText (g, themeManager, tag, theme.micro, badge, juce::Justification::centred, tagColour);
    }
}

void ChannelStrip::paint (juce::Graphics& g)
{
    auto& theme = themeManager.getTheme();
    const auto bounds = getLocalBounds().toFloat();
    const auto radius = theme.radiusLg;

    g.setColour (state.track.selected ? theme.bgElevated : theme.bgTrack);
    g.fillRoundedRectangle (bounds, radius);

    // Head: colour bar, number in the track colour, name (ellipsis; the tooltip has it all).
    {
        juce::Graphics::ScopedSaveState save (g);
        juce::Path shape;
        shape.addRoundedRectangle (bounds, radius);
        g.reduceClipRegion (shape);
        g.setColour (colour);
        g.fillRect (headArea.withHeight (3));
    }

    auto head = headArea.withTrimmedTop (3).reduced (padX, sectionPadY);
    const auto number = state.isReturn ? state.returnLetter : twoDigits (state.number);
    const auto numberFont = themeManager.numberFont (TypeStyle { 10.0f, true, 600 });
    g.setFont (numberFont);
    g.setColour (colour);
    g.drawText (number, head.removeFromLeft (juce::GlyphArrangement::getStringWidthInt (numberFont, number)),
                juce::Justification::centredLeft, false);
    head.removeFromLeft (7);
    drawStyledText (g, themeManager, state.track.name, TypeStyle { 12.0f, false, 600 }, head,
                    juce::Justification::centredLeft, theme.textPrimary);

    auto divider = [&] (juce::Rectangle<int> area)
    {
        if (! area.isEmpty())
        {
            g.setColour (theme.borderSoft);
            g.fillRect (area.getX(), area.getY(), area.getWidth(), 1);
        }
    };

    // I/O: the input select draws itself; the output is read-only for now.
    if (! ioArea.isEmpty())
    {
        divider (ioArea);
        paintSectionHeader (g, ioArea, "I/O", {}, {});
        auto out = ioArea.reduced (padX, sectionPadY).withTrimmedTop (labelHeight + rowGap + selectHeight + rowGap).withHeight (selectHeight);
        g.setColour (theme.bgSlot);
        g.fillRoundedRectangle (out.toFloat(), theme.radiusMd);
        drawStyledText (g, themeManager, juce::String (juce::CharPointer_UTF8 ("\xe2\x86\x92 ")) + state.output, theme.bodySm,
                        out.reduced (7, 0), juce::Justification::centredLeft, theme.textPrimary);
    }

    // Track chain: a read-only summary of the device chain; click opens it.
    if (! chainArea.isEmpty())
    {
        divider (chainArea);
        paintSectionHeader (g, chainArea, "Track chain", "Racks", theme.textSecondary);
        const auto hovered = chainLink.contains (getMouseXYRelative()) && isMouseOver (true);
        g.setColour (hovered ? theme.bgHover : theme.bgSlot);
        g.fillRoundedRectangle (chainLink.toFloat(), theme.radiusMd);
        g.setColour (theme.borderSoft);
        g.drawRoundedRectangle (chainLink.toFloat().reduced (0.5f), theme.radiusMd, 1.0f);

        auto link = chainLink.reduced (6, 0);
        g.setColour (colour);
        g.fillRoundedRectangle (link.removeFromLeft (2).withSizeKeepingCentre (2, 12).toFloat(), 1.0f);
        link.removeFromLeft (5);
        drawIcon (g, Icon::layers, link.removeFromLeft (10).toFloat().withSizeKeepingCentre (10.0f, 10.0f), theme.textSecondary);
        drawIcon (g, Icon::arrowUpRight, link.removeFromRight (10).toFloat().withSizeKeepingCentre (10.0f, 10.0f), theme.textDim);
        link.reduce (5, 0);
        drawStyledText (g, themeManager, chainSummary(), TypeStyle { 9.5f, false, 400 }, link, juce::Justification::centredLeft,
                        state.deviceChain.empty() ? theme.textDim : theme.textPrimary);
    }

    if (! flowArea.isEmpty())
        drawIcon (g, Icon::arrowDown, flowArea.toFloat().withSizeKeepingCentre (9.0f, 9.0f), theme.textDim);

    if (! insertsArea.isEmpty())
        paintSectionHeader (g, insertsArea, "Mixer inserts", "Post", theme.accent);

    if (! sendsArea.isEmpty())
    {
        divider (sendsArea);
        paintSectionHeader (g, sendsArea, "Sends", {}, {});
    }

    divider (panArea);

    if (shown (Section::fader))
    {
        divider (faderArea);

        // Peak readout: red above -1.5 dBFS; click resets.
        const auto peak = meter.getPeakDb();
        g.setColour (theme.bgSlot);
        g.fillRoundedRectangle (peakReadout.toFloat(), theme.radiusMd);
        drawNumber (g, themeManager, peak <= FaderLaw::floorDb ? juce::String (juce::CharPointer_UTF8 ("-\xe2\x88\x9e")) : juce::String (peak, 1),
                    TypeStyle { 10.0f, true, 400 }, peakReadout, juce::Justification::centred,
                    peak > -1.5 ? theme.meterHigh : theme.textSecondary);
    }
}

int ChannelStrip::flowStageAt (juce::Point<int> p) const
{
    if (chainArea.contains (p) || flowArea.contains (p))  return trackChainStage;
    if (insertsArea.contains (p))                          return insertsStage;
    if (sendsArea.contains (p))                            return sendsStage;
    if (panArea.contains (p) || faderArea.contains (p))    return faderStage;
    return -1;
}

void ChannelStrip::mouseMove (const juce::MouseEvent& e)
{
    repaint (chainLink);

    if (onFlowStageHovered)
        onFlowStageHovered (flowStageAt (e.getEventRelativeTo (this).getPosition()));
}

void ChannelStrip::mouseExit (const juce::MouseEvent&)
{
    repaint (chainLink);

    if (onFlowStageHovered)
        onFlowStageHovered (-1);
}

void ChannelStrip::mouseDown (const juce::MouseEvent& e)
{
    if (chainLink.contains (e.getPosition()))
    {
        if (onTrackChainClicked)
            onTrackChainClicked();

        return;
    }

    if (peakReadout.contains (e.getPosition()))
    {
        resetPeaks();
        return;
    }

    // Selecting a strip selects its track in every view (PRD §16.1).
    commands.invoke ("track.select", trackArgs (state.track.id));
}

} // namespace papercut
