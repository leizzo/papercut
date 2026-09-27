#pragma once

#include <juce_core/juce_core.h>

namespace papercut
{

struct AppCommandHost;
class ApplicationModel;
class CommandRegistry;
class Production;
class ThemeManager;

// Shortcut-table rows for the parent (ApplicationCommandTable is not edited here):
//   track.freeze
//   track.unfreeze
//   track.bounce
//   file.exportMix
//   project.saveTemplate
//   project.newFromTemplate
//   project.autosave
//   project.recover
//   theme.use

/** Registers production Commands:

    track.freeze            track.unfreeze
    track.bounce            file.exportMix
    project.saveTemplate    project.newFromTemplate
    project.autosave        project.recover
    theme.use
*/
void registerProductionCommands (CommandRegistry&, Production&, ApplicationModel&, ThemeManager&, AppCommandHost&);

/** Arguments for track.freeze and track.unfreeze. */
juce::var trackIdArgs (const juce::String& trackId);

/** Arguments for track.bounce. file is an absolute path; empty asks the host chooser. */
juce::var bounceArgs (const juce::String& trackId, const juce::String& file);

/** Arguments for file.exportMix. file is an absolute path; empty asks the host chooser. */
juce::var exportMixArgs (const juce::String& file);

/** Arguments for project.saveTemplate. */
juce::var saveTemplateArgs (const juce::String& folder);

/** Arguments for project.newFromTemplate. */
juce::var newFromTemplateArgs (const juce::String& templateFolder, const juce::String& destFolder);

/** Arguments for theme.use. file is relative to the UI folder, e.g. "themes/light.json". */
juce::var themeFileArgs (const juce::String& file);

} // namespace papercut
