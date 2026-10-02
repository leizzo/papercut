#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <memory>

namespace resamper
{

/** Where a sandboxed plug-in's own UI shows (PluginSandbox): in a panel of
    its sandbox host, laid exactly over the vendor area of its plug-in window
    in Resamper and kept just above that window. The panel never makes the
    host the front app, so Resamper keeps its menu bar and its windows. */
namespace sandboxdock
{
    /** A desktop window, as another process can order its own windows by it. */
    struct WindowRef
    {
        juce::int64 number = 0;   ///< 0: none (not on the desktop)
        int level = 0;            ///< its window level (floating while Resamper is in front)

        bool operator== (const WindowRef&) const = default;
    };

    /** The desktop window a component is in. */
    WindowRef windowOf (juce::Component&);

    /** The sandbox host's panel holding the plug-in's own editor. */
    class Panel
    {
    public:
        /** content goes on the panel's desktop window; onClicked hears a mouse press inside it. */
        Panel (juce::Component& content, std::function<void()> onClicked);
        ~Panel();

        /** Shows the panel over screenArea (logical desktop coordinates), just above
            the given window and at its level; or hides it. */
        void place (juce::Rectangle<int> screenArea, bool visible, WindowRef above);

        /** Where the panel is on the desktop; empty while hidden. */
        juce::Rectangle<int> getScreenBounds() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl;

        JUCE_DECLARE_NON_COPYABLE (Panel)
    };
}

} // namespace resamper
