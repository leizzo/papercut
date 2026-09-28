#pragma once

#include "Commands/CommandRegistry.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace papercut
{

/** A context-menu item that invokes a Command and shows the shortcut bound to
    it (PRD §16.5). Disabled when the Command is. label defaults to its name. */
juce::PopupMenu::Item commandItem (CommandRegistry&, const juce::String& commandId, const juce::var& args = {},
                                   const juce::String& label = {});

/** "Name (shortcut)" for a tooltip, when the Command has a shortcut (PRD §16.6). */
juce::String tooltipFor (const juce::String& name, const juce::String& commandId);

} // namespace papercut
