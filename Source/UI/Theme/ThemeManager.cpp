#include "ThemeManager.h"
#include "UI/Layout/LayoutSource.h"

namespace papercut
{

namespace
{
    class PapercutLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        explicit PapercutLookAndFeel (const ThemeManager& tm) : themeManager (tm) {}

        void apply (const Theme& t)
        {
            setColour (juce::ResizableWindow::backgroundColourId, t.background);
            setColour (juce::DocumentWindow::textColourId, t.text);
            setColour (juce::TextButton::buttonColourId, t.panel);
            setColour (juce::TextButton::buttonOnColourId, t.accent);
            setColour (juce::TextButton::textColourOffId, t.text);
            setColour (juce::TextButton::textColourOnId, t.text);
            setColour (juce::ComboBox::outlineColourId, t.mutedText.withAlpha (0.4f));
            setColour (juce::Slider::backgroundColourId, t.background);
            setColour (juce::Slider::trackColourId, t.accent);
            setColour (juce::Slider::thumbColourId, t.text);
            setColour (juce::Slider::rotarySliderFillColourId, t.accent);
            setColour (juce::Slider::rotarySliderOutlineColourId, t.background);
            setColour (juce::BubbleComponent::backgroundColourId, t.panel);
            setColour (juce::BubbleComponent::outlineColourId, t.mutedText.withAlpha (0.4f));
            setColour (juce::TooltipWindow::backgroundColourId, t.panel);
            setColour (juce::TooltipWindow::textColourId, t.text);
            setColour (juce::PopupMenu::backgroundColourId, t.panel);
            setColour (juce::PopupMenu::textColourId, t.text);
            setColour (juce::PopupMenu::highlightedBackgroundColourId, t.accent);
            setColour (juce::AlertWindow::backgroundColourId, t.panel);
            setColour (juce::AlertWindow::textColourId, t.text);
        }

        juce::Font getTextButtonFont (juce::TextButton&, int) override   { return themeManager.getFont(); }
        juce::Font getPopupMenuFont() override                           { return themeManager.getFont(); }

        void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& background,
                                   bool highlighted, bool down) override
        {
            auto colour = background.withMultipliedBrightness (down ? 1.4f : highlighted ? 1.2f : 1.0f);
            auto bounds = b.getLocalBounds().toFloat().reduced (0.5f);
            const auto radius = themeManager.getTheme().cornerRadius;

            g.setColour (colour);
            g.fillRoundedRectangle (bounds, radius);
            g.setColour (b.findColour (juce::ComboBox::outlineColourId));
            g.drawRoundedRectangle (bounds, radius, 1.0f);
        }

        /** Fits any size (V4 insets by a fixed 10 px); a range around zero, like
            pan, fills from zero rather than from the start. */
        void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float position,
                               float startAngle, float endAngle, juce::Slider& slider) override
        {
            const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
            const auto lineWidth = juce::jmax (1.5f, bounds.getWidth() * 0.12f);
            const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - lineWidth;
            const auto centre = bounds.getCentre();

            const auto zero = slider.getMinimum() < 0 && slider.getMaximum() > 0
                                ? (float) slider.valueToProportionOfLength (0.0) : 0.0f;
            const auto angleAt = [&] (float proportion) { return startAngle + proportion * (endAngle - startAngle); };
            const auto stroke = juce::PathStrokeType (lineWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

            juce::Path track, fill;
            track.addCentredArc (centre.x, centre.y, radius, radius, 0, startAngle, endAngle, true);
            fill.addCentredArc (centre.x, centre.y, radius, radius, 0, angleAt (zero), angleAt (position), true);

            g.setColour (slider.findColour (juce::Slider::rotarySliderOutlineColourId));
            g.strokePath (track, stroke);
            g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId));
            g.strokePath (fill, stroke);

            const auto thumb = centre.getPointOnCircumference (radius, angleAt (position));
            g.setColour (slider.findColour (juce::Slider::thumbColourId));
            g.drawLine ({ centre, thumb }, lineWidth * 0.75f);
        }

    private:
        const ThemeManager& themeManager;
    };

    juce::Result readColour (const juce::var& colours, const char* key, juce::Colour& out)
    {
        auto value = colours[key].toString();

        if (! value.startsWith ("#") || (value.length() != 7 && value.length() != 9))
            return juce::Result::fail (juce::String ("colors.") + key + " must be \"#rrggbb\" or \"#rrggbbaa\"");

        auto hex = value.substring (1);
        out = hex.length() == 6 ? juce::Colour ((juce::uint32) (0xff000000u | (juce::uint32) hex.getHexValue32()))
                                : juce::Colour::fromRGBA ((juce::uint8) hex.substring (0, 2).getHexValue32(),
                                                          (juce::uint8) hex.substring (2, 4).getHexValue32(),
                                                          (juce::uint8) hex.substring (4, 6).getHexValue32(),
                                                          (juce::uint8) hex.substring (6, 8).getHexValue32());
        return juce::Result::ok();
    }

    template <typename T>
    juce::Result readNumber (const juce::var& section, const juce::String& path, const char* key, T& out)
    {
        auto value = section[key];

        if (! (value.isInt() || value.isInt64() || value.isDouble()))
            return juce::Result::fail (path + "." + key + " must be a number");

        out = (T) value;
        return juce::Result::ok();
    }
}

ThemeManager::ThemeManager (const LayoutSource& s, juce::String file)
    : source (s), themeFile (std::move (file)),
      lookAndFeel (std::make_unique<PapercutLookAndFeel> (*this))
{
}

ThemeManager::~ThemeManager() = default;

juce::Font ThemeManager::getFont (float scale) const
{
    return juce::Font (juce::FontOptions (theme.fontSize * scale));
}

juce::Result ThemeManager::parse (const juce::String& text, Theme& t, LayoutMetrics& m)
{
    juce::var json;

    if (auto r = juce::JSON::parse (text, json); r.failed())
        return juce::Result::fail ("Theme JSON: " + r.getErrorMessage());

    auto colours = json["colors"];
    auto style = json["style"];
    auto geometry = json["metrics"];

    const std::pair<const char*, juce::Colour Theme::*> colourKeys[] =
    {
        { "background", &Theme::background }, { "panel", &Theme::panel }, { "text", &Theme::text },
        { "mutedText", &Theme::mutedText }, { "accent", &Theme::accent },
        { "laneA", &Theme::laneA }, { "laneB", &Theme::laneB },
        { "trackHeader", &Theme::trackHeader }, { "trackHeaderSelected", &Theme::trackHeaderSelected },
        { "mute", &Theme::mute }, { "solo", &Theme::solo }, { "armed", &Theme::armed },
        { "recording", &Theme::recording }, { "loop", &Theme::loop },
        { "clip", &Theme::clip }, { "clipSelected", &Theme::clipSelected }, { "clipText", &Theme::clipText }, { "waveform", &Theme::waveform },
        { "midiClip", &Theme::midiClip }, { "midiClipSelected", &Theme::midiClipSelected }, { "midiNote", &Theme::midiNote },
        { "pianoWhite", &Theme::pianoWhite }, { "pianoBlack", &Theme::pianoBlack },
        { "noteSelected", &Theme::noteSelected }, { "gridLine", &Theme::gridLine }, { "velocity", &Theme::velocity },
        { "ruler", &Theme::ruler }, { "playhead", &Theme::playhead }, { "error", &Theme::error },
    };

    const std::pair<const char*, int LayoutMetrics::*> metricKeys[] =
    {
        { "transportHeight", &LayoutMetrics::transportHeight }, { "statusBarHeight", &LayoutMetrics::statusBarHeight },
        { "timelineHeight", &LayoutMetrics::timelineHeight }, { "trackHeight", &LayoutMetrics::trackHeight },
        { "trackHeaderWidth", &LayoutMetrics::trackHeaderWidth }, { "clipHeaderHeight", &LayoutMetrics::clipHeaderHeight },
        { "inset", &LayoutMetrics::inset }, { "textPadding", &LayoutMetrics::textPadding },
        { "playheadWidth", &LayoutMetrics::playheadWidth }, { "clipResizeHandleWidth", &LayoutMetrics::clipResizeHandleWidth },
        { "trackControlHeight", &LayoutMetrics::trackControlHeight }, { "trackButtonWidth", &LayoutMetrics::trackButtonWidth },
        { "midiNoteHeight", &LayoutMetrics::midiNoteHeight },
        { "pianoKeyWidth", &LayoutMetrics::pianoKeyWidth }, { "pianoKeyHeight", &LayoutMetrics::pianoKeyHeight },
        { "pianoBlackKeyWidth", &LayoutMetrics::pianoBlackKeyWidth }, { "pianoScrollMargin", &LayoutMetrics::pianoScrollMargin },
        { "velocityLaneHeight", &LayoutMetrics::velocityLaneHeight },
        { "gridEighthPixels", &LayoutMetrics::gridEighthPixels }, { "gridSixteenthPixels", &LayoutMetrics::gridSixteenthPixels },
    };

    Theme newTheme;
    LayoutMetrics newMetrics;

    for (auto& [key, member] : colourKeys)
        if (auto r = readColour (colours, key, newTheme.*member); r.failed())
            return r;

    for (auto r : { readNumber (style, "style", "cornerRadius", newTheme.cornerRadius),
                    readNumber (style, "style", "fontSize", newTheme.fontSize) })
        if (r.failed())
            return r;

    for (auto& [key, member] : metricKeys)
        if (auto r = readNumber (geometry, "metrics", key, newMetrics.*member); r.failed())
            return r;

    t = newTheme;
    m = newMetrics;
    return juce::Result::ok();
}

juce::Result ThemeManager::read (Theme& newTheme, LayoutMetrics& newMetrics) const
{
    juce::String text;

    if (auto r = source.read (themeFile, text); r.failed())
        return r;

    if (auto r = parse (text, newTheme, newMetrics); r.failed())
        return juce::Result::fail (themeFile + ": " + r.getErrorMessage());

    return juce::Result::ok();
}

juce::Result ThemeManager::load()
{
    Theme newTheme;
    LayoutMetrics newMetrics;

    if (auto r = read (newTheme, newMetrics); r.failed())
        return r;

    theme = newTheme;
    metrics = newMetrics;
    static_cast<PapercutLookAndFeel&> (*lookAndFeel).apply (theme);
    return juce::Result::ok();
}

juce::Result ThemeManager::reloadTheme()
{
    Theme newTheme;
    LayoutMetrics ignoredMetrics;

    if (auto r = read (newTheme, ignoredMetrics); r.failed())
        return r;

    theme = newTheme;
    static_cast<PapercutLookAndFeel&> (*lookAndFeel).apply (theme);
    listeners.call ([] (Listener& l) { l.themeChanged(); });
    return juce::Result::ok();
}

juce::Result ThemeManager::useTheme (juce::String file)
{
    auto previous = themeFile;
    themeFile = std::move (file);

    if (auto r = reloadTheme(); r.failed())
    {
        themeFile = std::move (previous);
        return r;
    }

    return juce::Result::ok();
}

} // namespace papercut
