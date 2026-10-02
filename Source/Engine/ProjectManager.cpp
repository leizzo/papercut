#include "ProjectManager.h"
#include "EngineManager.h"

#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion;

namespace resamper
{

namespace
{
    juce::File editFileFor (const juce::File& folder)
    {
        return folder.getChildFile (folder.getFileName()).withFileExtension (te::editFileSuffix);
    }

    std::unique_ptr<te::Edit> createEdit (te::Engine& engine, const juce::ValueTree& state, const juce::File& editFile)
    {
        auto id = te::ProjectItemID::fromProperty (state, te::IDs::projectID);

        if (! id.isValid())
            id = te::ProjectItemID::createNewID (te::ProjectID{});

        te::Edit::Options options { engine, state, id };
        options.editFileRetriever = [editFile] { return editFile; };

        // Warped clips stretch in real time. A proxy is rendered for a clip's exact
        // timing, so every trim or tempo change re-rendered it (#111, ADR 0001).
        options.role = te::Edit::proxiesDisabled;

        // The engine's default adds an audio track to every Edit, including loaded
        // ones. A New Project is an empty Edit, and a loaded Edit keeps what it had.
        options.numAudioTracks = 0;

        auto edit = te::Edit::createEdit (std::move (options));

        // The engine creates the Edit's SCENES node lazily, through the UndoManager,
        // the first time an audio track is added. Left alone, it lands inside the
        // user's first "Add Track" undo step while the engine keeps its cached
        // SceneList, so undoing that step corrupts the Edit. Create it up front and
        // start the Project with an empty undo history.
        edit->getSceneList();

        // SCENES lands after the tracks, where the engine's track sort (run after
        // each new track, through the UndoManager) moves it from. Run later, that
        // move joins whatever undo step is newest, or after an Undo starts one of
        // its own and clears Redo (#133). Sorted here, outside undo, it never moves.
        te::TrackList::sortTracksByType (edit->state, nullptr);
        edit->getUndoManager().clearUndoHistory();

        return edit;
    }
}

ProjectManager::ProjectManager (EngineManager& em)
    : engineManager (em)
{
    newProject();
}

ProjectManager::~ProjectManager() = default;

te::Edit& ProjectManager::getEdit() const noexcept
{
    jassert (edit != nullptr);
    return *edit;
}

juce::String ProjectManager::getProjectName() const
{
    return untitled ? juce::String ("Untitled") : projectFolder.getFileName();
}

void ProjectManager::setCurrent (std::unique_ptr<te::Edit> newEdit, const juce::File& folder, bool isUntitled)
{
    // Destroy the old Edit before the new one takes over the playback context.
    edit.reset();
    edit = std::move (newEdit);
    projectFolder = folder;
    untitled = isUntitled;
}

void ProjectManager::newProject()
{
    auto& engine = engineManager.getEngine();
    auto folder = engine.getTemporaryFileManager().getTempDirectory()
                        .getChildFile ("Untitled-" + juce::Uuid().toString());

    auto id = te::ProjectItemID::createNewID (te::ProjectID{});
    setCurrent (createEdit (engine, te::loadEditFromFile (engine, juce::File(), id), editFileFor (folder)), folder, true);

    // The engine stores a recording's path relative to the Edit file, but measures
    // from the file's folder only if the file exists. Written out, the untitled
    // Project stores paths exactly as a saved one does, so they survive Save As.
    [[maybe_unused]] auto written = writeProject (folder, {});
    jassert (written.wasOk());
}

juce::Result ProjectManager::readProjectFile (const juce::File& folder, juce::var& uiState)
{
    auto file = folder.getChildFile (projectFileName);

    if (! file.existsAsFile())
        return juce::Result::fail ("Missing " + juce::String (projectFileName) + " in " + folder.getFullPathName());

    juce::var json;
    auto parsed = juce::JSON::parse (file.loadFileAsString(), json);

    if (parsed.failed())
        return juce::Result::fail ("Invalid " + juce::String (projectFileName) + ": " + parsed.getErrorMessage());

    if (! json.hasProperty ("version"))
        return juce::Result::fail (juce::String (projectFileName) + " has no version field");

    if ((int) json["version"] > projectFormatVersion)
        return juce::Result::fail ("Project format version " + json["version"].toString()
                                   + " is newer than this app supports (" + juce::String (projectFormatVersion) + ")");

    uiState = json["ui"];
    return juce::Result::ok();
}

juce::Result ProjectManager::open (const juce::File& folder, juce::var& uiState)
{
    auto editFiles = folder.findChildFiles (juce::File::findFiles, false, juce::String ("*") + te::editFileSuffix);

    if (editFiles.size() != 1)
        return juce::Result::fail ("A Project folder must contain exactly one Edit file: " + folder.getFullPathName());

    juce::var savedUIState;

    if (auto r = readProjectFile (folder, savedUIState); r.failed())
        return r;

    auto& engine = engineManager.getEngine();
    auto state = te::loadEditFromFile (engine, editFiles.getFirst(), te::ProjectItemID());
    auto loaded = state.hasType (te::IDs::EDIT) ? createEdit (engine, state, editFiles.getFirst()) : nullptr;

    if (loaded == nullptr)
        return juce::Result::fail ("Could not load " + editFiles.getFirst().getFullPathName());

    setCurrent (std::move (loaded), folder, false);
    uiState = savedUIState;
    return juce::Result::ok();
}

juce::Result ProjectManager::writeProject (const juce::File& folder, const juce::var& uiState)
{
    for (auto sub : { folder, getAudioFolder (folder), folder.getChildFile ("Cache") })
        if (auto r = sub.createDirectory(); r.failed())
            return r;

    auto editFile = editFileFor (folder);

    if (! te::EditFileOperations (*edit).writeToFile (editFile, false))
        return juce::Result::fail ("Could not write " + editFile.getFullPathName());

    // Only what the engine does not serialize.
    auto json = std::make_unique<juce::DynamicObject>();
    json->setProperty ("version", projectFormatVersion);
    json->setProperty ("ui", uiState);

    if (! folder.getChildFile (projectFileName).replaceWithText (juce::JSON::toString (juce::var (json.release()))))
        return juce::Result::fail ("Could not write " + juce::String (projectFileName));

    return juce::Result::ok();
}

juce::Result ProjectManager::importAudio (const juce::File& file, juce::File& playable)
{
    auto& engine = engineManager.getEngine();
    te::AudioFile audioFile (engine, file);

    if (! audioFile.isValid())
        return juce::Result::fail ("Not a readable audio file: " + file.getFullPathName());

    // The engine reads WAV and AIFF directly; anything else it would decode into
    // a proxy, which an Edit without proxies never makes.
    if (! audioFile.getInfo().needsCachedProxy)
    {
        playable = file;
        return juce::Result::ok();
    }

    std::unique_ptr<juce::AudioFormatReader> reader (te::AudioFileUtils::createReaderFor (engine, file));

    if (reader == nullptr)
        return juce::Result::fail ("Not a readable audio file: " + file.getFullPathName());

    const auto folder = getAudioFolder (projectFolder);

    if (auto r = folder.createDirectory(); r.failed())
        return r;

    const auto decoded = folder.getChildFile (file.getFileNameWithoutExtension() + ".wav").getNonexistentSibling (false);
    std::unique_ptr<juce::OutputStream> out (decoded.createOutputStream().release());
    juce::WavAudioFormat wav;

    auto writer = out != nullptr ? wav.createWriterFor (out, juce::AudioFormatWriterOptions()
                                                                 .withSampleRate (reader->sampleRate)
                                                                 .withNumChannels ((int) reader->numChannels)
                                                                 .withBitsPerSample (32)
                                                                 .withSampleFormat (juce::AudioFormatWriterOptions::SampleFormat::floatingPoint))
                                 : nullptr;

    const auto written = writer != nullptr && writer->writeFromAudioReader (*reader, 0, -1);
    writer.reset();   // finishes the file's header

    if (! written)
    {
        decoded.deleteFile();
        return juce::Result::fail ("Could not decode " + file.getFullPathName() + " into " + decoded.getFullPathName());
    }

    playable = decoded;
    return juce::Result::ok();
}

void ProjectManager::setClipSource (te::AudioClipBase& clip, const juce::File& file) const
{
    using Style = te::SourceFileReference::PathStyle;
    clip.getSourceFileReference().setToFile (file, file.isAChildOf (projectFolder) ? Style::alwaysRelative : Style::alwaysAbsolute,
                                             false);
}

juce::Result ProjectManager::save (const juce::var& uiState)
{
    if (untitled)
        return juce::Result::fail ("An untitled Project must be saved with Save As");

    return writeProject (projectFolder, uiState);
}

juce::Result ProjectManager::saveAs (const juce::File& folder, const juce::var& uiState)
{
    if (auto r = writeProject (folder, uiState); r.failed())
        return r;

    if (auto audio = getAudioFolder (projectFolder); audio.isDirectory() && folder != projectFolder)
        if (! audio.copyDirectoryTo (getAudioFolder (folder)))
            return juce::Result::fail ("Could not copy " + audio.getFullPathName() + " into " + folder.getFullPathName());

    edit->editFileRetriever = [f = editFileFor (folder)] { return f; };
    edit->resetChangedStatus();
    projectFolder = folder;
    untitled = false;
    return juce::Result::ok();
}

} // namespace resamper
