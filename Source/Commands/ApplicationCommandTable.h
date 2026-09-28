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

/** The menus, in order: File Edit Create View Options Help (PRD §6.1). */
std::span<const char* const> getMenuNames();

/** One menu's items, with their shortcuts, from the table. */
juce::PopupMenu createCommandMenu (juce::ApplicationCommandManager&, const juce::String& menuName);

const ApplicationCommandEntry* findApplicationCommand (juce::CommandID);

} // namespace papercut
