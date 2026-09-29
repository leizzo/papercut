#pragma once

#include "UI/Controls/Controls.h"

#include <deque>

namespace resamper
{

/** Toasts (PRD §16.7): short notes at the bottom centre for outcomes that
    aren't obvious (and for errors, instead of modal dialogs), each gone after
    4 s, optionally with an Undo action. The newest sits at the bottom; at
    most three show. Covers its parent but lets clicks through except on a toast. */
class Toasts : public juce::Component,
               private juce::Timer
{
public:
    static constexpr int lifetimeMs = 4000, maxShown = 3;

    explicit Toasts (ThemeManager&);

    /** undo: shows an Undo action that runs it and closes the toast. */
    void show (const juce::String& message, std::function<void()> undo = {}, bool isError = false);

    bool hitTest (int x, int y) override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    struct Toast
    {
        juce::String message;
        std::function<void()> undo;
        bool error = false;
        juce::uint32 shownAt = 0;
        juce::Rectangle<int> bounds, undoBounds;
    };

    ThemeManager& themeManager;
    std::deque<Toast> toasts;

    void layoutToasts();
    void timerCallback() override;
};

} // namespace resamper
