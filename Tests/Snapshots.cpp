#include "TestFixture.h"
#include "UI/MainWindow/MainComponent.h"

#include <tracktion_engine/tracktion_engine.h>

namespace resamper::test
{

/** Not a test: renders the whole window offscreen to PNGs in /tmp/resamper-snapshots,
    one per view, for eyeballing against the design. Run with
    `ResamperTests --snapshot`. */
struct Snapshots : juce::UnitTest
{
    Snapshots() : juce::UnitTest ("Window snapshots", "Snapshot") {}

    void runTest() override
    {
        beginTest ("Render every view");

        Fixture f;
        expect (f.theme.load().wasOk());
        juce::LookAndFeel::setDefaultLookAndFeel (&f.theme.getLookAndFeel());

        // Some content to look at.
        f.invoke ("track.add");
        f.invoke ("track.addMidi");
        f.audioFileToChoose = writeSineWav (f.scratchDir().getChildFile ("tone.wav"), 4.0);
        f.model.selectTrack (f.model.getTracks()[0].id);
        f.invoke ("clip.add");
        f.model.selectTrack (f.model.getTracks()[1].id);
        f.invoke ("clip.addMidi");
        f.invoke ("mixer.addReturn");
        f.plugins.insert (f.model.getTracks()[0].id, tracktion::ReverbPlugin::xmlTypeName);
        f.plugins.insert (f.model.getTracks()[0].id, tracktion::CompressorPlugin::xmlTypeName, PluginChain::mixer);
        f.plugins.insert (f.model.getTracks()[0].id, tracktion::DelayPlugin::xmlTypeName);
        f.invoke ("note.add", noteAddArgs (f.model.getTracks()[1].clips[0].id, 0.0, 0.25, 60));
        f.invoke ("note.add", noteAddArgs (f.model.getTracks()[1].clips[0].id, 0.5, 0.25, 64));
        f.invoke ("note.add", noteAddArgs (f.model.getTracks()[1].clips[0].id, 1.0, 0.5, 67));
        f.invoke ("track.toggleSolo", trackArgs (f.model.getTracks()[1].id));
        f.invoke ("transport.setLoopRange", loopRangeArgs (0.0, 8.0));
        f.model.selectClip (f.model.getTracks()[0].clips[0].id);

        juce::ApplicationCommandManager commandManager;
        const auto size = juce::Point<int> (juce::SystemStats::getEnvironmentVariable ("SNAPSHOT_W", "1600").getIntValue(),
                                            juce::SystemStats::getEnvironmentVariable ("SNAPSHOT_H", "1000").getIntValue());
        {
            MainComponent main (f.app, commandManager);
            main.setSize (size.x, size.y);

            auto dir = juce::File ("/tmp/resamper-snapshots");
            dir.createDirectory();

            for (auto* view : { "session", "arrange", "mixer", "pianoRoll", "editor" })
            {
                f.invoke ((juce::String ("view.") + view).toRawUTF8());
                juce::MessageManager::getInstance()->runDispatchLoopUntil (200);
                main.resized();
                auto image = main.createComponentSnapshot (main.getLocalBounds(), true, 2.0f);
                auto file = dir.getChildFile (juce::String (view) + ".png");
                file.deleteFile();
                juce::FileOutputStream out (file);
                juce::PNGImageFormat().writeImageToStream (image, out);
            }
        }

        juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
    }
};

static Snapshots snapshots;

} // namespace resamper::test
