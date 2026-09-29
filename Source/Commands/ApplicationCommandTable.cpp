#include "ApplicationCommandTable.h"

#include <array>

namespace resamper
{

namespace
{
    using MK = juce::ModifierKeys;
    constexpr int cmd = MK::commandModifier;
    constexpr int shift = MK::shiftModifier;
    constexpr int alt = MK::altModifier;
    using KP = juce::KeyPress;

    const std::array table
    {
        ApplicationCommandEntry { 0x1001, "project.new",             "File" },
        ApplicationCommandEntry { 0x1002, "project.open",            "File" },
        ApplicationCommandEntry { 0x1003, "project.save",            "File" },
        ApplicationCommandEntry { 0x1004, "project.saveAs",          "File" },
        ApplicationCommandEntry { 0x5005, "project.autosave",        "File" },
        ApplicationCommandEntry { 0x5006, "project.recover",         "File" },
        ApplicationCommandEntry { 0x5007, "project.saveTemplate",    "File" },
        ApplicationCommandEntry { 0x5008, "project.newFromTemplate", "File" },
        ApplicationCommandEntry { 0x5004, "file.exportMix",          "File" },

        ApplicationCommandEntry { 0x2001, "edit.undo",               "Edit" },
        ApplicationCommandEntry { 0x2002, "edit.redo",               "Edit" },
        ApplicationCommandEntry { 0x200a, "clip.duplicate",          "Edit" },
        ApplicationCommandEntry { 0x2006, "clip.split",              "Edit" },
        ApplicationCommandEntry { 0x200b, "clip.consolidate",        "Edit" },
        ApplicationCommandEntry { 0x2009, "edit.delete",             "Edit" },
        ApplicationCommandEntry { 0x200c, "ui.escape",               "Edit" },
        ApplicationCommandEntry { 0x2004, "track.remove",            "Edit" },
        ApplicationCommandEntry { 0x5001, "track.freeze",            "Edit" },
        ApplicationCommandEntry { 0x5002, "track.unfreeze",          "Edit" },
        ApplicationCommandEntry { 0x5003, "track.bounce",            "Edit" },

        ApplicationCommandEntry { 0x2003, "track.add",               "Create" },
        ApplicationCommandEntry { 0x2007, "track.addMidi",           "Create" },
        ApplicationCommandEntry { 0x6002, "mixer.addReturn",         "Create" },
        ApplicationCommandEntry { 0x6005, "mixer.addBus",            "Create" },
        ApplicationCommandEntry { 0x2005, "clip.add",                "Create" },
        ApplicationCommandEntry { 0x2008, "clip.addMidi",            "Create" },

        ApplicationCommandEntry { 0x7001, "view.session",            "View" },
        ApplicationCommandEntry { 0x7002, "view.arrange",            "View" },
        ApplicationCommandEntry { 0x7003, "view.mixer",              "View" },
        ApplicationCommandEntry { 0x7004, "view.pianoRoll",          "View" },
        ApplicationCommandEntry { 0x7005, "view.editor",             "View" },
        ApplicationCommandEntry { 0x7006, "view.toggleSessionArrange", "View" },
        ApplicationCommandEntry { 0x7007, "view.toggleBrowser",      "View" },
        ApplicationCommandEntry { 0x7008, "view.toggleDetail",       "View" },
        ApplicationCommandEntry { 0x700a, "arrange.zoomIn",          "View" },
        ApplicationCommandEntry { 0x700b, "arrange.zoomOut",         "View" },
        ApplicationCommandEntry { 0x700c, "arrange.zoomToSelection", "View" },
        ApplicationCommandEntry { 0x700d, "arrange.zoomToSong",      "View" },
        ApplicationCommandEntry { 0x5009, "theme.use",               "View" },
        ApplicationCommandEntry { 0x4001, "dev.reloadLayout",        "View" },
        ApplicationCommandEntry { 0x4002, "dev.reloadTheme",         "View" },
        ApplicationCommandEntry { 0x4003, "dev.toggleOverlay",       "View" },

        ApplicationCommandEntry { 0x3001, "transport.togglePlay",    "Options" },
        ApplicationCommandEntry { 0x3009, "transport.playFromSelection", "Options" },
        ApplicationCommandEntry { 0x3002, "transport.play",          "Options" },
        ApplicationCommandEntry { 0x3003, "transport.stop",          "Options" },
        ApplicationCommandEntry { 0x3004, "transport.returnToStart", "Options" },
        ApplicationCommandEntry { 0x3005, "transport.record",        "Options" },
        ApplicationCommandEntry { 0x3006, "transport.loopSelection", "Options" },
        ApplicationCommandEntry { 0x300a, "transport.toggleLoop",    "Options" },
        ApplicationCommandEntry { 0x3007, "transport.toggleMetronome", "Options" },
        ApplicationCommandEntry { 0x300b, "transport.toggleCountIn", "Options" },
        ApplicationCommandEntry { 0x3008, "transport.tapTempo",      "Options" },
        ApplicationCommandEntry { 0x7009, "view.toggleFollow",       "Options" },
        ApplicationCommandEntry { 0x6001, "plugin.scan",             "Options" },
        ApplicationCommandEntry { 0x6003, "session.stopAll",         "Options" },
        ApplicationCommandEntry { 0x6004, "session.recordToArrangement", "Options" },
    };

    constexpr int timeline = arrangeView | sessionView;

    /** PRD §17, plus the app's own (file, developer). Contextual keys come after the global ones. */
    const std::array bindings
    {
        // File
        KeyBinding { "project.new",               'N',                 cmd,               anyView },
        KeyBinding { "project.open",              'O',                 cmd,               anyView },
        KeyBinding { "project.save",              'S',                 cmd,               anyView },
        KeyBinding { "project.saveAs",            'S',                 cmd | shift,       anyView },
        KeyBinding { "project.autosave",          'S',                 cmd | alt,         anyView },
        KeyBinding { "file.exportMix",            'E',                 cmd | shift,       anyView },
        KeyBinding { "clip.add",                  'I',                 cmd,               anyView },
        KeyBinding { "clip.addMidi",              'I',                 cmd | shift,       anyView },

        // Transport
        KeyBinding { "transport.togglePlay",      KP::spaceKey,        0,                 anyView },
        KeyBinding { "transport.playFromSelection", KP::spaceKey,      shift,             anyView },
        KeyBinding { "transport.record",          KP::F9Key,           0,                 anyView },
        KeyBinding { "transport.loopSelection",   'L',                 cmd,               anyView },
        KeyBinding { "transport.toggleMetronome", 'C',                 0,                 anyView },
        KeyBinding { "transport.tapTempo",        'T',                 0,                 anyView },
        KeyBinding { "transport.returnToStart",   KP::homeKey,         0,                 anyView },

        // Views
        KeyBinding { "view.toggleSessionArrange", KP::tabKey,          0,                 anyView },
        KeyBinding { "view.mixer",                'M',                 cmd | alt,         anyView },
        KeyBinding { "view.toggleDetail",         'L',                 cmd | alt,         anyView },
        KeyBinding { "view.toggleBrowser",        'B',                 cmd | alt,         anyView },

        // Edit
        KeyBinding { "edit.undo",                 'Z',                 cmd,               anyView },
        KeyBinding { "edit.redo",                 'Z',                 cmd | shift,       anyView },
        KeyBinding { "clip.duplicate",            'D',                 cmd,               anyView },
        KeyBinding { "clip.split",                'E',                 cmd,               anyView },
        KeyBinding { "clip.consolidate",          'J',                 cmd,               anyView },
        KeyBinding { "edit.delete",               KP::deleteKey,       0,                 anyView },
        KeyBinding { "edit.delete",               KP::backspaceKey,    0,                 anyView },
        KeyBinding { "ui.escape",                 KP::escapeKey,       0,                 anyView },
        KeyBinding { "track.remove",              KP::backspaceKey,    cmd,               anyView },
        KeyBinding { "track.freeze",              'F',                 cmd | shift,       anyView },
        KeyBinding { "track.unfreeze",            'F',                 cmd | alt,         anyView },
        KeyBinding { "track.bounce",              'B',                 cmd,               anyView },

        // Tracks
        KeyBinding { "track.add",                 'T',                 cmd,               anyView },
        KeyBinding { "track.addMidi",             'T',                 cmd | shift,       anyView },
        KeyBinding { "mixer.addReturn",           'T',                 cmd | alt,         anyView },
        KeyBinding { "track.toggleMuteAt",        KP::F1Key,           0,                 anyView, 0 },
        KeyBinding { "track.toggleMuteAt",        KP::F2Key,           0,                 anyView, 1 },
        KeyBinding { "track.toggleMuteAt",        KP::F3Key,           0,                 anyView, 2 },
        KeyBinding { "track.toggleMuteAt",        KP::F4Key,           0,                 anyView, 3 },
        KeyBinding { "track.toggleMuteAt",        KP::F5Key,           0,                 anyView, 4 },
        KeyBinding { "track.toggleMuteAt",        KP::F6Key,           0,                 anyView, 5 },
        KeyBinding { "track.toggleMuteAt",        KP::F7Key,           0,                 anyView, 6 },
        KeyBinding { "track.toggleMuteAt",        KP::F8Key,           0,                 anyView, 7 },

        // Other app shortcuts
        KeyBinding { "plugin.scan",               'P',                 cmd | shift,       anyView },
        KeyBinding { "dev.reloadLayout",          'L',                 cmd | alt | shift, anyView },
        KeyBinding { "dev.reloadTheme",           'T',                 cmd | alt | shift, anyView },
        KeyBinding { "dev.toggleOverlay",         'D',                 cmd | alt | shift, anyView },

        // Only in the view in front
        KeyBinding { "track.toggleSoloSelected",  'S',                 0,                 timeline | mixerView },
        KeyBinding { "arrange.zoomIn",            '=',                 0,                 arrangeView },
        KeyBinding { "arrange.zoomIn",            '+',                 0,                 arrangeView },
        KeyBinding { "arrange.zoomIn",            '+',                 shift,             arrangeView },   // + typed as Shift+=
        KeyBinding { "arrange.zoomOut",           '-',                 0,                 arrangeView },
        KeyBinding { "arrange.zoomToSelection",   'Z',                 0,                 arrangeView },
        KeyBinding { "arrange.zoomToSong",        'Z',                 shift,             arrangeView },
        KeyBinding { "pianoRoll.quantize",        'Q',                 0,                 pianoRollView },
        KeyBinding { "pianoRoll.transpose",       KP::upKey,           0,                 pianoRollView, 1 },
        KeyBinding { "pianoRoll.transpose",       KP::downKey,         0,                 pianoRollView, -1 },
        KeyBinding { "pianoRoll.transpose",       KP::upKey,           shift,             pianoRollView, 12 },
        KeyBinding { "pianoRoll.transpose",       KP::downKey,         shift,             pianoRollView, -12 },
        KeyBinding { "pianoRoll.selectAll",       'A',                 cmd,               pianoRollView },
    };

    /** §17 rows waiting on their feature tickets. */
    const std::array pending
    {
        PendingShortcut { "Mod+G / Mod+Shift+G", "Group / ungroup (folder or rack)", "Folders (M2) and racks (M5)" },
        PendingShortcut { "A",                   "Automation mode",                  "Automation (M3)" },
        PendingShortcut { "B",                   "Pencil / draw",                    "Automation (M3) and editors (M4)" },
        PendingShortcut { "Mod+Shift+R",         "Re-enable automation",             "Automation recording (M3)" },
        PendingShortcut { "Mod+D in the Piano Roll", "Duplicate notes",              "Piano roll (M4)" },
        PendingShortcut { "+ / - in the Piano Roll", "Zoom",                         "Piano roll (M4)" },
    };

    bool sameKey (const KeyBinding& b, const juce::KeyPress& key)
    {
        return juce::KeyPress (b.keyCode, juce::ModifierKeys (b.modifiers), 0) == key;
    }
}

std::span<const ApplicationCommandEntry> getApplicationCommandTable()   { return table; }
std::span<const KeyBinding> getKeyBindings()                            { return bindings; }
std::span<const PendingShortcut> getPendingShortcuts()                  { return pending; }

std::span<const char* const> getMenuNames()
{
    static const char* const names[] = { "File", "Edit", "Create", "View", "Options", "Help" };
    return names;
}

juce::PopupMenu createCommandMenu (juce::ApplicationCommandManager& manager, const juce::String& menuName)
{
    juce::PopupMenu menu;

    for (auto& entry : table)
        if (menuName == entry.category && manager.getCommandForID (entry.applicationCommandID) != nullptr)
            menu.addCommandItem (&manager, entry.applicationCommandID);

    if (menuName == "Help")
        if (auto* app = juce::JUCEApplicationBase::getInstance())
            menu.addItem (app->getApplicationName() + " " + app->getApplicationVersion(), false, false, nullptr);

    return menu;
}

const ApplicationCommandEntry* findApplicationCommand (juce::CommandID id)
{
    for (auto& e : table)
        if (e.applicationCommandID == id)
            return &e;

    return nullptr;
}

juce::KeyPress findShortcut (const juce::String& commandId)
{
    for (auto& b : bindings)
        if (commandId == b.commandId)
            return juce::KeyPress (b.keyCode, juce::ModifierKeys (b.modifiers), 0);

    return {};
}

const KeyBinding* findBinding (const juce::KeyPress& key, int context)
{
    const KeyBinding* global = nullptr;

    for (auto& b : bindings)
    {
        if (! sameKey (b, key))
            continue;

        if (b.contexts == anyView)
            global = &b;
        else if ((b.contexts & context) != 0)
            return &b;
    }

    return global;
}

juce::var bindingArgs (const KeyBinding& b)
{
    if (b.argument == KeyBinding::noArgument)
        return {};

    auto args = new juce::DynamicObject();
    args->setProperty ("argument", b.argument);
    return args;
}

} // namespace resamper
