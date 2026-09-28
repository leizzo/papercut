#include "ApplicationCommandTable.h"

#include <array>

namespace papercut
{

namespace
{
    using MK = juce::ModifierKeys;
    constexpr int cmd = MK::commandModifier;
    constexpr int shift = MK::shiftModifier;
    constexpr int alt = MK::altModifier;

    const std::array table
    {
        ApplicationCommandEntry { 0x1001, "project.new",             "File",    'N',                            cmd },
        ApplicationCommandEntry { 0x1002, "project.open",            "File",    'O',                            cmd },
        ApplicationCommandEntry { 0x1003, "project.save",            "File",    'S',                            cmd },
        ApplicationCommandEntry { 0x1004, "project.saveAs",          "File",    'S',                            cmd | shift },
        ApplicationCommandEntry { 0x5005, "project.autosave",        "File",    'S',                            cmd | alt },
        ApplicationCommandEntry { 0x5006, "project.recover",         "File",    0,                              0 },
        ApplicationCommandEntry { 0x5007, "project.saveTemplate",    "File",    0,                              0 },
        ApplicationCommandEntry { 0x5008, "project.newFromTemplate", "File",    0,                              0 },
        ApplicationCommandEntry { 0x5004, "file.exportMix",          "File",    'E',                            cmd | shift },

        ApplicationCommandEntry { 0x2001, "edit.undo",               "Edit",    'Z',                            cmd },
        ApplicationCommandEntry { 0x2002, "edit.redo",               "Edit",    'Z',                            cmd | shift },
        ApplicationCommandEntry { 0x2006, "clip.split",              "Edit",    'E',                            cmd },
        ApplicationCommandEntry { 0x200c, "ui.escape",               "Edit",    juce::KeyPress::escapeKey,      0 },
        ApplicationCommandEntry { 0x200a, "clip.duplicate",          "Edit",    'D',                            cmd },
        ApplicationCommandEntry { 0x200b, "clip.consolidate",        "Edit",    'J',                            cmd },
        ApplicationCommandEntry { 0x2009, "edit.delete",             "Edit",    juce::KeyPress::deleteKey,      0 },
        ApplicationCommandEntry { 0x2004, "track.remove",            "Edit",    juce::KeyPress::backspaceKey,   cmd },
        ApplicationCommandEntry { 0x5001, "track.freeze",            "Edit",    'F',                            cmd | shift },
        ApplicationCommandEntry { 0x5002, "track.unfreeze",          "Edit",    'F',                            cmd | alt },
        ApplicationCommandEntry { 0x5003, "track.bounce",            "Edit",    'B',                            cmd },

        ApplicationCommandEntry { 0x2003, "track.add",               "Create",  'T',                            cmd },
        ApplicationCommandEntry { 0x2007, "track.addMidi",           "Create",  'T',                            cmd | shift },
        ApplicationCommandEntry { 0x6002, "mixer.addReturn",         "Create",  'T',                            cmd | alt },
        ApplicationCommandEntry { 0x2005, "clip.add",                "Create",  'I',                            cmd },
        ApplicationCommandEntry { 0x2008, "clip.addMidi",            "Create",  'I',                            cmd | shift },

        ApplicationCommandEntry { 0x7001, "view.session",            "View",    0,                              0 },
        ApplicationCommandEntry { 0x7002, "view.arrange",            "View",    0,                              0 },
        ApplicationCommandEntry { 0x7003, "view.mixer",              "View",    'M',                            cmd | alt },
        ApplicationCommandEntry { 0x7004, "view.pianoRoll",          "View",    0,                              0 },
        ApplicationCommandEntry { 0x7005, "view.editor",             "View",    0,                              0 },
        ApplicationCommandEntry { 0x7006, "view.toggleSessionArrange", "View",  juce::KeyPress::tabKey,         0 },
        ApplicationCommandEntry { 0x7007, "view.toggleBrowser",      "View",    'B',                            cmd | alt },
        ApplicationCommandEntry { 0x7008, "view.toggleDetail",       "View",    'L',                            cmd | alt },
        ApplicationCommandEntry { 0x700a, "arrange.zoomIn",          "View",    '=',                            0 },
        ApplicationCommandEntry { 0x700b, "arrange.zoomOut",         "View",    '-',                            0 },
        ApplicationCommandEntry { 0x700c, "arrange.zoomToSelection", "View",    'Z',                            0 },
        ApplicationCommandEntry { 0x700d, "arrange.zoomToSong",      "View",    'Z',                            shift },
        ApplicationCommandEntry { 0x5009, "theme.use",               "View",    0,                              0 },
        ApplicationCommandEntry { 0x4001, "dev.reloadLayout",        "View",    'L',                            cmd | alt | shift },
        ApplicationCommandEntry { 0x4002, "dev.reloadTheme",         "View",    'T',                            cmd | alt | shift },

        ApplicationCommandEntry { 0x3001, "transport.togglePlay",    "Options", juce::KeyPress::spaceKey,       0 },
        ApplicationCommandEntry { 0x3002, "transport.play",          "Options", 0,                              0 },
        ApplicationCommandEntry { 0x3003, "transport.stop",          "Options", 0,                              0 },
        ApplicationCommandEntry { 0x3004, "transport.returnToStart", "Options", juce::KeyPress::homeKey,        0 },
        ApplicationCommandEntry { 0x3005, "transport.record",        "Options", juce::KeyPress::F9Key,          0 },
        ApplicationCommandEntry { 0x3006, "transport.toggleLoop",    "Options", 'L',                            cmd },
        ApplicationCommandEntry { 0x3007, "transport.toggleMetronome", "Options", 'C',                          0 },
        ApplicationCommandEntry { 0x3008, "transport.tapTempo",      "Options", 'T',                            0 },
        ApplicationCommandEntry { 0x7009, "view.toggleFollow",       "Options", 0,                              0 },
        ApplicationCommandEntry { 0x6001, "plugin.scan",             "Options", 'P',                            cmd | shift },
        ApplicationCommandEntry { 0x6003, "session.stopAll",         "Options", 0,                              0 },
        ApplicationCommandEntry { 0x6004, "session.recordToArrangement", "Options", 0,                          0 },
    };
}

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

std::span<const ApplicationCommandEntry> getApplicationCommandTable()
{
    return table;
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
    for (auto& e : table)
        if (commandId == e.commandId && e.keyCode != 0)
            return juce::KeyPress (e.keyCode, juce::ModifierKeys (e.modifiers), 0);

    return {};
}

} // namespace papercut
