#include "PluginWindow.h"

#include "Commands/CommandRegistry.h"
#include "Commands/EditCommands.h"
#include "Commands/PluginCommands.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace resamper
{

namespace
{
    // Design: PluginWindow. Title bar padding 0 8 0 12, gap 7; toolbar padding 0 10, gap 7;
    // footer padding 0 8 0 12, gap 8. The bar heights are Layout Metrics.
    constexpr int titlePaddingLeft = 12, titlePaddingRight = 8, toolbarPadding = 10, footerPaddingLeft = 12,
                  footerPaddingRight = 8, gap = 7, iconTarget = 22, bypassWidth = 46, controlHeight = 22,
                  presetStepWidth = 18, presetNameWidth = 120, slotWidth = 22, slotHeight = 16, copyWidth = 66,
                  statsWidth = 92, sandboxWidth = 16, scaleWidth = 120, scaleHeight = 18, gripSize = 14,
                  minFrameWidth = 540, loadingHeight = 180, statusPollMs = 500, loadingPollMs = 20;

    const TypeStyle trackStyle { 10.0f, false, 400 }, titleStyle { 11.5f, false, 600 }, vendorStyle { 9.5f, false, 400 },
                    badgeStyle { 7.5f, true, 600 }, controlStyle { 9.5f, false, 600 }, slotStyle { 9.0f, false, 700 },
                    statsStyle { 9.0f, true, 400 }, footerStyle { 8.5f, true, 400 }, stateStyle { 11.0f, false, 400 },
                    stateDetailStyle { 9.5f, false, 400 };

    /** The design's corner for the toolbar's framed controls (Bypass, the preset name, the A/B well). */
    constexpr float controlRadius = 5.0f;

    const juce::String middleDot (juce::CharPointer_UTF8 ("\xc2\xb7"));
    const juce::String rightArrow (juce::CharPointer_UTF8 ("\xe2\x86\x92"));

    juce::String formatBadge (const PluginInfo& info)
    {
        return info.format == "AudioUnit" ? juce::String ("AU") : info.format;
    }

    juce::String vendorOf (const PluginInfo& info)
    {
        return info.manufacturer.isNotEmpty() ? info.manufacturer : juce::String ("Unknown vendor");
    }

    juce::String processText (const PluginInfo& info)
    {
        return info.sandboxed ? "out-of-process" : "in-process";
    }

    /** The plug-in's own editor inside the engine's wrapper, if it is a JUCE AudioProcessorEditor. */
    juce::AudioProcessorEditor* processorEditorIn (juce::Component* c)
    {
        if (c == nullptr)
            return nullptr;

        if (auto* editor = dynamic_cast<juce::AudioProcessorEditor*> (c))
            return editor;

        for (auto* child : c->getChildren())
            if (auto* editor = dynamic_cast<juce::AudioProcessorEditor*> (child))
                return editor;

        return nullptr;
    }

    /** Shown when the plug-in has no editor of its own. */
    struct NoEditor : juce::Component
    {
        explicit NoEditor (ThemeManager& tm) : themeManager (tm)
        {
            setSize (minFrameWidth, loadingHeight);
        }

        void paint (juce::Graphics& g) override
        {
            auto& theme = themeManager.getTheme();
            drawStyledText (g, themeManager, "This plug-in has no editor of its own", stateStyle, getLocalBounds(),
                            juce::Justification::centred, theme.textSecondary);
        }

        ThemeManager& themeManager;
    };
}

//==============================================================================
/** The chrome's buttons: an icon (Pin, Close, the preset steps, undo / redo),
    the framed Bypass (power + On / Off, accent when on), a text button (the
    preset name, Copy A→B, Retry, Run in-process) or an A/B slot (accent when
    selected). Each is focusable and says what it does (§18). */
class PluginWindow::ChromeButton : public ThemedButton
{
public:
    enum class Kind { icon, bypass, text, slot };

    ChromeButton (ThemeManager& tm, const juce::String& name, Kind k, std::optional<Icon> i = {})
        : ThemedButton (tm, name), kind (k), icon (i)
    {
        setTitle (name);
        setTooltip (name);
        setButtonText (name);
        setWantsKeyboardFocus (true);
    }

    std::optional<juce::Colour> onColour;   ///< the icon's colour when on (Pin: accent)

    void paintButton (juce::Graphics& g, bool highlighted, bool down) override
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

                const auto colour = on && onColour ? *onColour : theme.textSecondary;
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
                g.drawRoundedRectangle (frame, 5.0f, 1.0f);
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

private:
    Kind kind;
    std::optional<Icon> icon;
};

//==============================================================================
/** Host text a screen reader reads: the stats, the sandbox status, the footer. Paints itself. */
class PluginWindow::Readout : public juce::Component,
                              public juce::SettableTooltipClient
{
public:
    enum class Kind { stats, sandbox, footer };

    Readout (ThemeManager& tm, Kind k) : themeManager (tm), kind (k)
    {
        setInterceptsMouseClicks (false, false);
    }

    void setText (const juce::String& t, bool good = false)
    {
        if (t == text && good == positive)
            return;

        text = t;
        positive = good;
        setTitle (t);
        setTooltip (t);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto& theme = themeManager.getTheme();

        if (kind == Kind::sandbox)
            drawIcon (g, Icon::shieldCheck, getLocalBounds().toFloat().withSizeKeepingCentre (11.0f, 11.0f),
                      positive ? theme.meterLow : theme.textDim);
        else
            drawNumber (g, themeManager, text, kind == Kind::stats ? statsStyle : footerStyle, getLocalBounds(),
                        kind == Kind::stats ? juce::Justification::centredRight : juce::Justification::centredLeft, theme.textDim);
    }

    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
    {
        return std::make_unique<juce::AccessibilityHandler> (*this, juce::AccessibilityRole::staticText);
    }

private:
    ThemeManager& themeManager;
    Kind kind;
    juce::String text;
    bool positive = false;
};

//==============================================================================
/** The footer's resize grip: dragging it resizes the plug-in's editor (within its own limits). */
class PluginWindow::Grip : public juce::Component
{
public:
    explicit Grip (PluginWindow& w) : window (w)
    {
        setMouseCursor (juce::MouseCursor::BottomRightCornerResizeCursor);
        setTitle ("Resize");
    }

    void paint (juce::Graphics& g) override
    {
        auto& theme = window.themeManager.getTheme();
        g.setColour (theme.textDim);
        const auto r = getLocalBounds().toFloat().reduced (2.0f);

        for (float d = 0; d < r.getWidth(); d += 4.0f)
            g.drawLine (r.getRight() - d, r.getBottom(), r.getRight(), r.getBottom() - d, 1.0f);
    }

    void mouseDown (const juce::MouseEvent&) override
    {
        if (auto* editor = processorEditorIn (window.vendor.get()))
            startSize = { editor->getWidth(), editor->getHeight() };
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        auto* editor = processorEditorIn (window.vendor.get());

        if (editor == nullptr)
            return;

        const auto factor = (float) window.uiScale / 100.0f;
        auto bounds = editor->getBounds().withSize (startSize.x + juce::roundToInt ((float) e.getDistanceFromDragStartX() / factor),
                                                   startSize.y + juce::roundToInt ((float) e.getDistanceFromDragStartY() / factor));

        if (auto* constrainer = editor->getConstrainer())
            constrainer->checkBounds (bounds, editor->getBounds(), {}, false, false, true, true);

        editor->setSize (juce::jmax (8, bounds.getWidth()), juce::jmax (8, bounds.getHeight()));
    }

private:
    PluginWindow& window;
    juce::Point<int> startSize;
};

//==============================================================================
/** Any click in the window, the vendor UI's included, selects its track. */
struct PluginWindow::ClickWatch : juce::MouseListener
{
    explicit ClickWatch (PluginWindow& w) : window (w) {}

    void mouseDown (const juce::MouseEvent&) override
    {
        if (window.onActivated)
            window.onActivated();
    }

    PluginWindow& window;
};

//==============================================================================
PluginWindow::PluginWindow (PluginRack& r, CommandRegistry& c, ThemeManager& tm, const PluginInfo& info,
                            const juce::String& track)
    : rack (r), commands (c), themeManager (tm), plugin (info), trackName (track)
{
    using Kind = ChromeButton::Kind;
    setComponentID ("PluginWindow");
    setOpaque (false);
    setWantsKeyboardFocus (true);
    setFocusContainerType (FocusContainerType::keyboardFocusContainer);

    pin = std::make_unique<ChromeButton> (tm, "Pin (keep on top)", Kind::icon, Icon::pin);
    close = std::make_unique<ChromeButton> (tm, "Close", Kind::icon, Icon::x);
    bypass = std::make_unique<ChromeButton> (tm, "Bypass", Kind::bypass);
    previousPreset = std::make_unique<ChromeButton> (tm, "Previous preset", Kind::icon, Icon::chevronLeft);
    presetName = std::make_unique<ChromeButton> (tm, "Presets", Kind::text, Icon::chevronDown);
    nextPreset = std::make_unique<ChromeButton> (tm, "Next preset", Kind::icon, Icon::chevronRight);
    savePreset = std::make_unique<ChromeButton> (tm, "Save preset", Kind::icon, Icon::save);
    slotA = std::make_unique<ChromeButton> (tm, "A", Kind::slot);
    slotB = std::make_unique<ChromeButton> (tm, "B", Kind::slot);
    copyAToB = std::make_unique<ChromeButton> (tm, "Copy A" + rightArrow + "B", Kind::text);
    undo = std::make_unique<ChromeButton> (tm, "Undo", Kind::icon, Icon::undo2);
    redo = std::make_unique<ChromeButton> (tm, "Redo", Kind::icon, Icon::redo2);
    retry = std::make_unique<ChromeButton> (tm, "Retry", Kind::text);
    runInProcess = std::make_unique<ChromeButton> (tm, "Run in-process", Kind::text);
    stats = std::make_unique<Readout> (tm, Readout::Kind::stats);
    sandbox = std::make_unique<Readout> (tm, Readout::Kind::sandbox);
    footerInfo = std::make_unique<Readout> (tm, Readout::Kind::footer);
    scale = std::make_unique<Segmented> (tm, juce::StringArray { "100%", "150%", "200%" });
    grip = std::make_unique<Grip> (*this);
    clickWatch = std::make_unique<ClickWatch> (*this);

    pin->onColour = tm.getTheme().accent;
    pin->setClickingTogglesState (false);
    slotA->setTitle ("A/B compare: A");
    slotB->setTitle ("A/B compare: B");
    scale->setTitle ("UI scale");

    pin->setComponentID ("pin");
    close->setComponentID ("close");
    bypass->setComponentID ("bypass");
    presetName->setComponentID ("preset");
    slotA->setComponentID ("slotA");
    slotB->setComponentID ("slotB");
    copyAToB->setComponentID ("copyAToB");
    retry->setComponentID ("retry");
    runInProcess->setComponentID ("runInProcess");
    scale->setComponentID ("uiScale");
    grip->setComponentID ("resizeGrip");

    pin->onClick = [this]
    {
        setPinned (! pinned);

        if (onPinChanged)
            onPinChanged (pinned);
    };
    close->onClick = [this] { if (onCloseRequested) onCloseRequested(); };
    bypass->onClick = [this] { commands.invoke (cmd::pluginSetBypassed, { plugin.trackId, plugin.id, plugin.enabled }); };
    previousPreset->onClick = [this] { stepPreset (-1); };
    nextPreset->onClick = [this] { stepPreset (1); };
    presetName->onClick = [this] { showPresetMenu(); };
    savePreset->onClick = [this] { askPresetName(); };
    slotA->onClick = [this] { commands.invoke (cmd::pluginSelectAB, { plugin.id, 0 }); };
    slotB->onClick = [this] { commands.invoke (cmd::pluginSelectAB, { plugin.id, 1 }); };
    copyAToB->onClick = [this] { commands.invoke (cmd::pluginCopyAToB, { plugin.trackId, plugin.id }); };
    undo->onClick = [this] { commands.invoke (cmd::editUndo); };
    redo->onClick = [this] { commands.invoke (cmd::editRedo); };
    retry->onClick = [this] { retryLoading(); };
    runInProcess->onClick = [this] { if (onRunInProcess) onRunInProcess(); };
    scale->onChange = [this] (int index)
    {
        setUiScale (index == 2 ? 200 : index == 1 ? 150 : 100);

        if (onUiScaleChanged)
            onUiScaleChanged (uiScale);
    };

    for (juce::Component* child : { (juce::Component*) pin.get(), (juce::Component*) close.get(), (juce::Component*) bypass.get(),
                                    (juce::Component*) previousPreset.get(), (juce::Component*) presetName.get(),
                                    (juce::Component*) nextPreset.get(), (juce::Component*) savePreset.get(),
                                    (juce::Component*) slotA.get(), (juce::Component*) slotB.get(), (juce::Component*) copyAToB.get(),
                                    (juce::Component*) undo.get(), (juce::Component*) redo.get(), (juce::Component*) stats.get(),
                                    (juce::Component*) sandbox.get(), (juce::Component*) footerInfo.get(), (juce::Component*) scale.get() })
        addAndMakeVisible (child);

    addChildComponent (*retry);
    addChildComponent (*runInProcess);
    addChildComponent (*grip);
    addMouseListener (clickWatch.get(), true);

    // The frame's shadow (L3) is drawn in this margin.
    for (auto& shadow : tm.getTheme().elevation3)
        margin = { juce::jmax (margin.getTop(), shadow.radius - shadow.offset.y),
                   juce::jmax (margin.getLeft(), shadow.radius - shadow.offset.x),
                   juce::jmax (margin.getBottom(), shadow.radius + shadow.offset.y),
                   juce::jmax (margin.getRight(), shadow.radius + shadow.offset.x) };

    themeManager.addListener (this);
    setState (info, track);
    retryLoading();
}

PluginWindow::~PluginWindow()
{
    themeManager.removeListener (this);
    removeMouseListener (clickWatch.get());

    if (vendor != nullptr)
        vendor->removeComponentListener (this);
}

void PluginWindow::setState (const PluginInfo& info, const juce::String& track)
{
    plugin = info;
    trackName = track;
    setName (plugin.name);
    setTitle (plugin.name + " plug-in window");
    setDescription (trackName + ", " + vendorOf (plugin) + " " + middleDot + " " + formatBadge (plugin) + " " + plugin.version);
    bypass->setToggleState (plugin.enabled, juce::dontSendNotification);
    bypass->setTitle (plugin.enabled ? "Bypass (plug-in on)" : "Bypass (plug-in bypassed)");
    slotA->setToggleState (plugin.abSlot == 0, juce::dontSendNotification);
    slotB->setToggleState (plugin.abSlot == 1, juce::dontSendNotification);
    updateTexts();
    repaint();
}

void PluginWindow::updateTexts()
{
    const auto presets = rack.getPresetNames (plugin.id);
    presetName->setButtonText (plugin.presetName.isNotEmpty() ? plugin.presetName
                                                              : presets.isEmpty() ? juce::String ("No presets") : juce::String ("Presets"));
    presetName->setTitle ("Preset: " + presetName->getButtonText());

    for (auto* b : { previousPreset.get(), nextPreset.get() })
        b->setEnabled (! presets.isEmpty());

    stats->setText (juce::String (plugin.latencySamples) + " smp " + middleDot + " " + cpuText);
    sandbox->setText (plugin.sandboxed ? "Sandboxed: out-of-process" : "Not sandboxed: in-process", plugin.sandboxed);
    footerInfo->setText ("Plug-in UI " + middleDot + " rendered by " + vendorOf (plugin) + " " + middleDot + " "
                         + (formatBadge (plugin) + " " + plugin.version).trim() + " " + middleDot + " " + processText (plugin));
}

void PluginWindow::setPinned (bool shouldPin)
{
    pinned = shouldPin;
    pin->setToggleState (pinned, juce::dontSendNotification);
    pin->setTitle (pinned ? "Unpin" : "Pin (keep on top)");
    updateAlwaysOnTop();
}

void PluginWindow::updateAlwaysOnTop()
{
    // Pinned: on top always. Unpinned: floating while Resamper is in front, so the main window never buries it.
    setAlwaysOnTop (pinned || juce::Process::isForegroundProcess());
}

void PluginWindow::setUiScale (int percent)
{
    uiScale = percent >= 200 ? 200 : percent >= 150 ? 150 : 100;
    scale->setSelectedIndex (uiScale == 200 ? 2 : uiScale == 150 ? 1 : 0, juce::dontSendNotification);
    updateSize();
}

bool PluginWindow::hasResizeGrip() const
{
    auto* editor = processorEditorIn (vendor.get());
    return status == Status::ready && editor != nullptr && editor->isResizable();
}

void PluginWindow::retryLoading()
{
    if (vendor != nullptr)
    {
        vendor->removeComponentListener (this);
        vendor.reset();
    }

    status = Status::loading;
    loadStartedAt = juce::Time::getMillisecondCounter();
    retry->setVisible (false);
    runInProcess->setVisible (false);
    updateSize();
    repaint();

    // The chrome and the loading state show first; the vendor UI follows when ready (§19).
    startTimer (loadingPollMs);
}

void PluginWindow::loadVendor()
{
    vendor = rack.createEditor (plugin.id);

    if (vendor == nullptr)
        vendor = std::make_unique<NoEditor> (themeManager);

    status = Status::ready;
    addAndMakeVisible (*vendor);
    vendor->addComponentListener (this);
    updateSize();
    startTimer (statusPollMs);
}

juce::Point<int> PluginWindow::vendorSize() const
{
    const auto s = (float) uiScale / 100.0f;

    if (status == Status::ready && vendor != nullptr)
        return { juce::roundToInt ((float) vendor->getWidth() * s), juce::roundToInt ((float) vendor->getHeight() * s) };

    return { minFrameWidth, loadingHeight };
}

void PluginWindow::updateSize()
{
    auto& metrics = themeManager.getMetrics();
    const auto content = vendorSize();
    const auto width = juce::jmax (minFrameWidth, content.x);
    const auto height = metrics.pluginTitleBarHeight + metrics.pluginToolbarHeight + content.y + metrics.pluginFooterHeight;
    const auto topLeft = getFrameScreenBounds().getPosition();
    const auto wasOnDesktop = isOnDesktop();
    setSize (width + margin.getLeftAndRight(), height + margin.getTopAndBottom());

    // Growing or shrinking keeps the frame's top-left where it was.
    if (wasOnDesktop)
        setFramePosition (topLeft);
}

juce::Rectangle<int> PluginWindow::frame() const
{
    return margin.subtractedFrom (getLocalBounds());
}

juce::Rectangle<int> PluginWindow::titleBar() const
{
    return frame().removeFromTop (themeManager.getMetrics().pluginTitleBarHeight);
}

juce::Rectangle<int> PluginWindow::toolbar() const
{
    return frame().withTrimmedTop (titleBar().getHeight()).removeFromTop (themeManager.getMetrics().pluginToolbarHeight);
}

juce::Rectangle<int> PluginWindow::footer() const
{
    return frame().removeFromBottom (themeManager.getMetrics().pluginFooterHeight);
}

juce::Rectangle<int> PluginWindow::vendorArea() const
{
    return frame().withTrimmedTop (titleBar().getHeight() + toolbar().getHeight()).withTrimmedBottom (footer().getHeight());
}

juce::Rectangle<int> PluginWindow::getFrameScreenBounds() const
{
    return margin.subtractedFrom (getScreenBounds());
}

void PluginWindow::setFramePosition (juce::Point<int> topLeft)
{
    setTopLeftPosition (topLeft - juce::Point<int> (margin.getLeft(), margin.getTop()));
}

bool PluginWindow::hasFocusInside() const
{
    auto* focused = juce::Component::getCurrentlyFocusedComponent();
    return focused != nullptr && (focused == this || isParentOf (focused));
}

void PluginWindow::focusHost()
{
    close->grabKeyboardFocus();
}

void PluginWindow::resized()
{
    auto title = titleBar().withTrimmedLeft (titlePaddingLeft).withTrimmedRight (titlePaddingRight);
    close->setBounds (title.removeFromRight (iconTarget).withSizeKeepingCentre (iconTarget, iconTarget));
    title.removeFromRight (2);
    pin->setBounds (title.removeFromRight (iconTarget).withSizeKeepingCentre (iconTarget, iconTarget));

    layoutToolbar (toolbar().reduced (toolbarPadding, 0));
    layoutFooter (footer().withTrimmedLeft (footerPaddingLeft).withTrimmedRight (footerPaddingRight));

    const auto area = vendorArea();

    if (vendor != nullptr)
    {
        // Native size times the UI scale, centred in the vendor area; never restyled.
        const auto s = (float) uiScale / 100.0f;
        const auto size = vendorSize();
        const auto x = area.getX() + (area.getWidth() - size.x) / 2;
        vendor->setTransform (uiScale == 100 ? juce::AffineTransform() : juce::AffineTransform::scale (s));
        vendor->setTopLeftPosition (juce::roundToInt ((float) x / s), juce::roundToInt ((float) area.getY() / s));
    }

    auto buttons = area.withSizeKeepingCentre (juce::jmin (area.getWidth(), 220), controlHeight).translated (0, 34);
    retry->setBounds (buttons.removeFromLeft (buttons.getWidth() / 2 - 4));
    runInProcess->setBounds (buttons.withTrimmedLeft (8));
}

void PluginWindow::layoutToolbar (juce::Rectangle<int> bar)
{
    auto take = [&bar] (int width, int height = controlHeight)
    {
        auto r = bar.removeFromLeft (width).withSizeKeepingCentre (width, height);
        bar.removeFromLeft (gap);
        return r;
    };

    bypass->setBounds (take (bypassWidth));

    // The preset menu is one group: prev, name, next, save, 1 px apart.
    for (auto* part : { previousPreset.get(), presetName.get(), nextPreset.get() })
    {
        const auto width = part == presetName.get() ? presetNameWidth : presetStepWidth;
        part->setBounds (bar.removeFromLeft (width).withSizeKeepingCentre (width, controlHeight));
        bar.removeFromLeft (1);
    }

    savePreset->setBounds (take (presetStepWidth));

    auto slots = take (2 * slotWidth + 6, slotHeight + 4).reduced (2);
    slotA->setBounds (slots.removeFromLeft (slotWidth));
    slotB->setBounds (slots.removeFromRight (slotWidth));
    copyAToB->setBounds (take (copyWidth));
    undo->setBounds (take (presetStepWidth));
    redo->setBounds (take (presetStepWidth));

    sandbox->setBounds (bar.removeFromRight (sandboxWidth));
    bar.removeFromRight (gap);
    stats->setBounds (bar.removeFromRight (juce::jmin (statsWidth, bar.getWidth())));
}

void PluginWindow::layoutFooter (juce::Rectangle<int> bar)
{
    grip->setVisible (hasResizeGrip());

    if (grip->isVisible())
    {
        grip->setBounds (bar.removeFromRight (gripSize).withSizeKeepingCentre (gripSize, gripSize));
        bar.removeFromRight (8);
    }

    scale->setBounds (bar.removeFromRight (scaleWidth).withSizeKeepingCentre (scaleWidth, scaleHeight));
    bar.removeFromRight (8);
    footerInfo->setBounds (bar);
}

void PluginWindow::paint (juce::Graphics& g)
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

        // Title bar and toolbar on bg-panel with bottom borders; the footer with a top one.
        for (auto bar : { titleBar(), toolbar() })
        {
            g.setColour (theme.bgPanel);
            g.fillRect (bar);
            g.setColour (theme.border);
            g.fillRect (bar.removeFromBottom (1));
        }

        g.setColour (theme.bgSlot);
        g.fillRect (vendorArea());

        auto foot = footer();
        g.setColour (theme.bgPanel);
        g.fillRect (foot);
        g.setColour (theme.border);
        g.fillRect (foot.removeFromTop (1));

        // The A/B well.
        g.setColour (theme.bgSlot);
        g.fillRoundedRectangle (slotA->getBounds().getUnion (slotB->getBounds()).expanded (2).toFloat(), controlRadius);
    }

    g.setColour (theme.border);
    g.drawRoundedRectangle (f.reduced (0.5f), radius, 1.0f);

    // Title: plug (accent), track › name, vendor, format badge; what's left is the drag area.
    auto title = titleBar().withTrimmedLeft (titlePaddingLeft).withRight (pin->getX() - gap);
    drawIcon (g, Icon::plug, title.removeFromLeft (12).toFloat().withSizeKeepingCentre (12.0f, 12.0f), theme.accent);
    title.removeFromLeft (gap);

    auto text = [&] (const juce::String& s, const TypeStyle& style, juce::Colour colour)
    {
        const auto w = juce::jmin (title.getWidth(), juce::GlyphArrangement::getStringWidthInt (themeManager.font (style), s) + 1);
        drawStyledText (g, themeManager, s, style, title.removeFromLeft (w), juce::Justification::centredLeft, colour);
        title.removeFromLeft (gap);
    };

    text (trackName, trackStyle, theme.textDim);
    drawIcon (g, Icon::chevronRight, title.removeFromLeft (9).toFloat().withSizeKeepingCentre (9.0f, 9.0f), theme.textDim);
    title.removeFromLeft (gap);
    text (plugin.name, titleStyle, theme.textPrimary);
    text (vendorOf (plugin), vendorStyle, theme.textDim);

    const auto badgeText = formatBadge (plugin);
    const auto badgeWidth = juce::GlyphArrangement::getStringWidthInt (themeManager.font (badgeStyle), badgeText) + 2 * 5 + 2;

    if (badgeWidth <= title.getWidth())
    {
        const auto badge = title.removeFromLeft (badgeWidth).withSizeKeepingCentre (badgeWidth, 13);
        g.setColour (theme.border);
        g.drawRoundedRectangle (badge.toFloat().reduced (0.5f), 3.0f, 1.0f);
        drawNumber (g, themeManager, badgeText, badgeStyle, badge, juce::Justification::centred, theme.textSecondary);
    }

    // The vendor area while there is no vendor UI: loading (spinner + name) or failed.
    if (status != Status::ready)
    {
        auto area = vendorArea().withSizeKeepingCentre (vendorArea().getWidth(), 60).translated (0, -10);

        if (status == Status::loading)
        {
            const auto spinner = area.removeFromTop (22).toFloat().withSizeKeepingCentre (18.0f, 18.0f);
            const auto angle = (float) (juce::Time::getMillisecondCounter() % 1000) / 1000.0f * juce::MathConstants<float>::twoPi;
            juce::Path arc;
            arc.addCentredArc (spinner.getCentreX(), spinner.getCentreY(), 8.0f, 8.0f, angle, 0.0f,
                               juce::MathConstants<float>::pi * 1.5f, true);
            g.setColour (theme.accent);
            g.strokePath (arc, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            area.removeFromTop (8);
            drawStyledText (g, themeManager, "Loading " + plugin.name + juce::String (juce::CharPointer_UTF8 ("\xe2\x80\xa6")),
                            stateStyle, area.removeFromTop (16), juce::Justification::centred, theme.textSecondary);
        }
        else
        {
            const auto error = rack.getLoadError (plugin.id);
            drawStyledText (g, themeManager, plugin.name + " didn't load", stateStyle, area.removeFromTop (16),
                            juce::Justification::centred, theme.rec);
            drawStyledText (g, themeManager, error.isNotEmpty() ? error : "It took longer than " + juce::String (loadTimeoutMs / 1000.0, 1) + " s to start.",
                            stateDetailStyle, area.removeFromTop (16), juce::Justification::centred, theme.textDim);
        }
    }
}

bool PluginWindow::hitTest (int x, int y)
{
    // Clicks on the shadow fall through to whatever is behind.
    return frame().contains (x, y);
}

bool PluginWindow::keyPressed (const juce::KeyPress& key)
{
    const auto mods = key.getModifiers();

    if (key == juce::KeyPress::escapeKey)
    {
        // Esc in the vendor UI hands focus back to the host (§18); on the host chrome it closes.
        if (auto* focused = juce::Component::getCurrentlyFocusedComponent();
            vendor != nullptr && focused != nullptr && (focused == vendor.get() || vendor->isParentOf (focused)))
            focusHost();
        else if (onCloseRequested)
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

void PluginWindow::mouseDown (const juce::MouseEvent& e)
{
    // The title bar left of Pin is the drag area.
    dragging = titleBar().withRight (pin->getX()).contains (e.getPosition());

    if (dragging)
        dragger.startDraggingComponent (this, e);
}

void PluginWindow::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging)
        dragger.dragComponent (this, e, nullptr);
}

void PluginWindow::mouseUp (const juce::MouseEvent& e)
{
    if (std::exchange (dragging, false) && e.mouseWasDraggedSinceMouseDown() && onMoved)
        onMoved();
}

std::unique_ptr<juce::AccessibilityHandler> PluginWindow::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler> (*this, juce::AccessibilityRole::window);
}

void PluginWindow::stepPreset (int delta)
{
    const auto presets = rack.getPresetNames (plugin.id);

    if (presets.isEmpty())
        return;

    const auto current = presets.indexOf (plugin.presetName);
    const auto next = current < 0 ? (delta > 0 ? 0 : presets.size() - 1)
                                  : juce::negativeAwareModulo (current + delta, presets.size());
    commands.invoke (cmd::pluginSelectPreset, { plugin.id, next });
}

void PluginWindow::showPresetMenu()
{
    juce::PopupMenu menu;
    const auto presets = rack.getPresetNames (plugin.id);

    for (int i = 0; i < presets.size(); ++i)
        menu.addItem (presets[i], true, presets[i] == plugin.presetName,
                      [this, i] { commands.invoke (cmd::pluginSelectPreset, { plugin.id, i }); });

    if (presets.isEmpty())
        menu.addItem ("No presets yet: Save stores one", false, false, nullptr);

    menu.addSeparator();
    menu.addItem ("Save Preset" + juce::String (juce::CharPointer_UTF8 ("\xe2\x80\xa6")), [this] { askPresetName(); });
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (presetName.get()));
}

void PluginWindow::askPresetName()
{
    auto* dialog = new juce::AlertWindow ("Save Preset", "Save the current settings of " + plugin.name + " as:",
                                          juce::MessageBoxIconType::NoIcon, this);
    dialog->addTextEditor ("name", plugin.presetName.isNotEmpty() ? plugin.presetName : juce::String ("My Preset"));
    dialog->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    dialog->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    dialog->enterModalState (true, juce::ModalCallbackFunction::create (
        [safe = juce::Component::SafePointer<PluginWindow> (this), dialog] (int result)
        {
            if (result == 1 && safe != nullptr)
                safe->commands.invoke (cmd::pluginSavePreset, { safe->plugin.id, dialog->getTextEditorContents ("name") });
        }), true);
}

void PluginWindow::timerCallback()
{
    if (status == Status::loading)
    {
        if (! rack.isLoading (plugin.id))
        {
            loadVendor();
            return;
        }

        if (juce::Time::getMillisecondCounter() - loadStartedAt > (juce::uint32) loadTimeoutMs)
        {
            status = Status::failed;
            retry->setVisible (true);
            runInProcess->setVisible (true);
            startTimer (statusPollMs);
        }

        repaint (vendorArea());
    }

    updateAlwaysOnTop();

    auto text = juce::String (rack.getCpuLoad (plugin.id) * 100.0, 1) + "%";

    if (text != cpuText)
    {
        cpuText = text;
        updateTexts();
    }
}

void PluginWindow::componentMovedOrResized (juce::Component& c, bool, bool wasResized)
{
    // The plug-in resized its own UI: the window follows.
    if (&c == vendor.get() && wasResized)
        updateSize();
}

void PluginWindow::themeChanged()
{
    pin->onColour = themeManager.getTheme().accent;
    repaint();
}

} // namespace resamper
