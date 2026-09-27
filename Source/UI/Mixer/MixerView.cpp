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
    addReturnButton.onClick = [this]
    {
        commands.invoke ("mixer.addReturn", returnArgs ("Return " + juce::String (mixer.getReturns().size() + 1)));
    };
    addBusButton.onClick = [this]
    {
        commands.invoke ("mixer.addBus", busArgs ("Bus " + juce::String (mixer.getBuses().size() + 1)));
    };

    for (auto* child : std::initializer_list<juce::Component*> { &addReturnButton, &addBusButton, &master })
        addAndMakeVisible (child);

    model.addListener (this);
    themeManager.addListener (this);
    refresh();
}

MixerView::~MixerView()
{
    themeManager.removeListener (this);
    model.removeListener (this);
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

    for (auto* button : { &addReturnButton, &addBusButton })
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
    addReturnButton.setBounds (bar.removeFromLeft (metrics.trackHeaderWidth));
    bar.removeFromLeft (metrics.inset);
    addBusButton.setBounds (bar.removeFromLeft (metrics.trackHeaderWidth));

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
