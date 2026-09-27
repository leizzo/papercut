#include "ShaperPanel.h"
#include "Commands/AutomationCommands.h"

#include <algorithm>

namespace papercut
{

ShaperPanel::ShaperPanel (ApplicationModel& m, Shaper& s, CommandRegistry& c, ThemeManager& tm)
    : model (m), shapers (s), commands (c), themeManager (tm)
{
    setOpaque (true);

    loopButton.setClickingTogglesState (true);
    audioButton.setClickingTogglesState (true);
    loopButton.setRadioGroupId (1);
    audioButton.setRadioGroupId (1);
    loopButton.setToggleState (true, juce::dontSendNotification);

    const auto textBoxWidth = juce::jmax (themeManager.getMetrics().trackButtonWidth * 2, themeManager.getMetrics().textPadding * 8);
    const auto textBoxHeight = themeManager.getMetrics().trackControlHeight;
    auto style = [&] (juce::Slider& slider, double min, double max, double value) {
        slider.setSliderStyle (juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, textBoxWidth, textBoxHeight);
        slider.setRange (min, max, 0.0);
        slider.setValue (value, juce::dontSendNotification);
    };

    style (depth, 0.0, 1.0, 1.0);
    style (length, 0.25, 64.0, 1.0);
    style (attack, 0.0, 5.0, 0.01);
    style (hold, 0.0, 5.0, 0.0);
    style (release, 0.0, 5.0, 0.1);
    style (threshold, -60.0, 0.0, -20.0);

    for (auto* child : std::initializer_list<juce::Component*> {
             &shaperList, &addButton, &loopButton, &audioButton,
             &depthLabel, &depth, &lengthLabel, &length,
             &attackLabel, &attack, &holdLabel, &hold,
             &releaseLabel, &release, &thresholdLabel, &threshold })
        addAndMakeVisible (child);

    auto hook = [this] (juce::Slider& slider) {
        slider.onDragEnd = [this] { commit(); };
        slider.onValueChange = [this, &slider] {
            if (! updating && ! slider.isMouseButtonDown())
                commit();
        };
    };

    for (auto* slider : { &depth, &length, &attack, &hold, &release, &threshold })
        hook (*slider);

    shaperList.onChange = [this] {
        if (updating)
            return;

        const auto row = shaperList.getSelectedItemIndex();
        auto ids = idsOnTrack();

        if (juce::isPositiveAndBelow (row, ids.size()))
            shaperId = ids[row];

        if (auto info = selected())
            loadControls (&*info);

        resized();
    };

    loopButton.onClick = [this] {
        if (updating || shaperId.isEmpty())
            return;

        if (auto info = selected(); info && info->mode != ShaperMode::loop)
        {
            resized();
            commit();
        }
    };

    audioButton.onClick = [this] {
        if (updating || shaperId.isEmpty())
            return;

        if (auto info = selected(); info && info->mode != ShaperMode::audioTrigger)
        {
            resized();
            commit();
        }
    };

    addButton.onClick = [this] {
        if (trackId.isEmpty())
            return;

        const auto previous = idsOnTrack();
        const auto mode = audioButton.getToggleState() ? ShaperMode::audioTrigger : ShaperMode::loop;
        commands.invoke ("shaper.add", shaperAddArgs (trackId, parameterKey.isEmpty() ? juce::String ("volume") : parameterKey, mode));

        for (auto& info : shapers.getShapers (trackId))
            if (! previous.contains (info.id))
                shaperId = info.id;

        refresh();
    };

    applyTheme();
    model.addListener (this);
    themeManager.addListener (this);
    refresh();
}

ShaperPanel::~ShaperPanel()
{
    themeManager.removeListener (this);
    model.removeListener (this);
}

void ShaperPanel::setTrack (const juce::String& newTrackId, const juce::String& newParameterKey)
{
    trackId = newTrackId;
    parameterKey = newParameterKey;
    shaperId.clear();
    refresh();
}

void ShaperPanel::setParameterKey (const juce::String& newParameterKey)
{
    parameterKey = newParameterKey;
}

int ShaperPanel::getPreferredHeight() const
{
    auto& metrics = themeManager.getMetrics();
    const int rows = 6;
    return juce::jmax (metrics.trackHeight,
                       metrics.timelineHeight * 2 + rows * (metrics.trackControlHeight + metrics.inset));
}

void ShaperPanel::paint (juce::Graphics& g)
{
    g.fillAll (themeManager.getTheme().panel);
}

void ShaperPanel::resized()
{
    auto& metrics = themeManager.getMetrics();
    auto area = getLocalBounds().reduced (metrics.inset);

    auto selectRow = area.removeFromTop (metrics.timelineHeight);
    addButton.setBounds (selectRow.removeFromRight (metrics.trackButtonWidth * 2).reduced (metrics.inset, 0));
    shaperList.setBounds (selectRow);

    area.removeFromTop (metrics.inset);
    auto modeRow = area.removeFromTop (metrics.timelineHeight);
    loopButton.setBounds (modeRow.removeFromLeft (modeRow.getWidth() / 2).reduced (metrics.inset, 0));
    audioButton.setBounds (modeRow.reduced (metrics.inset, 0));

    const bool loop = loopButton.getToggleState();
    length.setVisible (loop);
    lengthLabel.setVisible (loop);
    for (auto* control : std::initializer_list<juce::Component*> { &attack, &attackLabel, &hold, &holdLabel,
                                                                   &release, &releaseLabel, &threshold, &thresholdLabel })
        control->setVisible (! loop);

    auto place = [&] (juce::Label& label, juce::Slider& slider) {
        if (! slider.isVisible())
            return;

        area.removeFromTop (metrics.inset);
        auto row = area.removeFromTop (metrics.trackControlHeight);
        label.setBounds (row.removeFromLeft (metrics.trackButtonWidth * 4));
        slider.setBounds (row);
    };

    place (depthLabel, depth);
    place (lengthLabel, length);
    place (attackLabel, attack);
    place (holdLabel, hold);
    place (releaseLabel, release);
    place (thresholdLabel, threshold);
}

void ShaperPanel::refresh()
{
    const juce::ScopedValueSetter<bool> guard (updating, true);
    auto all = trackId.isEmpty() ? std::vector<ShaperInfo>() : shapers.getShapers (trackId);

    shaperList.clear (juce::dontSendNotification);

    bool found = false;

    for (int i = 0; i < (int) all.size(); ++i)
    {
        const auto& info = all[(size_t) i];
        const auto label = info.parameterKey + (info.mode == ShaperMode::loop ? "  loop" : "  audio");
        shaperList.addItem (label, i + 1);

        if (info.id == shaperId)
        {
            shaperList.setSelectedId (i + 1, juce::dontSendNotification);
            found = true;
        }
    }

    if (! found)
        shaperId = all.empty() ? juce::String() : all.front().id;

    if (! found && ! all.empty())
        shaperList.setSelectedId (1, juce::dontSendNotification);

    if (shaperList.getNumItems() == 0)
        shaperList.setTextWhenNothingSelected ("No shaper");

    const ShaperInfo* current = nullptr;

    for (auto& info : all)
        if (info.id == shaperId)
            current = &info;

    loadControls (current);
    resized();
}

void ShaperPanel::loadControls (const ShaperInfo* info)
{
    const juce::ScopedValueSetter<bool> guard (updating, true);
    const bool enabled = info != nullptr;

    for (auto* slider : { &depth, &length, &attack, &hold, &release, &threshold })
        slider->setEnabled (enabled);

    loopButton.setEnabled (enabled);
    audioButton.setEnabled (enabled);

    if (info == nullptr)
        return;

    const bool loop = info->mode == ShaperMode::loop;
    loopButton.setToggleState (loop, juce::dontSendNotification);
    audioButton.setToggleState (! loop, juce::dontSendNotification);
    depth.setValue (info->depth, juce::dontSendNotification);
    length.setValue (info->lengthBeats, juce::dontSendNotification);
    attack.setValue (info->attackSeconds, juce::dontSendNotification);
    hold.setValue (info->holdSeconds, juce::dontSendNotification);
    release.setValue (info->releaseSeconds, juce::dontSendNotification);
    threshold.setValue (info->thresholdDb, juce::dontSendNotification);
}

void ShaperPanel::commit()
{
    if (updating || shaperId.isEmpty())
        return;

    const auto previous = idsOnTrack();
    auto info = selected();
    const bool loop = loopButton.getToggleState();

    if (loop)
    {
        auto shape = info ? info->shape : std::vector<ShaperShapePoint> {};

        if (shape.empty())
            shape = { { 0.0f, 0.0f }, { 1.0f, 1.0f } };

        commands.invoke ("shaper.setLoop", shaperSetLoopArgs (shaperId, length.getValue(), (float) depth.getValue(), shape));
    }
    else
    {
        commands.invoke ("shaper.setAudioTrigger",
                         shaperSetAudioTriggerArgs (shaperId, (float) attack.getValue(), (float) hold.getValue(),
                                                    (float) release.getValue(), (float) threshold.getValue(),
                                                    (float) depth.getValue()));
    }

    auto now = shapers.getShapers (trackId);

    if (std::none_of (now.begin(), now.end(), [&] (const ShaperInfo& item) { return item.id == shaperId; }))
        for (auto& item : now)
            if (! previous.contains (item.id))
                shaperId = item.id;

    refresh();
}

void ShaperPanel::applyTheme()
{
    auto& theme = themeManager.getTheme();

    for (auto* button : { &addButton, &loopButton, &audioButton })
    {
        button->setColour (juce::TextButton::buttonColourId, theme.trackHeader);
        button->setColour (juce::TextButton::buttonOnColourId, theme.accent);
        button->setColour (juce::TextButton::textColourOffId, theme.text);
        button->setColour (juce::TextButton::textColourOnId, theme.background);
    }

    for (auto* slider : { &depth, &length, &attack, &hold, &release, &threshold })
    {
        slider->setColour (juce::Slider::thumbColourId, theme.accent);
        slider->setColour (juce::Slider::trackColourId, theme.laneB);
        slider->setColour (juce::Slider::textBoxTextColourId, theme.text);
        slider->setColour (juce::Slider::textBoxBackgroundColourId, theme.background);
        slider->setColour (juce::Slider::textBoxOutlineColourId, theme.gridLine);
    }

    for (auto* label : { &depthLabel, &lengthLabel, &attackLabel, &holdLabel, &releaseLabel, &thresholdLabel })
    {
        label->setColour (juce::Label::textColourId, theme.text);
        label->setFont (themeManager.getFont (0.85f));
    }

    shaperList.setColour (juce::ComboBox::backgroundColourId, theme.background);
    shaperList.setColour (juce::ComboBox::textColourId, theme.text);
    shaperList.setColour (juce::ComboBox::outlineColourId, theme.gridLine);
    repaint();
}

std::optional<ShaperInfo> ShaperPanel::selected() const
{
    if (trackId.isEmpty() || shaperId.isEmpty())
        return {};

    for (auto& info : shapers.getShapers (trackId))
        if (info.id == shaperId)
            return info;

    return {};
}

juce::StringArray ShaperPanel::idsOnTrack() const
{
    juce::StringArray ids;

    if (trackId.isNotEmpty())
        for (auto& info : shapers.getShapers (trackId))
            ids.add (info.id);

    return ids;
}

} // namespace papercut
