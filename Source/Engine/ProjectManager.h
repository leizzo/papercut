#pragma once

#include <juce_core/juce_core.h>
#include <memory>

namespace tracktion::inline engine { class Edit; }

namespace resamper
{

class EngineManager;

/** Owns the Project lifecycle: New, Open, Save, Save As. One Edit per Project.

    A Project is a folder:
        <Name>.tracktionedit   the Edit (engine-owned state)
        project.json           format version + UI State only (ADR-0002)
        Audio/, Cache/         media (recordings among it) and cache subfolders

    A New Project lives in a temporary "untitled" folder, written out like any
    other Project, until it is saved with Save As.
*/
class ProjectManager
{
public:
    static constexpr int projectFormatVersion = 1;
    static constexpr const char* projectFileName = "project.json";

    explicit ProjectManager (EngineManager&);
    ~ProjectManager();

    /** Replaces the current Project with an untitled one holding an empty Edit. */
    void newProject();

    /** Replaces the current Project with the one in the given folder.
        On success, uiState receives the UI State stored in project.json.
        On failure the current Project is left untouched. */
    juce::Result open (const juce::File& projectFolder, juce::var& uiState);

    /** Saves in place. Fails for an untitled Project: use saveAs(). */
    juce::Result save (const juce::var& uiState);

    /** Writes the Project into a new folder and makes that folder the current
        Project. The Project's Audio folder comes along: the Edit refers to
        recordings by paths relative to itself. */
    juce::Result saveAs (const juce::File& projectFolder, const juce::var& uiState);

    bool isUntitled() const noexcept                { return untitled; }
    juce::File getProjectFolder() const noexcept    { return projectFolder; }
    juce::String getProjectName() const;

    tracktion::Edit& getEdit() const noexcept;

    /** Where a Project keeps its media, recordings included. */
    static juce::File getAudioFolder (const juce::File& projectFolder)   { return projectFolder.getChildFile ("Audio"); }

    /** Reads project.json from a Project folder. Fails on a missing file, bad JSON,
        or an unsupported format version. */
    static juce::Result readProjectFile (const juce::File& projectFolder, juce::var& uiState);

private:
    EngineManager& engineManager;
    std::unique_ptr<tracktion::Edit> edit;
    juce::File projectFolder;
    bool untitled = true;

    juce::Result writeProject (const juce::File& folder, const juce::var& uiState);
    void setCurrent (std::unique_ptr<tracktion::Edit>, const juce::File& folder, bool isUntitled);

    JUCE_DECLARE_NON_COPYABLE (ProjectManager)
};

} // namespace resamper
