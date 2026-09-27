#include "ClipComponent.h"
#include "UI/Theme/ThemeManager.h"

namespace papercut
{

ClipComponent::ClipComponent (ApplicationModel& m, ThemeManager& tm, const ClipInfo& info)
    : model (m), themeManager (tm)
{
    setInterceptsMouseClicks (false, false);
    setClip (info);
}

void ClipComponent::setClip (const ClipInfo& info)
{
    const bool fileChanged = info.file != clip.file || waveform == nullptr;
    clip = info;

    if (fileChanged)
        waveform = model.createWaveform (clip.id, *this);

    repaint();
}

void ClipComponent::paint (juce::Graphics& g)
{
    auto& theme = themeManager.getTheme();
    auto& metrics = themeManager.getMetrics();
    auto bounds = getLocalBounds();

    g.setColour (theme.clip);
    g.fillRoundedRectangle (bounds.toFloat(), theme.cornerRadius);

    auto header = bounds.removeFromTop (metrics.clipHeaderHeight);
    g.setColour (theme.clipText);
    g.setFont (themeManager.getFont (0.85f));
    // Keep the name readable when the clip starts off-screen.
    auto nameArea = header.withLeft (std::max (header.getX(), g.getClipBounds().getX())).reduced (metrics.textPadding, 0);
    g.drawText (clip.name, nameArea, juce::Justification::centredLeft, true);

    if (waveform == nullptr || getWidth() <= 0 || clip.lengthSeconds <= 0)
        return;

    // Only the visible slice: a zoomed-in clip can be far wider than the screen.
    auto visible = bounds.getIntersection (g.getClipBounds());

    if (visible.isEmpty())
        return;

    const auto secondsPerPixel = clip.lengthSeconds / getWidth();
    const auto sourceStart = clip.sourceOffsetSeconds + visible.getX() * secondsPerPixel;
    const auto sourceEnd = clip.sourceOffsetSeconds + visible.getRight() * secondsPerPixel;

    g.setColour (theme.waveform);
    waveform->draw (g, visible, sourceStart, sourceEnd);

    if (waveform->isGenerating())
    {
        g.setColour (theme.clipText);
        g.drawText ("Reading waveform " + juce::String (juce::roundToInt (waveform->getProgress() * 100.0)) + "%",
                    visible.reduced (metrics.textPadding), juce::Justification::centredLeft, true);
    }
}

} // namespace papercut
