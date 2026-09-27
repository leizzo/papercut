#include "Playhead.h"
#include "Engine/ApplicationModel.h"
#include "UI/State/ArrangementViewState.h"
#include "UI/Theme/ThemeManager.h"

namespace papercut
{

Playhead::Playhead (ApplicationModel& m, ThemeManager& tm, ArrangementViewState& v)
    : model (m), themeManager (tm), view (v)
{
    setInterceptsMouseClicks (false, false);
    startTimerHz (30);
}

void Playhead::update()
{
    const auto x = juce::roundToInt (view.timeToX (model.getTransportPositionSeconds()));

    if (x != lastX)
    {
        const auto lineWidth = themeManager.getMetrics().playheadWidth;
        repaint (lastX - lineWidth, 0, lineWidth * 2, getHeight());
        repaint (x - lineWidth, 0, lineWidth * 2, getHeight());
        lastX = x;
    }
}

void Playhead::paint (juce::Graphics& g)
{
    if (lastX < 0 || lastX > getWidth())
        return;

    const auto lineWidth = themeManager.getMetrics().playheadWidth;
    g.setColour (themeManager.getTheme().playhead);
    g.fillRect (lastX - lineWidth / 2, 0, lineWidth, getHeight());
}

} // namespace papercut
