#include "MixerView.h"
#include "Commands/MixerCommands.h"
#include "Commands/PluginCommands.h"

namespace papercut
{

namespace
{
    constexpr int stripsPadding = 12, groupGap = 14, stripGap = 6, masterWidth = 186;
}

MixerView::MixerView (ApplicationModel& m, Mixer& mx, PluginRack& p, CommandRegistry& c, ThemeManager& tm, juce::ValueTree uiState)
    : model (m), mixer (mx), plugins (p), commands (c), themeManager (tm), state (std::move (uiState)),
      addButton (tm, "Add Return, Bus or Send", Icon::plus, IconButton::Kind::small),
      meterMode (tm, { "Peak", "RMS", "LUFS" }),
      resetPeaks (tm, "Reset Peaks", Button::Variant::outline),
      master (c, tm)
{
    setComponentID (componentId);

    addButton.onClick = [this] { showAddMenu(); };

    for (auto& chip : sectionChips)
    {
        chip.button = std::make_unique<Chip> (themeManager, chip.name);
        chip.button->setToggleState (! (bool) state.getProperty ("hide_" + juce::String (chip.name), false), juce::dontSendNotification);
        chip.button->onClick = [this, &chip]
        {
            state.setProperty ("hide_" + juce::String (chip.name), ! chip.button->getToggleState(), nullptr);
            applySections();
        };

        // EQ and Comments have no strip section yet.
        chip.button->setEnabled (chip.section.has_value());
        chip.button->setTooltip (chip.section ? "Show or hide " + juce::String (chip.name) + " on every strip"
                                              : juce::String (chip.name) + ": not in the strip yet");
        addAndMakeVisible (*chip.button);
    }

    meterMode.setTitle ("Meter mode");
    meterMode.setSelectedIndex (juce::jlimit (0, 2, (int) state.getProperty ("meterMode", 0)), juce::dontSendNotification);
    meterMode.onChange = [this] (int index)
    {
        state.setProperty ("meterMode", index, nullptr);
        applyMeterMode();
    };

    resetPeaks.setTooltip ("Clear every peak hold");
    resetPeaks.onClick = [this]
    {
        for (auto& [id, strip] : strips)
            strip->resetPeaks();

        master.resetPeaks();
    };

    for (auto* child : std::initializer_list<juce::Component*> { &addButton, &meterMode, &resetPeaks, &viewport, &master })
        addAndMakeVisible (child);

    viewport.setViewedComponent (&stripsArea, false);
    viewport.setScrollBarsShown (false, true);
    viewport.setScrollBarThickness (6);

    startTimerHz (30);
    model.addListener (this);
    themeManager.addListener (this);
    refresh();
    applyMeterMode();
}

MixerView::~MixerView()
{
    stopTimer();
    themeManager.removeListener (this);
    model.removeListener (this);
}

void MixerView::showAddMenu()
{
    juce::PopupMenu menu;
    menu.addItem ("Add Return", [this]
    {
        commands.invoke ("mixer.addReturn", returnArgs ("Return " + juce::String (mixer.getReturns().size() + 1)));
    });
    menu.addItem ("Add Bus", [this]
    {
        commands.invoke ("mixer.addBus", busArgs ("Bus " + juce::String (mixer.getBuses().size() + 1)));
    });

    const auto trackId = targetTrackId();
    juce::PopupMenu sends, buses;

    for (auto& ret : mixer.getReturns())
        sends.addItem (ret.name, trackId.isNotEmpty(), false,
                       [this, trackId, bus = ret.bus] { commands.invoke ("mixer.addSend", sendArgs (trackId, bus)); });

    for (auto& bus : mixer.getBuses())
        buses.addItem (bus.name, trackId.isNotEmpty(), false,
                       [this, trackId, id = bus.trackId] { commands.invoke ("mixer.moveToBus", moveToBusArgs (trackId, id)); });

    menu.addSubMenu ("Add Send from Selected Track", sends, sends.getNumItems() > 0);
    menu.addSubMenu ("Move Selected Track to Bus", buses, buses.getNumItems() > 0);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&addButton));
}

void MixerView::showEffectPicker (const juce::String& trackId, InsertSlot& slot, const juce::String& replacing)
{
    // Mixer inserts take effects only (PRD §10.6).
    juce::PopupMenu builtIn, external;

    for (auto& info : plugins.getCatalogue())
    {
        if (info.instrument || info.midiEffect)
            continue;

        auto action = [this, trackId, replacing, path = info.path]
        {
            if (replacing.isNotEmpty())
                commands.invoke ("plugin.replace", pluginReplaceArgs (trackId, replacing, path));
            else
                commands.invoke ("plugin.insert", pluginInsertArgs (trackId, path, PluginChain::mixer));
        };

        (info.external ? external : builtIn).addItem (info.name, action);
    }

    juce::PopupMenu menu;
    menu.addSectionHeader (replacing.isNotEmpty() ? "Replace with" : "Add effect");
    menu.addSubMenu ("Papercut", builtIn);
    menu.addSubMenu ("Plug-Ins", external, external.getNumItems() > 0);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&slot));
}

void MixerView::applySections()
{
    for (auto& chip : sectionChips)
        if (chip.section)
            for (auto& [id, strip] : strips)
                strip->setSectionVisible (*chip.section, chip.button->getToggleState());
}

void MixerView::applyMeterMode()
{
    const auto mode = (MeterMode) meterMode.getSelectedIndex();
    mixer.setMeasuringRms (mode != MeterMode::peak);

    for (auto& [id, strip] : strips)
        strip->setMeterMode (mode);

    master.setMeterMode (mode);
}

void MixerView::setFlowStage (int stage)
{
    if (stage != flowStage)
    {
        flowStage = stage;
        repaint (flowIndicator);
    }
}

juce::String MixerView::targetTrackId() const
{
    if (auto id = model.getSelectedTrackId(); id.isNotEmpty())
        return id;

    return trackOrder.empty() ? juce::String() : trackOrder.front();
}

void MixerView::timerCallback()
{
    // Hidden strips don't meter (PRD §19).
    if (! isShowing())
    {
        lastMeterTime = 0;
        return;
    }

    const auto now = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    const auto elapsed = lastMeterTime > 0 ? now - lastMeterTime : 0.0;
    lastMeterTime = now;

    for (auto& [id, strip] : strips)
        strip->setLevel (mixer.getTrackLevel (id), elapsed);

    master.setLevel (mixer.getMasterLevel(), elapsed);
}

void MixerView::refresh()
{
    const auto tracks = model.getTracks();
    const auto returns = mixer.getReturns();
    const auto buses = mixer.getBuses();
    const auto inputs = model.getAudioInputs();

    trackOrder.clear();
    returnOrder.clear();
    std::map<juce::String, std::unique_ptr<ChannelStrip>> kept;

    for (size_t i = 0; i < tracks.size(); ++i)
    {
        auto& track = tracks[i];
        StripState stripState;
        stripState.number = (int) i + 1;
        stripState.track = track;
        stripState.sends = mixer.getSends (track.id);
        stripState.inserts = plugins.getChain (track.id, PluginChain::mixer);
        stripState.deviceChain = plugins.getChain (track.id, PluginChain::device);
        stripState.inputs = inputs;

        for (auto& bus : buses)
            if (std::find (bus.childTrackIds.begin(), bus.childTrackIds.end(), track.id) != bus.childTrackIds.end())
                stripState.output = bus.name;

        for (auto& ret : returns)
            if (ret.trackId == track.id)
            {
                stripState.isReturn = true;
                stripState.returnLetter = juce::String::charToString ((juce::juce_wchar) ('A' + juce::jlimit (0, 25, ret.bus)));
            }

        (stripState.isReturn ? returnOrder : trackOrder).push_back (track.id);

        auto existing = strips.find (track.id);
        auto strip = existing != strips.end() ? std::move (existing->second) : nullptr;

        if (strip == nullptr)
        {
            strip = std::make_unique<ChannelStrip> (commands, themeManager);
            strip->onTrackChainClicked = [this, id = track.id] { if (onShowDeviceChain) onShowDeviceChain (id); };
            strip->onFlowStageHovered = [this] (int stage) { setFlowStage (stage); };
            strip->onOpenPlugin = [this] (const juce::String& pluginId) { if (onOpenPlugin) onOpenPlugin (pluginId); };
            strip->onPickInsert = [this, id = track.id] (InsertSlot& slot, const juce::String& replacing)
            {
                showEffectPicker (id, slot, replacing);
            };
            strip->setMeterMode ((MeterMode) meterMode.getSelectedIndex());

            for (auto& chip : sectionChips)
                if (chip.section)
                    strip->setSectionVisible (*chip.section, chip.button->getToggleState());

            stripsArea.addAndMakeVisible (*strip);
        }

        strip->setState (stripState);
        kept[track.id] = std::move (strip);
    }

    strips = std::move (kept);
    master.setMaster (mixer.getMaster());
    layoutStrips();
}

void MixerView::layoutStrips()
{
    auto& metrics = themeManager.getMetrics();
    const auto height = juce::jmax (0, viewport.getHeight() - viewport.getScrollBarThickness());
    auto x = stripsPadding;

    auto place = [&] (const std::vector<juce::String>& ids)
    {
        for (auto& id : ids)
            if (auto strip = strips.find (id); strip != strips.end())
            {
                strip->second->setBounds (x, stripsPadding, metrics.stripWidth, juce::jmax (0, height - 2 * stripsPadding));
                x += metrics.stripWidth + stripGap;
            }
    };

    place (trackOrder);

    if (! returnOrder.empty())
    {
        x += groupGap - stripGap;
        place (returnOrder);
    }

    stripsArea.setSize (juce::jmax (viewport.getWidth(), x - stripGap + stripsPadding), height);
}

void MixerView::paint (juce::Graphics& g)
{
    auto& theme = themeManager.getTheme();
    g.fillAll (theme.bgDeep);

    auto toolbar = getLocalBounds().removeFromTop (themeManager.getMetrics().toolbarHeight);
    g.setColour (theme.bgPanel);
    g.fillRect (toolbar);
    g.setColour (theme.borderSoft);
    g.fillRect (toolbar.removeFromBottom (1));

    drawStyledText (g, themeManager, "Mixer", theme.heading, titleArea, juce::Justification::centredLeft, theme.textPrimary);

    // Signal flow: Track chain › Inserts › Sends › Fader, the stage under the pointer in lime.
    const char* const stages[] = { "Track chain", "Inserts", "Sends", "Fader" };
    const auto font = themeManager.font (theme.bodySm);
    const auto chevron = juce::String (juce::CharPointer_UTF8 ("  \xe2\x80\xba  "));
    auto r = flowIndicator;

    for (int i = 0; i < 4; ++i)
    {
        const auto text = juce::String (stages[i]);
        g.setFont (i == flowStage ? themeManager.font (TypeStyle { theme.bodySm.size, false, 600 }) : font);
        g.setColour (i == flowStage ? theme.accent : theme.textSecondary);
        g.drawText (text, r.removeFromLeft (juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), text) + 1),
                    juce::Justification::centredLeft, false);

        if (i < 3)
        {
            g.setFont (font);
            g.setColour (theme.textDim);
            g.drawText (chevron, r.removeFromLeft (juce::GlyphArrangement::getStringWidthInt (font, chevron)),
                        juce::Justification::centredLeft, false);
        }
    }
}

void MixerView::resized()
{
    auto& metrics = themeManager.getMetrics();
    auto& theme = themeManager.getTheme();
    auto r = getLocalBounds();
    auto toolbar = r.removeFromTop (metrics.toolbarHeight).reduced (metrics.space2xl, 0);
    const auto rowHeight = metrics.controlMd;
    auto centred = [&] (juce::Rectangle<int> area) { return area.withSizeKeepingCentre (area.getWidth(), rowHeight); };

    titleArea = toolbar.removeFromLeft (juce::GlyphArrangement::getStringWidthInt (themeManager.font (theme.heading), "Mixer") + 4);
    toolbar.removeFromLeft (metrics.spaceSm);
    addButton.setBounds (centred (toolbar.removeFromLeft (24)).withSizeKeepingCentre (24, 24));
    toolbar.removeFromLeft (14);

    for (auto& chip : sectionChips)
    {
        chip.button->setBounds (centred (toolbar.removeFromLeft (chip.button->getIdealWidth())));
        toolbar.removeFromLeft (metrics.spaceSm);
    }

    resetPeaks.setBounds (centred (toolbar.removeFromRight (resetPeaks.getIdealWidth())));
    toolbar.removeFromRight (metrics.spaceXl);
    meterMode.setBounds (centred (toolbar.removeFromRight (meterMode.getIdealWidth())));
    toolbar.removeFromRight (metrics.spaceXl);

    const auto flowWidth = juce::jmin (toolbar.getWidth(), 280);
    flowIndicator = toolbar.removeFromRight (flowWidth);

    master.setBounds (r.removeFromRight (masterWidth + stripsPadding).reduced (0, stripsPadding).withTrimmedRight (stripsPadding));
    viewport.setBounds (r);
    layoutStrips();
}

} // namespace papercut
