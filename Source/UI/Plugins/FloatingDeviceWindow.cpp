#include "FloatingDeviceWindow.h"

namespace resamper
{

namespace
{
    constexpr int foregroundPollMs = 250;

    const TypeStyle trackStyle { 10.0f, false, 400 }, nameStyle { 11.5f, false, 600 }, controlStyle { 9.5f, false, 600 },
                    slotStyle { 9.0f, false, 700 };
}

//==============================================================================
FloatingDeviceWindow::ChromeButton::ChromeButton (ThemeManager& tm, const juce::String& name, Kind k, std::optional<Icon> i)
    : ThemedButton (tm, name), kind (k), icon (i)
{
    setTitle (name);
    setTooltip (name);
    setButtonText (name);
    setWantsKeyboardFocus (true);
}

void FloatingDeviceWindow::ChromeButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto& theme = themeManager.getTheme();
    const auto on = getToggleState();
    const auto bounds = getLocalBounds().toFloat();

    switch (kind)
    {
        case Kind::icon:
        {
            if (highlighted || down)
            {
                g.setColour (theme.bgHover);
                g.fillRoundedRectangle (bounds, theme.radiusMd);
            }

            const auto colour = on && accentWhenOn ? theme.accent : theme.textSecondary;
            drawIcon (g, *icon, bounds.withSizeKeepingCentre (12.0f, 12.0f), colour);
            break;
        }

        case Kind::bypass:
        {
            // On: a 10 % accent wash, accent-dim border, accent power and label.
            const auto frame = bounds.reduced (0.5f);
            g.setColour (on ? theme.accent.withAlpha (0.1f) : highlighted || down ? theme.bgHover : theme.bgSlot);
            g.fillRoundedRectangle (frame, controlRadius);
            g.setColour (on ? theme.accentDim : theme.border);
            g.drawRoundedRectangle (frame, controlRadius, 1.0f);
            auto content = getLocalBounds().reduced (7, 0);
            drawIcon (g, Icon::power, content.removeFromLeft (10).toFloat().withSizeKeepingCentre (10.0f, 10.0f),
                      on ? theme.accent : theme.textDim);
            content.removeFromLeft (4);
            drawStyledText (g, themeManager, on ? "On" : "Off", controlStyle, content, juce::Justification::centredLeft,
                            on ? theme.accent : theme.textSecondary);
            break;
        }

        case Kind::text:
        {
            g.setColour (highlighted || down ? theme.bgHover : theme.bgSlot);
            g.fillRoundedRectangle (bounds.reduced (0.5f), controlRadius);
            auto content = getLocalBounds().reduced (6, 0);

            if (icon)
                drawIcon (g, *icon, content.removeFromRight (10).toFloat().withSizeKeepingCentre (9.0f, 9.0f), theme.textDim);

            drawStyledText (g, themeManager, getButtonText(), controlStyle, content,
                            icon ? juce::Justification::centredLeft : juce::Justification::centred, theme.textPrimary);
            break;
        }

        case Kind::slot:
        {
            if (on)
            {
                g.setColour (theme.accent);
                g.fillRoundedRectangle (bounds, theme.radiusSm);
            }

            drawStyledText (g, themeManager, getButtonText(), slotStyle, getLocalBounds(), juce::Justification::centred,
                            on ? theme.textOnAccent : theme.textSecondary);
            break;
        }
    }

    paintFocus (g, kind == Kind::slot ? theme.radiusSm : controlRadius);
}

//==============================================================================
/** Any click in the window, its content's included, selects its track. */
struct FloatingDeviceWindow::ClickWatch : juce::MouseListener
{
    explicit ClickWatch (FloatingDeviceWindow& w) : window (w) {}

    void mouseDown (const juce::MouseEvent&) override
    {
        if (window.onActivated)
            window.onActivated();
    }

    FloatingDeviceWindow& window;
};

//==============================================================================
FloatingDeviceWindow::FloatingDeviceWindow (ThemeManager& tm, const PluginInfo& info, const juce::String& track,
                                            const juce::String& componentId)
    : themeManager (tm), plugin (info), trackName (track),
      pin (std::make_unique<ChromeButton> (tm, "Pin (keep on top)", ChromeButton::Kind::icon, Icon::pin)),
      close (std::make_unique<ChromeButton> (tm, "Close", ChromeButton::Kind::icon, Icon::x)),
      clickWatch (std::make_unique<ClickWatch> (*this)),
      foregroundWatch ([this] { updateAlwaysOnTop(); })
{
    setComponentID (componentId);
    setOpaque (false);
    setWantsKeyboardFocus (true);
    setFocusContainerType (FocusContainerType::keyboardFocusContainer);

    pin->accentWhenOn = true;
    pin->setClickingTogglesState (false);
    pin->setComponentID ("pin");
    close->setComponentID ("close");

    pin->onClick = [this]
    {
        setPinned (! pinned);

        if (onPinChanged)
            onPinChanged (pinned);
    };
    close->onClick = [this] { if (onCloseRequested) onCloseRequested(); };

    addAndMakeVisible (*pin);
    addAndMakeVisible (*close);
    addMouseListener (clickWatch.get(), true);

    // The frame's shadow (L3) is drawn in this margin.
    for (auto& shadow : tm.getTheme().elevation3)
        margin = { juce::jmax (margin.getTop(), shadow.radius - shadow.offset.y),
                   juce::jmax (margin.getLeft(), shadow.radius - shadow.offset.x),
                   juce::jmax (margin.getBottom(), shadow.radius + shadow.offset.y),
                   juce::jmax (margin.getRight(), shadow.radius + shadow.offset.x) };

    updateAlwaysOnTop();
    foregroundWatch.startTimer (foregroundPollMs);
}

FloatingDeviceWindow::~FloatingDeviceWindow()
{
    removeMouseListener (clickWatch.get());
}

void FloatingDeviceWindow::setPinned (bool shouldPin)
{
    pinned = shouldPin;
    pin->setToggleState (pinned, juce::dontSendNotification);
    pin->setTitle (pinned ? "Unpin" : "Pin (keep on top)");
    updateAlwaysOnTop();
}

void FloatingDeviceWindow::updateAlwaysOnTop()
{
    // Pinned: on top always. Unpinned: floating while Resamper is in front, so the main window never buries it.
    const auto onTop = pinned || juce::Process::isForegroundProcess();

    if (isAlwaysOnTop() != onTop)
        setAlwaysOnTop (onTop);
}

juce::Rectangle<int> FloatingDeviceWindow::frame() const
{
    return margin.subtractedFrom (getLocalBounds());
}

juce::Rectangle<int> FloatingDeviceWindow::titleBar() const
{
    return frame().removeFromTop (themeManager.getMetrics().pluginTitleBarHeight);
}

juce::Rectangle<int> FloatingDeviceWindow::titleArea() const
{
    return titleBar().withTrimmedLeft (titlePaddingLeft).withRight (pin->getX() - gap);
}

juce::Rectangle<int> FloatingDeviceWindow::getFrameScreenBounds() const
{
    return margin.subtractedFrom (getScreenBounds());
}

void FloatingDeviceWindow::setFramePosition (juce::Point<int> topLeft)
{
    setTopLeftPosition (topLeft - juce::Point<int> (margin.getLeft(), margin.getTop()));
}

void FloatingDeviceWindow::setFrameSize (int width, int height)
{
    const auto topLeft = getFrameScreenBounds().getPosition();
    const auto wasOnDesktop = isOnDesktop();
    setSize (width + margin.getLeftAndRight(), height + margin.getTopAndBottom());

    // Growing or shrinking keeps the frame's top-left where it was.
    if (wasOnDesktop)
        setFramePosition (topLeft);
}

bool FloatingDeviceWindow::hasFocusInside() const
{
    auto* focused = juce::Component::getCurrentlyFocusedComponent();
    return focused != nullptr && (focused == this || isParentOf (focused));
}

void FloatingDeviceWindow::focusHost()
{
    close->grabKeyboardFocus();
}

void FloatingDeviceWindow::layoutTitleBar()
{
    auto title = titleBar().withTrimmedLeft (titlePaddingLeft).withTrimmedRight (titlePaddingRight);
    close->setBounds (title.removeFromRight (iconTarget).withSizeKeepingCentre (iconTarget, iconTarget));
    title.removeFromRight (2);
    pin->setBounds (title.removeFromRight (iconTarget).withSizeKeepingCentre (iconTarget, iconTarget));
}

void FloatingDeviceWindow::drawTitleText (juce::Graphics& g, juce::Rectangle<int>& area, const juce::String& text,
                                          const TypeStyle& style, juce::Colour colour) const
{
    const auto width = juce::jmin (area.getWidth(), juce::GlyphArrangement::getStringWidthInt (themeManager.font (style), text) + 1);
    drawStyledText (g, themeManager, text, style, area.removeFromLeft (width), juce::Justification::centredLeft, colour);
    area.removeFromLeft (gap);
}

void FloatingDeviceWindow::drawTrackAndName (juce::Graphics& g, juce::Rectangle<int>& area) const
{
    auto& theme = themeManager.getTheme();
    drawTitleText (g, area, trackName, trackStyle, theme.textDim);
    drawIcon (g, Icon::chevronRight, area.removeFromLeft (9).toFloat().withSizeKeepingCentre (9.0f, 9.0f), theme.textDim);
    area.removeFromLeft (gap);
    drawTitleText (g, area, plugin.name, nameStyle, theme.textPrimary);
}

void FloatingDeviceWindow::paint (juce::Graphics& g)
{
    auto& theme = themeManager.getTheme();
    const auto f = frame().toFloat();
    const auto radius = theme.radius2xl;

    paintElevation (g, theme.elevation3, f, radius);
    g.setColour (theme.bgElevated);
    g.fillRoundedRectangle (f, radius);

    {
        juce::Graphics::ScopedSaveState save (g);
        juce::Path clip;
        clip.addRoundedRectangle (f, radius);
        g.reduceClipRegion (clip);

        auto bar = titleBar();
        g.setColour (theme.bgPanel);
        g.fillRect (bar);
        g.setColour (theme.border);
        g.fillRect (bar.removeFromBottom (1));

        paintBody (g);
    }

    g.setColour (theme.border);
    g.drawRoundedRectangle (f.reduced (0.5f), radius, 1.0f);

    paintTitle (g);
}

bool FloatingDeviceWindow::hitTest (int x, int y)
{
    // Clicks on the shadow fall through to whatever is behind.
    return frame().contains (x, y);
}

bool FloatingDeviceWindow::keyPressed (const juce::KeyPress& key)
{
    const auto mods = key.getModifiers();

    if (key == juce::KeyPress::escapeKey)
    {
        if (! releaseContentFocus() && onCloseRequested)
            onCloseRequested();

        return true;
    }

    if (key.getKeyCode() == 'W' && mods.isCommandDown() && ! mods.isAltDown() && ! mods.isShiftDown())
    {
        if (onCloseRequested)
            onCloseRequested();

        return true;
    }

    if (key.getKeyCode() == 'P' && mods.isCommandDown() && mods.isAltDown() && ! mods.isShiftDown())
    {
        if (onToggleAll)
            onToggleAll();

        return true;
    }

    return false;
}

void FloatingDeviceWindow::mouseDown (const juce::MouseEvent& e)
{
    // The title bar left of Pin is the drag area.
    dragging = titleBar().withRight (pin->getX()).contains (e.getPosition());

    if (dragging)
        dragger.startDraggingComponent (this, e);
}

void FloatingDeviceWindow::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging)
        dragger.dragComponent (this, e, nullptr);
}

void FloatingDeviceWindow::mouseUp (const juce::MouseEvent& e)
{
    if (std::exchange (dragging, false) && e.mouseWasDraggedSinceMouseDown() && onMoved)
        onMoved();
}

std::unique_ptr<juce::AccessibilityHandler> FloatingDeviceWindow::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler> (*this, juce::AccessibilityRole::window);
}

} // namespace resamper
