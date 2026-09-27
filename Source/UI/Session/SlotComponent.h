#pragma once

#include "Engine/ApplicationModel.h"
#include "Engine/Session.h"

namespace papercut
{

class CommandRegistry;
class ThemeManager;

/** One cell in the session grid. Click an empty audio slot to add a clip,
    click a filled slot to launch it, right-click a clip to clear it.
    Every change goes through a Session command. */
class SlotComponent : public juce::Component
{
public:
    SlotComponent (CommandRegistry&, ThemeManager&, const juce::String& trackId, TrackKind, const SlotInfo&);

    void setSlot (const SlotInfo&);
    const SlotInfo& getSlot() const noexcept   { return slot; }

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    CommandRegistry& commands;
    ThemeManager& themeManager;
    juce::String trackId;
    TrackKind kind;
    SlotInfo slot;
};

} // namespace papercut
