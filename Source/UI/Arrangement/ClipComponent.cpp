#include "ClipComponent.h"
#include "UI/Theme/Interaction.h"

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

void ClipComponent::setTrackLook (juce::Colour c, bool isMuted)
{
    if (c == colour && isMuted == muted)
        return;

    colour = c;
    muted = isMuted;
    setAlpha (muted ? 0.5f : 1.0f);
    repaint();
}

void ClipComponent::paint (juce::Graphics& g)
{
    auto& theme = themeManager.getTheme();
    auto& metrics = themeManager.getMetrics();
    auto bounds = getLocalBounds();
    constexpr float radius = 5.0f;
    const auto ink = theme.textOnAccent;

    g.setColour (colour);
    g.fillRoundedRectangle (bounds.toFloat(), radius);

    auto header = bounds.removeFromTop (metrics.clipHeaderHeight);
    {
        juce::Graphics::ScopedSaveState save (g);
        juce::Path shape;
        shape.addRoundedRectangle (getLocalBounds().toFloat(), radius);
        g.reduceClipRegion (shape);
        g.setColour (ink.withAlpha ((juce::uint8) 0x22));
        g.fillRect (header);
    }

    // Keep the name readable when the clip starts off-screen.
    auto nameArea = header.withLeft (std::max (header.getX(), g.getClipBounds().getX())).reduced (6, 0);
    const auto take = clip.numTakes == 0 ? juce::String()
                    : clip.currentTake < 0 ? "  (" + juce::String (clip.numTakes) + " takes)"
                                           : "  (Take " + juce::String (clip.currentTake + 1) + "/" + juce::String (clip.numTakes) + ")";
    drawStyledText (g, themeManager, clip.name + take, TypeStyle { 9.5f, false, 700 }, nameArea,
                    juce::Justification::centredLeft, ink);

    const auto content = ink.withAlpha ((juce::uint8) 0x88);
    const auto body = bounds.reduced (0, 4);

    if (clip.kind == TrackKind::midi)
    {
        if (getWidth() > 0 && clip.lengthSeconds > 0 && ! clip.notes.empty())
        {
            // Note dashes, placed by pitch across the clip's own range.
            int low = 127, high = 0;

            for (auto& note : clip.notes)
            {
                low = std::min (low, note.pitch);
                high = std::max (high, note.pitch);
            }

            const auto noteHeight = (float) metrics.midiNoteHeight;
            const auto span = (float) juce::jmax (12, high - low + 1);
            g.setColour (content);

            for (auto& note : clip.notes)
            {
                const auto x = (float) (note.startSeconds / clip.lengthSeconds) * (float) getWidth();
                const auto w = juce::jmax (2.0f, (float) (note.lengthSeconds / clip.lengthSeconds) * (float) getWidth() - 1.0f);
                const auto y = (float) body.getBottom() - noteHeight - ((float) (note.pitch - low) / span) * ((float) body.getHeight() - noteHeight);
                g.fillRoundedRectangle (x, y, w, noteHeight, 1.0f);
            }
        }
    }
    else if (waveform != nullptr && getWidth() > 0 && clip.lengthSeconds > 0)
    {
        // Only the visible slice: a zoomed-in clip can be far wider than the screen.
        auto visible = body.getIntersection (g.getClipBounds());

        if (! visible.isEmpty())
        {
            const auto secondsPerPixel = clip.lengthSeconds / getWidth();
            const auto sourceStart = clip.sourceOffsetSeconds + visible.getX() * secondsPerPixel;
            const auto sourceEnd = clip.sourceOffsetSeconds + visible.getRight() * secondsPerPixel;

            g.setColour (content);
            waveform->draw (g, visible, sourceStart, sourceEnd);

            if (waveform->isGenerating())
                drawStyledText (g, themeManager,
                                "Preparing audio " + juce::String (juce::roundToInt (waveform->getProgress() * 100.0)) + "%",
                                theme.bodySm, visible.reduced (6), juce::Justification::centredLeft, ink);
        }
    }

    if (clip.selected)
    {
        g.setColour (juce::Colours::white);
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), radius, 1.0f);
    }
}

} // namespace papercut
