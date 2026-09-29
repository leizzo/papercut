#pragma once

#include "CommandRegistry.h"
#include "TrackCommands.h"

namespace resamper
{

struct AppCommandHost;
class ApplicationModel;
class Production;
class ThemeManager;

/** A track to bounce. file is an absolute path; empty asks the host chooser. */
struct BounceArgs
{
    juce::String trackId;
    juce::String file;
};

/** A file: for file.exportMix an absolute path (empty asks the host chooser); for
    theme.use relative to the UI folder, e.g. "themes/light.json". */
struct FileArgs
{
    juce::String file;
};

/** The folder project.saveTemplate writes. */
struct FolderArgs
{
    juce::String folder;
};

/** project.newFromTemplate copies templateFolder to destFolder and opens the copy. */
struct NewFromTemplateArgs
{
    juce::String templateFolder;
    juce::String destFolder;
};

namespace cmd
{
    inline constexpr CommandRef<TrackArgs> trackFreeze { "track.freeze" };
    inline constexpr CommandRef<TrackArgs> trackUnfreeze { "track.unfreeze" };
    inline constexpr CommandRef<BounceArgs> trackBounce { "track.bounce" };
    inline constexpr CommandRef<FileArgs> fileExportMix { "file.exportMix" };
    inline constexpr CommandRef<FolderArgs> projectSaveTemplate { "project.saveTemplate" };
    inline constexpr CommandRef<NewFromTemplateArgs> projectNewFromTemplate { "project.newFromTemplate" };
    inline constexpr CommandRef<> projectAutosave { "project.autosave" };
    inline constexpr CommandRef<> projectRecover { "project.recover" };
    inline constexpr CommandRef<FileArgs> themeUse { "theme.use" };
}

/** Registers the production Commands above. */
void registerProductionCommands (CommandRegistry&, Production&, ApplicationModel&, ThemeManager&, AppCommandHost&);

} // namespace resamper
