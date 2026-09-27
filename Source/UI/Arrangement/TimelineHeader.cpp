#include "TimelineHeader.h"
#include "Commands/AppCommands.h"
#include "UI/State/ArrangementViewState.h"
#include "UI/Theme/ThemeManager.h"

namespace papercut
{

namespace
{
    /** The smallest "nice" tick interval that keeps labels apart. */
    double tickIntervalFor (double pixelsPerSecond, float minPixels)
    {
        for (auto step : { 0.01, 0.02, 0.05, 0.1, 0.25, 0.5, 1.0, 2.0, 5.0, 10.0, 15.0, 30.0, 60.0, 120.0, 300.0, 600.0 })
            if (step * pixelsPerSecond >= minPixels)
                return step;

        return 1200.0;
    }

    juce::String formatTime (double seconds, double interval)
    {
        const auto minutes = (int) (seconds / 60.0);
        const auto secs = seconds - minutes * 60.0;
        const auto decimals = interval >= 1.0 ? 0 : interval >= 0.1 ? 1 : 2;
        return juce::String (minutes) + ":" + (secs < 10.0 ? "0" : "") + juce::String (secs, decimals);
    }
}

TimelineHeader::TimelineHeader (ApplicationModel& m, CommandRegistry& c, ThemeManager& tm, ArrangementViewState& v)
    : model (m), commands (c), themeManager (tm), view (v)
{
}

void TimelineHeader::paint (juce::Graphics& g)
{
    auto& theme = themeManager.getTheme();
    g.fillAll (theme.panel);

    // The loop, in the ruler's upper half.
    const auto loop = draggedLoop.value_or (model.getLoopRange());

    if (loop.end > loop.start)
    {
        const auto x1 = view.timeToX (loop.start), x2 = view.timeToX (loop.end);
        g.setColour (theme.loop.withMultipliedAlpha (draggedLoop || model.isLooping() ? 0.8f : 0.25f));
        g.fillRect (juce::Rectangle<float> (x1, 0.0f, x2 - x1, (float) getHeight() * 0.5f));
    }

    const auto font = themeManager.getFont (0.8f);
    const auto textPadding = themeManager.getMetrics().textPadding / 2;
    const auto interval = tickIntervalFor (view.getPixelsPerSecond(), juce::GlyphArrangement::getStringWidth (font, "00:00.00") * 1.5f);
    const auto first = std::floor (view.xToTime (0) / interval) * interval;

    g.setFont (font);
    g.setColour (theme.ruler);

    for (auto t = first; view.timeToX (t) < (float) getWidth(); t += interval)
    {
        const auto x = view.timeToX (t);

        if (x < 0)
            continue;

        const auto text = formatTime (t, interval);
        const auto textWidth = (int) std::ceil (juce::GlyphArrangement::getStringWidth (font, text));

        g.drawVerticalLine (juce::roundToInt (x), (float) getHeight() * 0.5f, (float) getHeight());
        g.drawText (text, juce::roundToInt (x) + textPadding, 0, textWidth, getHeight() / 2 + textPadding,
                    juce::Justification::bottomLeft, false);
    }

    g.drawHorizontalLine (getHeight() - 1, 0.0f, (float) getWidth());
}

void TimelineHeader::mouseDown (const juce::MouseEvent& e)
{
    dragStartSeconds = std::max (0.0, view.xToTime ((float) e.x));
    draggedLoop.reset();
}

void TimelineHeader::mouseDrag (const juce::MouseEvent& e)
{
    const auto now = std::max (0.0, view.xToTime ((float) e.x));
    draggedLoop = TimeRangeSeconds { std::min (dragStartSeconds, now), std::max (dragStartSeconds, now) };
    repaint();
}

void TimelineHeader::mouseUp (const juce::MouseEvent&)
{
    if (auto loop = std::exchange (draggedLoop, std::nullopt))
        commands.invoke ("transport.setLoopRange", loopRangeArgs (loop->start, loop->end));

    repaint();
}

} // namespace papercut
