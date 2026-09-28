#include "MixerView.h"
#include "Commands/MixerCommands.h"

namespace papercut
{

namespace
{
    constexpr int stripsPadding = 12, groupGap = 14, stripGap = 6, masterWidth = 186;
}

MixerView::MixerView (ApplicationModel& m, Mixer& mx, PluginRack& p, CommandRegistry& c, ThemeManager& tm)
    : model (m), mixer (mx), plugins (p), commands (c), themeManager (tm),
      addReturnButton (tm, "Add Return", Button::Variant::ghost, Icon::plus),
      addBusButton (tm, "Add Bus", Button::Variant::ghost, Icon::plus),
      addSendButton (tm, "Add Send", Button::Variant::ghost, Icon::plus),
      toBusButton (tm, "To Bus", Button::Variant::ghost),
      master (c, tm)
{
    setComponentID (componentId);

    addReturnButton.setTooltip ("Add a return (Mod+Alt+T)");
    addBusButton.setTooltip ("Add a bus");
    addSendButton.setTooltip ("Add a send from the selected track to a return");
    toBusButton.setTooltip ("Move the selected track into a bus");
    addReturnButton.onClick = [this]
    {
        commands.invoke ("mixer.addReturn", returnArgs ("Return " + juce::String (mixer.getReturns().size() + 1)));
    };
    addBusButton.onClick = [this]
    {
        commands.invoke ("mixer.addBus", busArgs ("Bus " + juce::String (mixer.getBuses().size() + 1)));
    };
    addSendButton.onClick = [this]
    {
        const auto trackId = targetTrackId();
        const auto returns = mixer.getReturns();

        if (trackId.isEmpty())
            return;

        if (returns.size() <= 1)
        {
            const int bus = returns.empty() ? 0 : returns.front().bus;
            commands.invoke ("mixer.addSend", sendArgs (trackId, bus));
            return;
        }

        juce::PopupMenu menu;

        for (int i = 0; i < (int) returns.size(); ++i)
            menu.addItem (i + 1, returns[(size_t) i].name);

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&addSendButton),
                            [this, trackId, returns] (int result)
                            {
                                if (result > 0 && result <= (int) returns.size())
                                    commands.invoke ("mixer.addSend", sendArgs (trackId, returns[(size_t) result - 1].bus));
                            });
    };
    toBusButton.onClick = [this]
    {
        const auto trackId = targetTrackId();
        const auto buses = mixer.getBuses();

        if (trackId.isEmpty() || buses.empty())
            return;

        if (buses.size() == 1)
        {
            commands.invoke ("mixer.moveToBus", moveToBusArgs (trackId, buses.front().trackId));
            return;
        }

        juce::PopupMenu menu;

        for (int i = 0; i < (int) buses.size(); ++i)
            menu.addItem (i + 1, buses[(size_t) i].name);

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&toBusButton),
                            [this, trackId, buses] (int result)
                            {
                                if (result > 0 && result <= (int) buses.size())
                                    commands.invoke ("mixer.moveToBus", moveToBusArgs (trackId, buses[(size_t) result - 1].trackId));
                            });
    };

    for (auto* child : std::initializer_list<juce::Component*> { &addReturnButton, &addBusButton, &addSendButton, &toBusButton,
                                                                 &viewport, &master })
        addAndMakeVisible (child);

    viewport.setViewedComponent (&stripsArea, false);
    viewport.setScrollBarsShown (false, true);
    viewport.setScrollBarThickness (6);

    startTimerHz (30);
    model.addListener (this);
    themeManager.addListener (this);
    refresh();
}

MixerView::~MixerView()
{
    stopTimer();
    themeManager.removeListener (this);
    model.removeListener (this);
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
        StripState state;
        state.number = (int) i + 1;
        state.track = track;
        state.sends = mixer.getSends (track.id);
        state.inserts = plugins.getChain (track.id, PluginChain::mixer);
        state.deviceChain = plugins.getChain (track.id, PluginChain::device);
        state.inputs = inputs;

        for (auto& bus : buses)
            if (std::find (bus.childTrackIds.begin(), bus.childTrackIds.end(), track.id) != bus.childTrackIds.end())
                state.output = bus.name;

        for (auto& ret : returns)
            if (ret.trackId == track.id)
            {
                state.isReturn = true;
                state.returnLetter = juce::String::charToString ((juce::juce_wchar) ('A' + juce::jlimit (0, 25, ret.bus)));
            }

        (state.isReturn ? returnOrder : trackOrder).push_back (track.id);

        auto existing = strips.find (track.id);
        auto strip = existing != strips.end() ? std::move (existing->second) : nullptr;

        if (strip == nullptr)
        {
            strip = std::make_unique<ChannelStrip> (commands, themeManager);
            strip->onTrackChainClicked = [this, id = track.id] { if (onShowDeviceChain) onShowDeviceChain (id); };
            stripsArea.addAndMakeVisible (*strip);
        }

        strip->setState (state);
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
}

void MixerView::resized()
{
    auto& metrics = themeManager.getMetrics();
    auto r = getLocalBounds();
    auto toolbar = r.removeFromTop (metrics.toolbarHeight).reduced (metrics.space2xl, 7);

    for (auto* b : { &addReturnButton, &addBusButton, &addSendButton, &toBusButton })
    {
        b->setBounds (toolbar.removeFromLeft (b->getIdealWidth()));
        toolbar.removeFromLeft (metrics.spaceSm);
    }

    master.setBounds (r.removeFromRight (masterWidth + stripsPadding).reduced (0, stripsPadding).withTrimmedRight (stripsPadding));
    viewport.setBounds (r);
    layoutStrips();
}

} // namespace papercut
