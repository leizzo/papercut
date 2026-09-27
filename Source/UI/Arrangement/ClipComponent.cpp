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
    const bool audioFileChanged = info.kind == TrackKind::audio
                               && (info.file != clip.file || waveform == nullptr);

    if (info.kind != TrackKind::audio)
        waveform.reset();

    clip = info;

    if (audioFileChanged)
        waveform = model.createWaveform (clip.id, *this);

    repaint();
}

void ClipComponent::paint (juce::Graphics& g)
{
    auto& theme = themeManager.getTheme();
    auto& metrics = themeManager.getMetrics();
    auto bounds = getLocalBounds();

    const bool midi = clip.kind == TrackKind::midi;
    g.setColour (clip.selected ? (midi ? theme.midiClipSelected : theme.clipSelected)
                               : (midi ? theme.midiClip : theme.clip));
    g.fillRoundedRectangle (bounds.toFloat(), theme.cornerRadius);

    auto header = bounds.removeFromTop (metrics.clipHeaderHeight);
    g.setColour (theme.clipText);
    g.setFont (themeManager.getFont (0.85f));
    // Keep the name readable when the clip starts off-screen.
    auto nameArea = header.withLeft (std::max (header.getX(), g.getClipBounds().getX())).reduced (metrics.textPadding, 0);
    const auto take = clip.numTakes == 0 ? juce::String()
                    : clip.currentTake < 0 ? "  (" + juce::String (clip.numTakes) + " takes)"
                                           : "  (Take " + juce::String (clip.currentTake + 1) + "/" + juce::String (clip.numTakes) + ")";
    g.drawText (clip.name + take, nameArea, juce::Justification::centredLeft, true);

    if (midi)
    {
        if (getWidth() <= 0 || clip.lengthSeconds <= 0 || clip.notes.empty())
            return;

        auto body = bounds.toFloat();
        const auto noteHeight = (float) metrics.midiNoteHeight;
        g.setColour (theme.midiNote);

        for (auto& note : clip.notes)
        {
            const auto x = (float) (note.startSeconds / clip.lengthSeconds) * (float) getWidth();
            const auto w = juce::jmax (1.0f, (float) (note.lengthSeconds / clip.lengthSeconds) * (float) getWidth());
            const auto pitch = juce::jlimit (0, 127, note.pitch);
            const auto y = juce::jlimit (body.getY(), body.getBottom() - noteHeight,
                                          body.getBottom() - ((float) (pitch + 1) / 128.0f) * body.getHeight());
            g.fillRect (x, y, w, noteHeight);
        }

        return;
    }

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
        g.drawText ("Preparing audio " + juce::String (juce::roundToInt (waveform->getProgress() * 100.0)) + "%",
                    visible.reduced (metrics.textPadding), juce::Justification::centredLeft, true);
    }
}

} // namespace papercut
