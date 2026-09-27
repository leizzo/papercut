#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <span>

namespace papercut
{

/** One row of the static table that maps JUCE ApplicationCommand IDs (menus,
    keyboard shortcuts) onto Command string IDs (ADR-0006). */
struct ApplicationCommandEntry
{
    juce::CommandID applicationCommandID;
    const char* commandId;      ///< Command registry string ID
    const char* category;       ///< also the menu the item appears in
    int keyCode;                ///< 0 for no default shortcut
    int modifiers;              ///< juce::ModifierKeys flags
};

std::span<const ApplicationCommandEntry> getApplicationCommandTable();

const ApplicationCommandEntry* findApplicationCommand (juce::CommandID);

} // namespace papercut
