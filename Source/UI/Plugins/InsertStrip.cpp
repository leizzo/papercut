#include "InsertStrip.h"

#include "Commands/PluginCommands.h"

namespace papercut
{

class InsertRow : public juce::Component
{
public:
    InsertRow (InsertStrip& owner, const PluginInfo& info, bool isSelected)
        : strip (owner), pluginId (info.id), name (info.name), selected (isSelected)
    {
        remove.setButtonText ("Remove");
        remove.setTooltip ("Remove plug-in");
        remove.onClick = [this] { strip.removePlugin (pluginId); };
        addAndMakeVisible (remove);
        applyTheme();
    }

    void setSelected (bool now)
    {
        if (selected == now)
            return;

        selected = now;
        repaint();
    }

    void applyTheme()
    {
        auto& theme = strip.themeManager.getTheme();
        remove.setColour (juce::TextButton::buttonColourId, theme.panel);
        remove.setColour (juce::TextButton::textColourOffId, theme.text);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto& theme = strip.themeManager.getTheme();
        auto& metrics = strip.themeManager.getMetrics();

        g.fillAll (selected ? theme.trackHeaderSelected : theme.panel);
        g.setColour (theme.text);
        g.setFont (strip.themeManager.getFont());

        auto area = getLocalBounds().reduced (metrics.textPadding, 0);
        area.removeFromRight (remove.getWidth() + metrics.inset);
        g.drawText (name, area, juce::Justification::centredLeft, true);
    }

    void resized() override
    {
        auto& metrics = strip.themeManager.getMetrics();
        auto r = getLocalBounds().reduced (metrics.textPadding, 0);
        const auto width = juce::GlyphArrangement::getStringWidthInt (strip.themeManager.getFont(), remove.getButtonText())
                           + metrics.textPadding * 2;
        remove.setBounds (r.removeFromRight (width).reduced (0, metrics.inset));
    }

    void mouseDown (const juce::MouseEvent&) override
    {
        strip.selectPlugin (pluginId);
    }

    const juce::String& getPluginId() const noexcept { return pluginId; }

private:
    InsertStrip& strip;
    juce::String pluginId, name;
    juce::TextButton remove;
    bool selected = false;
};

//==============================================================================
InsertStrip::InsertStrip (CommandRegistry& c, PluginRack& r, ApplicationModel& m, ThemeManager& theme)
    : commands (c), rack (r), model (m), themeManager (theme)
{
    setComponentID ("InsertStrip");
    setLookAndFeel (&themeManager.getLookAndFeel());
    model.addListener (this);
    themeManager.addListener (this);
    refresh();
}

InsertStrip::~InsertStrip()
{
    themeManager.removeListener (this);
    model.removeListener (this);
    setLookAndFeel (nullptr);
}

void InsertStrip::modelChanged()
{
    refresh();
}

void InsertStrip::themeChanged()
{
    for (auto* row : rows)
        row->applyTheme();

    repaint();
}

void InsertStrip::paint (juce::Graphics& g)
{
    auto& theme = themeManager.getTheme();
    g.fillAll (theme.background);

    if (rows.isEmpty())
    {
        g.setColour (theme.mutedText);
        g.setFont (themeManager.getFont());
        g.drawText ("No inserts", getLocalBounds().reduced (themeManager.getMetrics().textPadding),
                    juce::Justification::centredLeft, true);
    }
}

void InsertStrip::resized()
{
    auto& metrics = themeManager.getMetrics();
    auto r = getLocalBounds();

    for (auto* row : rows)
    {
        row->setBounds (r.removeFromTop (metrics.trackControlHeight));

        if (r.getHeight() > 0)
            r.removeFromTop (metrics.inset);
    }
}

juce::String InsertStrip::currentTrackId() const
{
    const auto tracks = model.getTracks();

    for (const auto& track : tracks)
        if (track.selected)
            return track.id;

    return tracks.empty() ? juce::String() : tracks.front().id;
}

void InsertStrip::refresh()
{
    const auto trackId = currentTrackId();

    if (trackId != shownTrackId)
    {
        shownTrackId = trackId;
        selectedPluginId.clear();
    }

    const auto inserts = trackId.isNotEmpty() ? rack.getInserts (trackId) : std::vector<PluginInfo>{};
    auto stillThere = false;

    for (const auto& insert : inserts)
        if (insert.id == selectedPluginId)
            stillThere = true;

    if (! stillThere)
        selectedPluginId.clear();

    rows.clear();

    for (const auto& insert : inserts)
        addAndMakeVisible (rows.add (new InsertRow (*this, insert, insert.id == selectedPluginId)));

    resized();
    repaint();
}

void InsertStrip::selectPlugin (const juce::String& pluginId)
{
    selectedPluginId = pluginId;

    for (auto* row : rows)
        row->setSelected (row->getPluginId() == pluginId);
}

void InsertStrip::removePlugin (const juce::String& pluginId)
{
    if (shownTrackId.isNotEmpty() && pluginId.isNotEmpty())
        commands.invoke ("plugin.remove", pluginRemoveArgs (shownTrackId, pluginId));
}

} // namespace papercut
