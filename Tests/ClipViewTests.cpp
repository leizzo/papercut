#include "TestFixture.h"
#include "UI/Arrangement/ClipComponent.h"

namespace resamper::test
{

namespace
{
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

            ClipInfo info;
            info.id = "clip";
            info.name = "Clip";
            info.kind = TrackKind::midi;
            info.lengthSeconds = 4;

            ClipComponent clip (f.model, f.theme, info);
            clip.setTrackLook (juce::Colours::darkorange, false);
            clip.setBounds (0, 0, 400, 60);

            const auto full = paintClip (clip);
            const juce::Rectangle<int> strip (200, 0, 20, clip.getHeight());
            expectEquals (differingPixels (full, paintClip (clip, strip), strip), 0);
        }

        beginTest ("A clip that starts off-screen keeps its title at the visible edge");
        {
            Fixture f;
            expect (f.theme.load().wasOk());

            ClipInfo info;
            info.id = "clip";
            info.kind = TrackKind::midi;
            info.lengthSeconds = 4;

            ClipComponent unnamed (f.model, f.theme, info);
            info.name = "Clip";
            ClipComponent named (f.model, f.theme, info);

            juce::Component lanes;
            lanes.setSize (300, 60);

            for (auto* clip : { &unnamed, &named })
            {
                clip->setTrackLook (juce::Colours::darkorange, false);
                lanes.addAndMakeVisible (*clip);
                clip->setBounds (-200, 0, 400, 60);
            }

            const juce::Rectangle<int> visibleStart (200, 0, 40, f.theme.getMetrics().clipHeaderHeight);
            expectGreaterThan (differingPixels (paintClip (unnamed), paintClip (named), visibleStart), 0);
        }
    }
};

static ClipViewTests clipViewTests;

} // namespace resamper::test
