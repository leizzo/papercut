#include "ArrangementViewState.h"

namespace papercut
{

namespace
{
    const juce::Identifier pixelsPerSecondId ("pixelsPerSecond");
    const juce::Identifier scrollSecondsId ("scrollSeconds");
    const juce::Identifier scrollYId ("scrollY");
}

ArrangementViewState::ArrangementViewState (juce::ValueTree uiState)
    : state (std::move (uiState))
{
}

double ArrangementViewState::getPixelsPerSecond() const
{
    return juce::jlimit (minPixelsPerSecond, maxPixelsPerSecond,
                         (double) state.getProperty (pixelsPerSecondId, defaultPixelsPerSecond));
}

double ArrangementViewState::getScrollSeconds() const   { return std::max (0.0, (double) state.getProperty (scrollSecondsId, 0.0)); }
int ArrangementViewState::getScrollY() const            { return std::max (0, (int) state.getProperty (scrollYId, 0)); }

void ArrangementViewState::setPixelsPerSecond (double pps)
{
    state.setProperty (pixelsPerSecondId, juce::jlimit (minPixelsPerSecond, maxPixelsPerSecond, pps), nullptr);
}

void ArrangementViewState::setScrollSeconds (double seconds)   { state.setProperty (scrollSecondsId, std::max (0.0, seconds), nullptr); }
void ArrangementViewState::setScrollY (int y)                  { state.setProperty (scrollYId, std::max (0, y), nullptr); }

void ArrangementViewState::zoomAround (double factor, float anchorX)
{
    const auto anchorTime = xToTime (anchorX);
    setPixelsPerSecond (getPixelsPerSecond() * factor);
    setScrollSeconds (anchorTime - anchorX / getPixelsPerSecond());
}

float ArrangementViewState::timeToX (double seconds) const
{
    return (float) ((seconds - getScrollSeconds()) * getPixelsPerSecond());
}

double ArrangementViewState::xToTime (float x) const
{
    return getScrollSeconds() + x / getPixelsPerSecond();
}

int ArrangementViewState::rowToY (int row, int rowHeight) const
{
    return row * rowHeight - getScrollY();
}

int ArrangementViewState::yToRow (int y, int rowHeight) const
{
    return (int) std::floor ((double) (y + getScrollY()) / rowHeight);
}

} // namespace papercut
