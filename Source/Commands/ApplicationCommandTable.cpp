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
        ApplicationCommandEntry { 0x1001, "project.new",             "File",      'N',                            cmd },
        ApplicationCommandEntry { 0x1002, "project.open",            "File",      'O',                            cmd },
        ApplicationCommandEntry { 0x1003, "project.save",            "File",      'S',                            cmd },
        ApplicationCommandEntry { 0x1004, "project.saveAs",          "File",      'S',                            cmd | shift },

        ApplicationCommandEntry { 0x2001, "edit.undo",               "Edit",      'Z',                            cmd },
        ApplicationCommandEntry { 0x2002, "edit.redo",               "Edit",      'Z',                            cmd | shift },
        ApplicationCommandEntry { 0x2003, "track.add",               "Edit",      'T',                            cmd },
        ApplicationCommandEntry { 0x2004, "track.remove",            "Edit",      juce::KeyPress::backspaceKey,   cmd },
        ApplicationCommandEntry { 0x2005, "clip.add",                "Edit",      'I',                            cmd },
        ApplicationCommandEntry { 0x2006, "clip.split",              "Edit",      'E',                            cmd },
        ApplicationCommandEntry { 0x2007, "track.addMidi",           "Edit",      'T',                            cmd | shift },
        ApplicationCommandEntry { 0x2008, "clip.addMidi",            "Edit",      'I',                            cmd | shift },
        ApplicationCommandEntry { 0x2009, "note.delete",             "Edit",      juce::KeyPress::backspaceKey,   0 },

        ApplicationCommandEntry { 0x3001, "transport.togglePlay",    "Transport", juce::KeyPress::spaceKey,       0 },
        ApplicationCommandEntry { 0x3002, "transport.play",          "Transport", 0,                              0 },
        ApplicationCommandEntry { 0x3003, "transport.stop",          "Transport", 0,                              0 },
        ApplicationCommandEntry { 0x3004, "transport.returnToStart", "Transport", juce::KeyPress::homeKey,        0 },
        ApplicationCommandEntry { 0x3005, "transport.record",        "Transport", 'R',                            0 },
        ApplicationCommandEntry { 0x3006, "transport.toggleLoop",    "Transport", 'L',                            0 },

        ApplicationCommandEntry { 0x4001, "dev.reloadLayout",        "Developer", 'L',                            cmd | alt },
        ApplicationCommandEntry { 0x4002, "dev.reloadTheme",         "Developer", 'T',                            cmd | alt },
    };
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

} // namespace papercut
