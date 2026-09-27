#include "UIStateStore.h"

namespace papercut
{

namespace
{
    const juce::Identifier idProperty ("id");
    const juce::Identifier componentType ("Component");
}

juce::ValueTree UIStateStore::getState (const juce::String& componentId)
{
    auto existing = root.getChildWithProperty (idProperty, componentId);

    if (existing.isValid())
        return existing;

    juce::ValueTree state (componentType);
    state.setProperty (idProperty, componentId, nullptr);
    root.appendChild (state, nullptr);
    return state;
}

juce::var UIStateStore::toVar() const
{
    auto result = std::make_unique<juce::DynamicObject>();

    for (auto child : root)
    {
        auto props = std::make_unique<juce::DynamicObject>();

        for (int i = 0; i < child.getNumProperties(); ++i)
            if (auto name = child.getPropertyName (i); name != idProperty)
                props->setProperty (name, child[name]);

        result->setProperty (child[idProperty].toString(), props.release());
    }

    return result.release();
}

void UIStateStore::restore (const juce::var& snapshot)
{
    for (auto child : root)
    {
        auto saved = snapshot[juce::Identifier (child[idProperty].toString())];

        for (int i = child.getNumProperties(); --i >= 0;)
            if (auto name = child.getPropertyName (i); name != idProperty && ! saved.hasProperty (name))
                child.removeProperty (name, nullptr);
    }

    if (auto* obj = snapshot.getDynamicObject())
        for (auto& component : obj->getProperties())
            if (auto* props = component.value.getDynamicObject())
                for (auto& p : props->getProperties())
                    getState (component.name.toString()).setProperty (p.name, p.value, nullptr);
}

} // namespace papercut
