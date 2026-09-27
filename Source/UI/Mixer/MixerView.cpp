#include "MixerView.h"
#include "Commands/MixerCommands.h"

namespace papercut
{

MixerView::MixerView (ApplicationModel& m, Mixer& mx, CommandRegistry& c, ThemeManager& tm)
    : model (m), mixer (mx), commands (c), themeManager (tm), master (c, tm)
{
    setComponentID (componentId);

    addReturnButton.setTooltip ("Add a return");
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

    for (auto* child : std::initializer_list<juce::Component*> { &addReturnButton, &addBusButton, &addSendButton, &toBusButton, &master })
        addAndMakeVisible (child);

    startTimerHz (15);

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

    return tracks.empty() ? juce::String() : tracks.front().id;
}

void MixerView::timerCallback()
{
    for (auto& track : tracks)
        if (auto strip = strips.find (track.id); strip != strips.end())
            strip->second->setLevelDb (mixer.getTrackLevelDb (track.id));

    master.setLevelDb (mixer.getMasterLevelDb());
}

void MixerView::refresh()
{
    tracks = model.getTracks();
    std::map<juce::String, std::unique_ptr<ChannelStrip>> kept;

    for (auto& track : tracks)
    {
        const auto sends = mixer.getSends (track.id);
        const auto inserts = mixer.getInserts (track.id);

        if (auto existing = strips.find (track.id); existing != strips.end())
        {
            existing->second->setState (track, sends, inserts);
            kept[track.id] = std::move (existing->second);
        }
        else
        {
            auto strip = std::make_unique<ChannelStrip> (commands, themeManager, track, sends, inserts);
            addAndMakeVisible (*strip);
            kept[track.id] = std::move (strip);
        }
    }

    strips = std::move (kept);
    master.setMaster (mixer.getMaster());
    applyTheme();
    resized();
}

void MixerView::applyTheme()
{
    auto& theme = themeManager.getTheme();

    for (auto* button : { &addReturnButton, &addBusButton, &addSendButton, &toBusButton })
    {
        button->setColour (juce::TextButton::buttonColourId, theme.trackHeader);
        button->setColour (juce::TextButton::textColourOffId, theme.text);
    }

    for (auto& [id, strip] : strips)
        strip->applyTheme();

    master.applyTheme();
    repaint();
}

void MixerView::themeChanged()
{
    applyTheme();
}

void MixerView::paint (juce::Graphics& g)
{
    g.fillAll (themeManager.getTheme().background);
}

void MixerView::resized()
{
    auto& metrics = themeManager.getMetrics();
    auto r = getLocalBounds().reduced (metrics.inset);
    auto bar = r.removeFromTop (metrics.trackControlHeight);
    for (auto* button : { &addReturnButton, &addBusButton, &addSendButton, &toBusButton })
    {
        button->setBounds (bar.removeFromLeft (metrics.trackHeaderWidth));
        bar.removeFromLeft (metrics.inset);
    }

    r.removeFromTop (metrics.inset);

    auto place = [&] (juce::Component& component)
    {
        component.setBounds (r.removeFromLeft (metrics.trackHeaderWidth).withHeight (r.getHeight()));
        r.removeFromLeft (metrics.inset);
    };

    for (auto& track : tracks)
        if (auto strip = strips.find (track.id); strip != strips.end())
            place (*strip->second);

    place (master);
}

} // namespace papercut
