#include "MasterStrip.h"
#include "Commands/MixerCommands.h"
#include "Engine/ApplicationModel.h"
#include "UI/Controls/Icons.h"

namespace papercut
{

namespace
{

}

MasterStrip::MasterStrip (CommandRegistry& c, ThemeManager& tm)
    : commands (c), themeManager (tm), pan (tm, panKnobSpec(), "Pan", true), gain (tm, gainReadoutSpec()), fader (tm), meter (tm)
{
    setTitle ("Master");
    pan.setDialSize (22);
    pan.onChange = [this] (double v, bool continues) { commands.invoke ("mixer.setMasterPan", masterPanArgs (v, continues)); };

    auto setVolume = [this] (double db, bool continues) { commands.invoke ("mixer.setMasterVolume", masterVolumeArgs (db, continues)); };
    fader.onChange = setVolume;
    gain.onChange = setVolume;
    gain.setDoubleClickEdits (false);
    gain.setTooltip ("Master gain: click to type");

    for (auto* child : std::initializer_list<juce::Component*> { &pan, &gain, &fader, &meter })
        addAndMakeVisible (child);
}

void MasterStrip::setMaster (const MasterInfo& info)
{
    pan.setValue (info.pan);
    fader.setValue (info.volumeDb);
    gain.setValue (info.volumeDb);
    fader.setColour (themeManager.getTheme().accent);
}

void MasterStrip::setLevel (StereoLevel level, double elapsedSeconds)
{
    meter.setLevel (level, elapsedSeconds);
}

void MasterStrip::paint (juce::Graphics& g)
{
    auto& theme = themeManager.getTheme();
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (theme.bgPanel);
    g.fillRoundedRectangle (bounds, theme.radiusLg);
    g.setColour (theme.border);
    g.drawRoundedRectangle (bounds.reduced (0.5f), theme.radiusLg, 1.0f);

    auto head = getLocalBounds().removeFromTop (30).reduced (10, 8);
    drawIcon (g, Icon::audioLines, head.removeFromLeft (14).toFloat(), theme.accent);
    head.removeFromLeft (7);
    drawStyledText (g, themeManager, "Master", TypeStyle { 12.0f, false, 600 }, head, juce::Justification::centredLeft,
                    theme.textPrimary);
    drawNumber (g, themeManager, "1/2", TypeStyle { 10.0f, true, 400 }, head, juce::Justification::centredRight, theme.textDim);
}

void MasterStrip::resized()
{
    auto r = getLocalBounds().reduced (10, 0);
    r.removeFromTop (30);
    pan.setBounds (r.removeFromTop (56).removeFromLeft (60));
    gain.setBounds (r.removeFromTop (20).removeFromLeft (70));
    r.removeFromTop (8);
    r.removeFromBottom (10);

    auto meterColumn = r.removeFromRight (17);
    r.removeFromRight (6);
    fader.setBounds (r.removeFromLeft (70));
    meter.setBounds (meterColumn.withY (fader.getY() + fader.getTravelBounds().getY()).withHeight (fader.getTravelBounds().getHeight()));
}

} // namespace papercut
