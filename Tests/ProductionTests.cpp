#include "TestFixture.h"

#include "Commands/ProductionCommands.h"
#include "Engine/Production.h"
#include "UI/Developer/Inspector.h"
#include "UI/Layout/LayoutSource.h"
#include "UI/Theme/ThemeManager.h"

#include <tracktion_engine/tracktion_engine.h>
#include <juce_audio_formats/juce_audio_formats.h>

namespace papercut::test
{

/** Peak of a WAV, or 0 if the file is missing or empty. Same read as RenderTests. */
static float wavPeak (const juce::File& file)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));

    if (reader == nullptr || reader->lengthInSamples == 0)
        return 0.0f;

    juce::AudioBuffer<float> buffer ((int) reader->numChannels, (int) reader->lengthInSamples);
    reader->read (&buffer, 0, buffer.getNumSamples(), 0, true, true);
    return buffer.getMagnitude (0, buffer.getNumSamples());
}

static juce::var sampleUIState()
{
    auto arrangement = std::make_unique<juce::DynamicObject>();
    arrangement->setProperty ("pixelsPerSecond", 80.0);
    auto state = std::make_unique<juce::DynamicObject>();
    state->setProperty ("arrangement", arrangement.release());
    return state.release();
}

struct ProductionTests : juce::UnitTest
{
    ProductionTests() : juce::UnitTest ("Production", "Papercut") {}

    struct Harness
    {
        Fixture f;
        LayoutSource source;
        ThemeManager themes;
        Production production;
        juce::Result themeLoad;

        Harness()
            : themes (source, "themes/dark.json"),
              production (f.projects),
              themeLoad (themes.load())
        {
            registerProductionCommands (f.commands, production, f.model, themes, f.host);
        }
    };

    void runTest() override
    {
        beginTest ("exportMix of an Edit with a sine clip writes a WAV peaking above 0.1");
        {
            Harness h;
            h.f.invoke ("track.add");
            h.f.audioFileToChoose = writeSineWav (h.f.scratchDir().getChildFile ("tone.wav"), 1.0);
            h.f.invoke ("clip.add");

            auto dest = h.f.scratchDir().getChildFile ("mix.wav");
            expect (h.f.invoke ("file.exportMix", exportMixArgs (dest.getFullPathName())));
            expect (h.f.errors.isEmpty(), h.f.errors.joinIntoString ("; "));
            expect (dest.existsAsFile());
            expectGreaterThan (wavPeak (dest), 0.1f);
        }

        beginTest ("bounceTrack of the sine track peaks above 0.1 and an empty track peaks near 0");
        {
            Harness h;
            h.f.invoke ("track.add");
            h.f.invoke ("track.add");
            h.f.audioFileToChoose = writeSineWav (h.f.scratchDir().getChildFile ("tone.wav"), 1.0);
            h.f.invoke ("clip.add");

            const auto tone = h.f.model.getTracks()[0].id;
            const auto empty = h.f.model.getTracks()[1].id;
            expectEquals ((int) h.f.model.getTracks()[0].clips.size(), 1);
            expect (h.f.model.getTracks()[1].clips.empty());

            auto toneFile = h.f.scratchDir().getChildFile ("bounce-tone.wav");
            expect (h.f.invoke ("track.bounce", bounceArgs (tone, toneFile.getFullPathName())));
            expect (h.f.errors.isEmpty(), h.f.errors.joinIntoString ("; "));
            expectGreaterThan (wavPeak (toneFile), 0.1f);

            auto emptyFile = h.f.scratchDir().getChildFile ("bounce-empty.wav");
            expect (h.f.invoke ("track.bounce", bounceArgs (empty, emptyFile.getFullPathName())));
            expect (h.f.errors.isEmpty(), h.f.errors.joinIntoString ("; "));
            expectLessThan (wavPeak (emptyFile), 1.0e-3f);
        }

        beginTest ("saveTemplate + newFromTemplate round-trips the track and its clip");
        {
            Harness h;
            h.f.invoke ("track.add");
            auto tone = writeSineWav (h.f.scratchDir().getChildFile ("tone.wav"), 1.0);
            h.f.audioFileToChoose = tone;
            h.f.invoke ("clip.add");
            h.f.uiState = sampleUIState();
            h.f.projectSaveLocation = h.f.scratchDir().getChildFile ("Saved");
            h.f.invoke ("project.saveAs");
            expect (h.f.errors.isEmpty(), h.f.errors.joinIntoString ("; "));

            auto templ = h.f.scratchDir().getChildFile ("Template");
            expect (h.f.invoke ("project.saveTemplate", saveTemplateArgs (templ.getFullPathName())));
            expect (h.f.errors.isEmpty(), h.f.errors.joinIntoString ("; "));
            expectEquals (h.f.model.getProjectName(), juce::String ("Saved"));
            expect (templ.getChildFile ("project.json").existsAsFile());

            h.f.invoke ("track.add");
            expectEquals (h.f.numTracks(), 2);

            auto dest = h.f.scratchDir().getChildFile ("FromTemplate");
            expect (h.f.invoke ("project.newFromTemplate", newFromTemplateArgs (templ.getFullPathName(), dest.getFullPathName())));
            expect (h.f.errors.isEmpty(), h.f.errors.joinIntoString ("; "));

            auto tracks = h.f.model.getTracks();
            expectEquals ((int) tracks.size(), 1);
            expectEquals ((int) tracks[0].clips.size(), 1);
            expect (tracks[0].clips[0].file == tone);
            expectEquals ((double) h.f.uiState["arrangement"]["pixelsPerSecond"], 80.0);
        }

        beginTest ("autosave then track.add then recover restores the autosaved track count");
        {
            Harness h;
            h.f.invoke ("track.add");
            h.f.uiState = sampleUIState();
            const auto saved = h.f.numTracks();

            expect (h.f.invoke ("project.autosave"));
            expect (h.f.errors.isEmpty(), h.f.errors.joinIntoString ("; "));
            expect (h.production.hasRecovery());

            h.f.invoke ("track.add");
            h.f.uiState = juce::var();
            expectEquals (h.f.numTracks(), saved + 1);

            expect (h.f.invoke ("project.recover"));
            expect (h.f.errors.isEmpty(), h.f.errors.joinIntoString ("; "));
            expectEquals (h.f.numTracks(), saved);
            expectEquals ((double) h.f.uiState["arrangement"]["pixelsPerSecond"], 80.0);
        }

        beginTest ("hasNewerRecovery follows which Edit was written last");
        {
            Harness h;
            h.f.uiState = sampleUIState();
            const auto project = h.f.projects.getProjectFolder();
            expect (! Production::hasNewerRecovery (project));

            expect (h.f.invoke ("project.autosave"));
            expect (h.f.errors.isEmpty(), h.f.errors.joinIntoString ("; "));

            auto recoveryEdits = project.getChildFile ("Recovery").findChildFiles (juce::File::findFiles, false, "*.tracktionedit");
            auto projectEdits = project.findChildFiles (juce::File::findFiles, false, "*.tracktionedit");
            expectEquals (recoveryEdits.size(), 1);
            expect (! projectEdits.isEmpty());

            recoveryEdits[0].setLastModificationTime (projectEdits[0].getLastModificationTime() + juce::RelativeTime::seconds (2));
            expect (Production::hasNewerRecovery (project));

            projectEdits[0].setLastModificationTime (recoveryEdits[0].getLastModificationTime() + juce::RelativeTime::seconds (2));
            expect (! Production::hasNewerRecovery (project));
        }

        beginTest ("theme.use light then dark changes colour and keeps trackHeight");
        {
            Harness h;
            expect (h.themeLoad.wasOk(), h.themeLoad.getErrorMessage());

            const auto background = h.themes.getTheme().background;
            const auto accent = h.themes.getTheme().accent;
            const auto trackHeight = h.themes.getMetrics().trackHeight;
            expectGreaterThan (trackHeight, 0);

            expect (h.f.invoke ("theme.use", themeFileArgs ("themes/light.json")));
            expect (h.f.errors.isEmpty(), h.f.errors.joinIntoString ("; "));
            expect (h.themes.getTheme().background != background);
            expect (h.themes.getTheme().accent != accent);
            expectEquals (h.themes.getMetrics().trackHeight, trackHeight);

            expect (h.f.invoke ("theme.use", themeFileArgs ("themes/dark.json")));
            expect (h.f.errors.isEmpty(), h.f.errors.joinIntoString ("; "));
            expect (h.themes.getTheme().background == background);
            expect (h.themes.getTheme().accent == accent);
            expectEquals (h.themes.getMetrics().trackHeight, trackHeight);
        }

        beginTest ("freeze either freezes the track or fails without a new undo step");
        {
            // Undo follows the engine: frozenIndividually uses a null UndoManager, so the
            // flag is not its own undo step. A successful freeze may still join the open
            // UndoManager transaction via the freeze-point plug-in insert. A failed freeze
            // must not append a transaction.
            Harness h;
            h.f.invoke ("track.add");
            h.f.audioFileToChoose = writeSineWav (h.f.scratchDir().getChildFile ("tone.wav"), 1.0);
            h.f.invoke ("clip.add");

            const auto id = h.f.model.getTracks()[0].id;
            auto& undo = h.f.projects.getEdit().getUndoManager();
            const auto undoCount = undo.getUndoDescriptions().size();
            auto result = h.production.freezeTrack (id);

            if (result.wasOk())
            {
                expect (h.production.isFrozen (id));
                expect (h.production.unfreezeTrack (id).wasOk(), "unfreeze failed");
                expect (! h.production.isFrozen (id));
            }
            else
            {
                expect (! h.production.isFrozen (id));
                expectEquals (undo.getUndoDescriptions().size(), undoCount);
            }
        }

        beginTest ("Inspector shows the inspected component id");
        {
            Harness h;
            juce::Component parent, component;
            parent.setComponentID ("arrangement");
            component.setName ("Lane");
            component.setComponentID ("lane.track1");
            parent.addAndMakeVisible (component);

            Inspector inspector (h.themes);
            inspector.setInspected (&component);

            auto sawId = false, sawParent = false;

            for (int i = 0; i < inspector.getNumChildComponents(); ++i)
                if (auto* label = dynamic_cast<juce::Label*> (inspector.getChildComponent (i)))
                {
                    if (label->getText().contains ("lane.track1"))
                        sawId = true;

                    if (label->getText().contains ("arrangement"))
                        sawParent = true;
                }

            expect (sawId, "inspector did not show the component id");
            expect (sawParent, "inspector did not show the parent id");
            inspector.setInspected (nullptr);
        }
    }
};

static ProductionTests productionTests;

} // namespace papercut::test
