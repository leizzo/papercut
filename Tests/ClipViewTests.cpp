#include "TestFixture.h"
#include "UI/Arrangement/ClipComponent.h"

namespace resamper::test
{

namespace
{
    /** A MIDI clip, which paints without a waveform to wait for. */
    ClipInfo midiClip (const juce::String& name)
    {
        ClipInfo info;
        info.id = "clip";
        info.name = name;
        info.kind = TrackKind::midi;
        info.lengthSeconds = 4;
        return info;
    }

    /** The clip as painted into a clip-sized image, limited to region when it is
        not empty, as a partial repaint limits it. */
    juce::Image paintClip (ClipComponent& clip, juce::Rectangle<int> region = {})
    {
        juce::Image image (juce::Image::ARGB, clip.getWidth(), clip.getHeight(), true);
        juce::Graphics g (image);

        if (! region.isEmpty())
            g.reduceClipRegion (region);

        clip.paintEntireComponent (g, false);
        return image;
    }

    /** How many pixels in region differ between two images. */
    int differingPixels (const juce::Image& a, const juce::Image& b, juce::Rectangle<int> region)
    {
        int differing = 0;

        for (int y = region.getY(); y < region.getBottom(); ++y)
            for (int x = region.getX(); x < region.getRight(); ++x)
                if (a.getPixelAt (x, y) != b.getPixelAt (x, y))
                    ++differing;

        return differing;
    }

    constexpr int clipWidth = 400, clipHeight = 60, lanesWidth = 300;
}

/** A clip in the Arrangement, as it paints (PRD §8.1). */
struct ClipViewTests : juce::UnitTest
{
    ClipViewTests() : juce::UnitTest ("Clip View", "Resamper") {}

    void runTest() override
    {
        beginTest ("A partial repaint, as the Playhead's, draws what a full paint draws there (#88)");
        {
            Fixture f;
            expect (f.theme.load().wasOk());

            ClipComponent clip (f.model, f.theme, midiClip ("Clip"));
            clip.setTrackLook (juce::Colours::darkorange, false);

            juce::Component lanes;
            lanes.setSize (lanesWidth, clipHeight);
            lanes.addAndMakeVisible (clip);

            // On screen from its start, then starting off-screen.
            for (const int offScreen : { 0, 200 })
            {
                clip.setBounds (-offScreen, 0, clipWidth, clipHeight);
                const juce::Rectangle<int> playheadStrip (offScreen + 100, 0, 20, clipHeight);
                expectEquals (differingPixels (paintClip (clip), paintClip (clip, playheadStrip), playheadStrip), 0);
            }
        }

        beginTest ("A clip that starts off-screen keeps its title at the visible edge");
        {
            Fixture f;
            expect (f.theme.load().wasOk());

            ClipComponent unnamed (f.model, f.theme, midiClip ({}));
            ClipComponent named (f.model, f.theme, midiClip ("Clip"));

            juce::Component lanes;
            lanes.setSize (lanesWidth, clipHeight);
            constexpr int offScreen = 200;

            for (auto* clip : { &unnamed, &named })
            {
                clip->setTrackLook (juce::Colours::darkorange, false);
                lanes.addAndMakeVisible (*clip);
                clip->setBounds (-offScreen, 0, clipWidth, clipHeight);
            }

            const juce::Rectangle<int> visibleStart (offScreen, 0, 40, f.theme.getMetrics().clipHeaderHeight);
            expectGreaterThan (differingPixels (paintClip (unnamed), paintClip (named), visibleStart), 0);
        }
    }
};

static ClipViewTests clipViewTests;

} // namespace resamper::test
