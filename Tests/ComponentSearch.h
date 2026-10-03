#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>

namespace resamper::test
{

/** Every descendant of root with this component ID, depth first. */
inline void findAll (juce::Component& root, const juce::String& id, std::vector<juce::Component*>& out)
{
    for (auto* child : root.getChildren())
    {
        if (child->getComponentID() == id)
            out.push_back (child);

        findAll (*child, id, out);
    }
}

inline std::vector<juce::Component*> findAll (juce::Component& root, const juce::String& id)
{
    std::vector<juce::Component*> out;
    findAll (root, id, out);
    return out;
}

/** The one descendant with this ID; nullptr if none or several. */
inline juce::Component* findOne (juce::Component& root, const juce::String& id)
{
    auto all = findAll (root, id);
    return all.size() == 1 ? all.front() : nullptr;
}

/** The first descendant of this type, depth first. */
template <typename Type>
Type* findType (juce::Component& root)
{
    for (auto* child : root.getChildren())
    {
        if (auto* match = dynamic_cast<Type*> (child))
            return match;

        if (auto* match = findType<Type> (*child))
            return match;
    }

    return nullptr;
}

/** The first top-level desktop window of this type. */
template <typename Type>
Type* findOnDesktop()
{
    auto& desktop = juce::Desktop::getInstance();

    for (int i = 0; i < desktop.getNumComponents(); ++i)
        if (auto* match = dynamic_cast<Type*> (desktop.getComponent (i)))
            return match;

    return nullptr;
}

/** findType, or a top-level desktop window of this type when it has left the tree. */
template <typename Type>
Type* findTypeOrOnDesktop (juce::Component& root)
{
    if (auto* found = findType<Type> (root))
        return found;

    return findOnDesktop<Type>();
}

/** The desktop windows with this component ID that are on screen. */
inline std::vector<juce::Component*> visibleDesktopWindows (const juce::String& id)
{
    std::vector<juce::Component*> found;
    auto& desktop = juce::Desktop::getInstance();

    for (int i = 0; i < desktop.getNumComponents(); ++i)
        if (auto* c = desktop.getComponent (i); c->getComponentID() == id && c->isVisible())
            found.push_back (c);

    return found;
}

/** The plug-in windows on screen. */
inline int visiblePluginWindows()
{
    return (int) visibleDesktopWindows ("PluginWindow").size();
}

/** Clicks a button and lets its (asynchronous) click land. */
inline void click (juce::Component* c)
{
    if (auto* button = dynamic_cast<juce::Button*> (c))
        button->triggerClick();

    juce::MessageManager::getInstance()->runDispatchLoopUntil (50);
}

} // namespace resamper::test
