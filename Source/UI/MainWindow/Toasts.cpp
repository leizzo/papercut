#include "Toasts.h"

namespace papercut
{

namespace
{
    constexpr int toastHeight = 36, gap = 8, bottomMargin = 24, paddingX = 14;
}

Toasts::Toasts (ThemeManager& tm) : themeManager (tm)
{
    setInterceptsMouseClicks (true, false);
    setAlwaysOnTop (true);
}

void Toasts::show (const juce::String& message, std::function<void()> undo, bool isError)
{
    toasts.push_back ({ message, std::move (undo), isError, juce::Time::getMillisecondCounter(), {}, {} });

    while ((int) toasts.size() > maxShown)
        toasts.pop_front();

    layoutToasts();
    toFront (false);
    startTimer (100);
}

void Toasts::layoutToasts()
{
    auto& theme = themeManager.getTheme();
    const auto font = themeManager.font (theme.body);
    auto y = getHeight() - bottomMargin;

    for (auto it = toasts.rbegin(); it != toasts.rend(); ++it)
    {
        const auto textWidth = juce::GlyphArrangement::getStringWidthInt (font, it->message);
        const auto undoWidth = it->undo ? juce::GlyphArrangement::getStringWidthInt (font, "Undo") + 2 * paddingX : 0;
        const auto width = juce::jmin (getWidth() - 40, textWidth + 2 * paddingX + undoWidth);
        y -= toastHeight;
        it->bounds = juce::Rectangle<int> ((getWidth() - width) / 2, y, width, toastHeight);
        it->undoBounds = it->undo ? it->bounds.withLeft (it->bounds.getRight() - undoWidth) : juce::Rectangle<int>();
        y -= gap;
    }

    repaint();
}

void Toasts::resized()
{
    layoutToasts();
}

bool Toasts::hitTest (int x, int y)
{
    for (auto& t : toasts)
        if (t.bounds.contains (x, y))
            return true;

    return false;
}

void Toasts::paint (juce::Graphics& g)
{
    auto& theme = themeManager.getTheme();

    for (auto& t : toasts)
    {
        const auto r = t.bounds.toFloat();
        paintElevation (g, theme.elevation2, r, theme.radius2xl);
        g.setColour (theme.bgElevated);
        g.fillRoundedRectangle (r, theme.radius2xl);
        g.setColour (t.error ? theme.rec : theme.border);
        g.drawRoundedRectangle (r.reduced (0.5f), theme.radius2xl, 1.0f);

        auto text = t.bounds.reduced (paddingX, 0);

        if (t.undo)
        {
            drawStyledText (g, themeManager, "Undo", TypeStyle { theme.body.size, false, 600 }, t.undoBounds,
                            juce::Justification::centred, theme.accent);
            text = text.withRight (t.undoBounds.getX());
        }

        drawStyledText (g, themeManager, t.message, theme.body, text, juce::Justification::centredLeft,
                        t.error ? theme.rec : theme.textPrimary);
    }
}

void Toasts::mouseUp (const juce::MouseEvent& e)
{
    for (auto it = toasts.begin(); it != toasts.end(); ++it)
    {
        if (! it->bounds.contains (e.getPosition()))
            continue;

        if (it->undoBounds.contains (e.getPosition()) && it->undo)
            it->undo();

        toasts.erase (it);
        layoutToasts();
        return;
    }
}

void Toasts::timerCallback()
{
    const auto now = juce::Time::getMillisecondCounter();
    const auto before = toasts.size();

    while (! toasts.empty() && now - toasts.front().shownAt > (juce::uint32) lifetimeMs)
        toasts.pop_front();

    if (toasts.size() != before)
        layoutToasts();

    if (toasts.empty())
        stopTimer();
}

} // namespace papercut
