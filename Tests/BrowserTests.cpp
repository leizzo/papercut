#include "TestFixture.h"
#include "Commands/PluginCommands.h"
#include "UI/Browser/Library.h"

#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion;

namespace resamper::test
{

/** The Browser's library (PRD §6.2) and what dropping from it does. */
struct BrowserTests : juce::UnitTest
{
    BrowserTests() : juce::UnitTest ("Browser", "Resamper") {}

    static bool lists (const std::vector<LibraryItem>& items, const juce::String& path)
    {
        return std::any_of (items.begin(), items.end(), [&] (const LibraryItem& i) { return i.pluginPath == path; });
    }

    void runTest() override
    {
        using Category = LibraryCategory;

        beginTest ("Plug-in categories come from the catalogue: instruments, audio effects, MIDI effects");
        {
            Fixture f;
            auto& rack = f.plugins;
            Library library ([&rack] { return rack.getCatalogue(); }, f.scratchDir());

            const auto instruments = library.list (Category::instruments, {}, {});
            const auto effects = library.list (Category::audioEffects, {}, {});
            const auto midi = library.list (Category::midiEffects, {}, {});

            expect (lists (instruments, te::FourOscPlugin::xmlTypeName));
            expect (! lists (instruments, te::ReverbPlugin::xmlTypeName));
            expect (lists (effects, te::ReverbPlugin::xmlTypeName));
            expect (! lists (effects, te::FourOscPlugin::xmlTypeName));
            expect (lists (midi, te::MidiModifierPlugin::xmlTypeName));
            expect (! lists (effects, te::MidiModifierPlugin::xmlTypeName));

            // Engine plumbing is not something to drag onto a track.
            for (auto* plumbing : { te::VolumeAndPanPlugin::xmlTypeName, te::LevelMeterPlugin::xmlTypeName,
                                    te::AuxSendPlugin::xmlTypeName, te::AuxReturnPlugin::xmlTypeName })
                expect (! lists (effects, plumbing), plumbing);
        }

        beginTest ("Search filters by name, ignoring case");
        {
            Fixture f;
            auto& rack = f.plugins;
            Library library ([&rack] { return rack.getCatalogue(); }, f.scratchDir());

            const auto found = library.list (Category::audioEffects, {}, "REVERB");
            expectEquals ((int) found.size(), 1);
            expect (lists (found, te::ReverbPlugin::xmlTypeName));
        }

        beginTest ("Samples come from the user folder: subfolders first, then audio files; search looks inside subfolders");
        {
            Fixture f;
            Library library ([] { return juce::Array<PluginInfo>(); }, f.scratchDir());
            const auto samples = library.folderFor (Category::samples);
            writeSineWav (samples.getChildFile ("Kick_Punchy_01.wav"), 0.1);
            writeSineWav (samples.getChildFile ("808 Kits/Kick_Sub_Warm.wav"), 0.1);
            samples.getChildFile ("notes.txt").replaceWithText ("not audio");

            const auto top = library.list (Category::samples, {}, {});
            expectEquals ((int) top.size(), 2);
            expect (top[0].kind == LibraryItem::Kind::folder && top[0].name == "808 Kits");
            expect (top[1].kind == LibraryItem::Kind::audioFile && top[1].name == "Kick_Punchy_01");

            const auto inside = library.list (Category::samples, top[0].file, {});
            expectEquals ((int) inside.size(), 1);
            expectEquals (inside[0].name, juce::String ("Kick_Sub_Warm"));

            const auto searched = library.list (Category::samples, {}, "sub");
            expectEquals ((int) searched.size(), 1);
            expectEquals (searched[0].name, juce::String ("Kick_Sub_Warm"));
        }

        beginTest ("clip.insertAt drops an audio file on a track at a position, as one undo step");
        {
            Fixture f;
            f.invoke ("track.add");
            f.invoke ("track.add");
            const auto trackId = f.model.getTracks()[1].id;
            const auto file = writeSineWav (f.scratchDir().getChildFile ("drop.wav"), 1.0);

            expect (f.invoke ("clip.insertAt", clipInsertAtArgs (file, trackId, 3.0)));
            const auto tracks = f.model.getTracks();
            expect (tracks[0].clips.empty());
            expectEquals ((int) tracks[1].clips.size(), 1);
            expectWithinAbsoluteError (tracks[1].clips[0].startSeconds, 3.0, 1.0e-6);

            f.invoke ("edit.undo");
            expect (f.model.getTracks()[1].clips.empty());
        }

        beginTest ("clip.insertAt refuses a MIDI track and a file that isn't audio");
        {
            Fixture f;
            f.invoke ("track.addMidi");
            const auto trackId = f.model.getTracks()[0].id;
            const auto file = writeSineWav (f.scratchDir().getChildFile ("drop.wav"), 1.0);

            f.invoke ("clip.insertAt", clipInsertAtArgs (file, trackId, 0.0));
            expect (f.model.getTracks()[0].clips.empty());
            expectEquals (f.errors.size(), 1);

            f.invoke ("track.add");
            auto text = f.scratchDir().getChildFile ("notes.txt");
            text.replaceWithText ("x");
            f.invoke ("clip.insertAt", clipInsertAtArgs (text, f.model.getTracks()[1].id, 0.0));
            expect (f.model.getTracks()[1].clips.empty());
            expectEquals (f.errors.size(), 2);
        }

        beginTest ("A sample preview starts for an audio file and not for anything else");
        {
            Fixture f;
            auto& preview = f.preview;
            const auto file = writeSineWav (f.scratchDir().getChildFile ("preview.wav"), 0.5);
            auto text = f.scratchDir().getChildFile ("notes.txt");
            text.replaceWithText ("x");

            expect (! preview.play (text));
            expect (! preview.isPlaying());
            expect (preview.play (file));
            expect (preview.isPlaying());
            expect (preview.getFile() == file);
            preview.stop();
            expect (! preview.isPlaying());
        }
    }
};

static BrowserTests browserTests;

} // namespace resamper::test
