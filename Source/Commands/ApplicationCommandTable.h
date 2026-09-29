#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <limits>
#include <span>

namespace resamper
{

/** One row of the static table that maps JUCE ApplicationCommand IDs (menus)
    onto Command string IDs. */
struct ApplicationCommandEntry
{
    juce::CommandID applicationCommandID;
    const char* commandId;      ///< Command registry string ID
    const char* category;       ///< the menu the item appears in
};

/** Where a shortcut applies (PRD §17): everywhere, or only while a view shows,
    so `S`, `Q` and the arrows act on the view in front. Flags combine. */
enum ShortcutContext : int
{
    anyView       = 0,
    arrangeView   = 1 << 0,
    sessionView   = 1 << 1,
    mixerView     = 1 << 2,
    pianoRollView = 1 << 3,
    editorView    = 1 << 4,
};

/** One default shortcut. The map is data (customisable later): nothing else
    hard-codes a key. Mod is juce::ModifierKeys::commandModifier: Cmd on
    macOS, Ctrl on Windows. */
struct KeyBinding
{
    const char* commandId;
    int keyCode;
    int modifiers;          ///< juce::ModifierKeys flags
    int contexts;           ///< ShortcutContext flags; anyView for a global shortcut
    int argument = noArgument;   ///< passed to the Command as args["argument"] (F1-F8's track, a transpose)

    static constexpr int noArgument = std::numeric_limits<int>::min();
};

/** A §17 shortcut whose feature isn't built yet. */
struct PendingShortcut
{
    const char* keys;
    const char* action;
    const char* waitingOn;
};

std::span<const ApplicationCommandEntry> getApplicationCommandTable();
std::span<const KeyBinding> getKeyBindings();
std::span<const PendingShortcut> getPendingShortcuts();

const ApplicationCommandEntry* findApplicationCommand (juce::CommandID);

/** The menus, in order: File Edit Create View Options Help (PRD §6.1). */
std::span<const char* const> getMenuNames();

/** One menu's items, with their shortcuts, from the table. */
juce::PopupMenu createCommandMenu (juce::ApplicationCommandManager&, const juce::String& menuName);

/** The first shortcut bound to a Command, or an invalid KeyPress if none. */
juce::KeyPress findShortcut (const juce::String& commandId);

/** The binding for a key in a context (a view's flag), or nullptr. A
    contextual binding wins over a global one. */
const KeyBinding* findBinding (const juce::KeyPress&, int context);

/** The args a binding passes to its Command. */
juce::var bindingArgs (const KeyBinding&);

} // namespace resamper
