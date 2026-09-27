#include "TestFixture.h"

#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion;

namespace papercut::test
{

/** Feeds audio into the engine through its hosted device, in place of a
    sound card: the engine's inputs are the hosted device's input channels. */
struct HostedAudio
{
    HostedAudio()
    {
        // Blocks are pushed far faster than real time; don't let the CPU guard mute them.
        deviceManager.setCpuLimitBeforeMuting (1000.0);

        te::HostedAudioDeviceInterface::Parameters params;
        params.sampleRate = sampleRate;
        params.blockSize = blockSize;
        params.inputChannels = 2;
        params.outputChannels = 2;
        io.initialise (params);
        io.prepareToPlay (sampleRate, blockSize);
        deviceManager.dispatchPendingUpdates();
    }

    ~HostedAudio()
    {
        deviceManager.deviceManager.closeAudioDevice();
        deviceManager.removeHostedAudioDeviceInterface();
    }

    /** Runs the engine for this long with a sine wave on every input. */
    void process (double seconds)
    {
        const auto total = (int) (seconds * sampleRate);
        juce::MidiBuffer midi;

        for (int done = 0; done < total; done += blockSize)
        {
            juce::AudioBuffer<float> block (2, std::min (blockSize, total - done));

            for (int ch = 0; ch < block.getNumChannels(); ++ch)
                for (int i = 0; i < block.getNumSamples(); ++i)
                    block.setSample (ch, i, 0.5f * (float) std::sin (juce::MathConstants<double>::twoPi * 440.0 * (done + i) / sampleRate));

            io.processBlock (block, midi);
        }
    }

    static constexpr double sampleRate = 44100.0;
    static constexpr int blockSize = 512;

    te::DeviceManager& deviceManager { getEngineManager().getEngine().getDeviceManager() };
    te::HostedAudioDeviceInterface& io { deviceManager.getHostedAudioDeviceInterface() };
};

struct RecordingTests : juce::UnitTest
{
    RecordingTests() : juce::UnitTest ("Recording", "Papercut") {}

    /** A Project with one track, running on the hosted device (created first,
        so the Edit sees its inputs). */
    struct RecordingFixture : HostedAudio, Fixture
    {
        RecordingFixture()   { invoke ("track.add"); }

        TrackInfo track (int index = 0) const        { return model.getTracks()[(size_t) index]; }
        juce::String trackId (int index = 0) const   { return track (index).id; }

        /** Lets the engine rebuild its playback graph after input changes. */
        void settle()   { projects.getEdit().dispatchPendingUpdatesSynchronously(); }

        /** Records for this long onto the armed tracks, then stops. */
        void recordFor (double seconds)
        {
            settle();
            invoke ("transport.record");
            process (seconds);
            invoke ("transport.stop");
        }

        int undoDepth()
        {
            int steps = 0;

            while (model.canUndo())
            {
                model.undo();
                ++steps;
            }

            while (model.canRedo())
                model.redo();

            return steps;
        }
    };

    void runTest() override
    {
        beginTest ("The engine's audio inputs are listed; a new track has none and is not armed");
        {
            RecordingFixture f;
            expect (! f.model.getAudioInputs().isEmpty());
            expect (f.track().input.isEmpty());
            expect (! f.track().armed);
        }

        beginTest ("track.toggleArm arms a track, giving it the first input if it has none");
        {
            RecordingFixture f;
            const auto steps = f.undoDepth();

            f.invoke ("track.toggleArm", trackArgs (f.trackId()));
            expect (f.track().armed);
            expectEquals (f.track().input, f.model.getAudioInputs()[0]);

            f.invoke ("track.toggleArm", trackArgs (f.trackId()));
            expect (! f.track().armed);
            expectEquals (f.track().input, f.model.getAudioInputs()[0]);   // disarming keeps the input

            expectEquals (f.undoDepth(), steps);   // never undoable
        }

        beginTest ("track.setInput chooses a track's input; an empty name removes it");
        {
            RecordingFixture f;
            const auto inputs = f.model.getAudioInputs();
            const auto last = inputs[inputs.size() - 1];

            f.invoke ("track.setInput", trackInputArgs (f.trackId(), last));
            expectEquals (f.track().input, last);

            f.invoke ("track.setInput", trackInputArgs (f.trackId(), "no-such-input"));
            expectEquals (f.track().input, last);

            f.invoke ("track.toggleArm", trackArgs (f.trackId()));
            expect (f.track().armed);

            f.invoke ("track.setInput", trackInputArgs (f.trackId(), {}));
            expect (f.track().input.isEmpty());
            expect (! f.track().armed);
        }

        beginTest ("Two tracks may record from one input");
        {
            RecordingFixture f;
            f.invoke ("track.add");
            f.invoke ("track.toggleArm", trackArgs (f.trackId (0)));
            f.invoke ("track.toggleArm", trackArgs (f.trackId (1)));
            expect (f.track (0).armed && f.track (1).armed);

            f.recordFor (0.5);
            expectEquals ((int) f.track (0).clips.size(), 1);
            expectEquals ((int) f.track (1).clips.size(), 1);
        }

        //==============================================================================
        beginTest ("transport.record records the armed track's input into a clip in the Project's Audio folder");
        {
            RecordingFixture f;
            f.invoke ("track.toggleArm", trackArgs (f.trackId()));
            f.recordFor (1.0);

            expect (! f.model.isRecording());
            expect (! f.model.isPlaying());

            const auto clips = f.track().clips;
            expectEquals ((int) clips.size(), 1);

            if (clips.size() == 1)
            {
                expectWithinAbsoluteError (clips[0].startSeconds, 0.0, 1e-6);
                expectWithinAbsoluteError (clips[0].lengthSeconds, 1.0, 0.05);
                expect (clips[0].file.existsAsFile());
                expect (clips[0].file.isAChildOf (f.projects.getProjectFolder().getChildFile ("Audio")),
                        clips[0].file.getFullPathName());
                expectEquals (clips[0].numTakes, 0);
            }
        }

        beginTest ("transport.record reports why it can't record: nothing armed, or a loop under 2 seconds");
        {
            RecordingFixture f;
            f.invoke ("transport.record");
            expect (! f.model.isRecording());
            expectEquals (f.errors.size(), 1);

            f.invoke ("track.toggleArm", trackArgs (f.trackId()));
            f.invoke ("transport.setLoopRange", loopRangeArgs (0.0, ApplicationModel::minLoopRecordingSeconds - 0.5));
            f.invoke ("transport.record");
            expect (! f.model.isRecording());
            expectEquals (f.errors.size(), 2);

            f.invoke ("transport.toggleLoop");   // off: the loop no longer matters
            f.invoke ("transport.record");
            expect (f.model.isRecording());
            expectEquals (f.errors.size(), 2);
            f.invoke ("transport.stop");
        }

        beginTest ("Unarmed tracks record nothing");
        {
            RecordingFixture f;
            f.invoke ("track.add");
            f.invoke ("track.toggleArm", trackArgs (f.trackId (1)));
            f.recordFor (0.5);

            expect (f.track (0).clips.empty());
            expectEquals ((int) f.track (1).clips.size(), 1);
        }

        beginTest ("A recording is one undo step");
        {
            RecordingFixture f;
            f.invoke ("track.toggleArm", trackArgs (f.trackId()));
            const auto steps = f.undoDepth();
            f.recordFor (0.5);

            expectEquals (f.undoDepth(), steps + 1);
            f.invoke ("edit.undo");
            expect (f.track().clips.empty());
            expectEquals (f.numTracks(), 1);

            f.invoke ("edit.redo");
            expectEquals ((int) f.track().clips.size(), 1);
        }

        beginTest ("Returning to the start while recording ends the recording as its own undo step");
        {
            RecordingFixture f;
            f.invoke ("track.toggleArm", trackArgs (f.trackId()));
            const auto steps = f.undoDepth();
            f.settle();
            f.invoke ("transport.record");
            f.process (0.5);
            f.invoke ("transport.returnToStart");

            expect (! f.model.isRecording());
            expectEquals ((int) f.track().clips.size(), 1);
            expectEquals (f.undoDepth(), steps + 1);
        }

        beginTest ("The loop can't change while recording");
        {
            RecordingFixture f;
            f.invoke ("track.toggleArm", trackArgs (f.trackId()));
            f.invoke ("transport.setLoopRange", loopRangeArgs (0.0, 2.0));
            f.settle();
            f.invoke ("transport.record");
            f.process (0.5);

            f.invoke ("transport.toggleLoop");
            f.invoke ("transport.setLoopRange", loopRangeArgs (0.0, 3.0));
            expect (f.model.isLooping());
            expectWithinAbsoluteError (f.model.getLoopRange().end, 2.0, 1e-6);
            f.invoke ("transport.stop");
        }

        beginTest ("While recording, the model reports each armed track's recording so far");
        {
            RecordingFixture f;
            f.invoke ("track.toggleArm", trackArgs (f.trackId()));
            f.settle();
            expect (f.model.getRecordings().empty());

            f.invoke ("transport.record");
            f.process (0.5);
            expect (f.model.isRecording());

            const auto recordings = f.model.getRecordings();
            expectEquals ((int) recordings.size(), 1);

            if (recordings.size() == 1)
            {
                expectEquals (recordings[0].trackId, f.trackId());
                expectWithinAbsoluteError (recordings[0].startSeconds, 0.0, 1e-6);
                expectWithinAbsoluteError (recordings[0].lengthSeconds, 0.5, 0.05);

                expect (f.model.createRecordingWaveform (f.trackId()) != nullptr);
            }

            f.invoke ("transport.stop");
            expect (f.model.getRecordings().empty());
        }

        beginTest ("A recording in an untitled Project moves with it on Save As");
        {
            RecordingFixture f;
            f.invoke ("track.toggleArm", trackArgs (f.trackId()));
            f.recordFor (0.5);

            f.projectSaveLocation = f.scratchDir().getChildFile ("Recorded");
            f.invoke ("project.saveAs");
            expect (f.errors.isEmpty(), f.errors.joinIntoString ("; "));

            f.invoke ("project.new");
            f.projectToOpen = f.projectSaveLocation;
            f.invoke ("project.open");

            const auto clips = f.track().clips;
            expectEquals ((int) clips.size(), 1);

            if (clips.size() == 1)
            {
                expect (clips[0].file.existsAsFile());
                expect (clips[0].file.isAChildOf (f.projectSaveLocation), clips[0].file.getFullPathName());
            }
        }

        //==============================================================================
        beginTest ("transport.setLoopRange sets the loop and turns looping on; transport.toggleLoop flips it");
        {
            RecordingFixture f;
            const auto steps = f.undoDepth();
            expect (! f.model.isLooping());

            f.invoke ("transport.setLoopRange", loopRangeArgs (1.0, 3.0));
            expect (f.model.isLooping());
            expectWithinAbsoluteError (f.model.getLoopRange().start, 1.0, 1e-6);
            expectWithinAbsoluteError (f.model.getLoopRange().end, 3.0, 1e-6);

            f.invoke ("transport.setLoopRange", loopRangeArgs (3.0, 3.0));   // empty: ignored
            expectWithinAbsoluteError (f.model.getLoopRange().end, 3.0, 1e-6);

            f.invoke ("transport.toggleLoop");
            expect (! f.model.isLooping());
            f.invoke ("transport.toggleLoop");
            expect (f.model.isLooping());

            expectEquals (f.undoDepth(), steps);   // transport state is never undoable
        }

        beginTest ("Loop recording makes one clip holding a take per pass");
        {
            RecordingFixture f;
            f.invoke ("track.toggleArm", trackArgs (f.trackId()));
            f.invoke ("transport.setLoopRange", loopRangeArgs (0.0, 2.0));
            f.recordFor (5.0);

            const auto clips = f.track().clips;
            expectEquals ((int) clips.size(), 1);

            if (clips.size() == 1)
            {
                expectWithinAbsoluteError (clips[0].lengthSeconds, 2.0, 0.05);
                expectEquals (clips[0].numTakes, 3);
                expect (juce::isPositiveAndBelow (clips[0].currentTake, 3));
            }
        }

        beginTest ("clip.setTake switches a clip's take as one undo step");
        {
            RecordingFixture f;
            f.invoke ("track.toggleArm", trackArgs (f.trackId()));
            f.invoke ("transport.setLoopRange", loopRangeArgs (0.0, 2.0));
            f.recordFor (5.0);

            if (f.track().clips.empty())
            {
                expect (false, "the loop recording made no clip");
                return;
            }

            auto clip = f.track().clips[0];
            const auto original = clip.currentTake;
            const auto other = original == 0 ? 1 : 0;
            const auto steps = f.undoDepth();

            f.invoke ("clip.setTake", clipTakeArgs (clip.id, other));
            expectEquals (f.track().clips[0].currentTake, other);
            expect (f.track().clips[0].file != clip.file);
            expect (f.track().clips[0].file.existsAsFile());

            f.invoke ("clip.setTake", clipTakeArgs (clip.id, other));   // already current
            f.invoke ("clip.setTake", clipTakeArgs (clip.id, 7));       // no such take
            expectEquals (f.undoDepth(), steps + 1);

            f.invoke ("edit.undo");
            expectEquals (f.track().clips[0].currentTake, original);
            expect (f.track().clips[0].file == clip.file);
        }
    }
};

static RecordingTests recordingTests;

} // namespace papercut::test
