#include "ComponentFactory.h"

namespace resamper
{

namespace
{
    juce::String describe (const juce::var& node)
    {
        auto id = node["id"].toString();
        return id.isNotEmpty() ? " (id \"" + id + "\")" : juce::String();
    }
}

juce::String requireString (const juce::var& node, const char* field)
{
    auto value = node[field];

    if (! value.isString() || value.toString().isEmpty())
        throw LayoutError ("\"" + node["type"].toString() + "\"" + describe (node)
                           + " requires a \"" + field + "\" string");

    return value.toString();
}

void ComponentFactory::registerType (const juce::String& type, Creator creator)
{
    jassert (! creators.contains (type));
    creators[type] = std::move (creator);
}

std::unique_ptr<juce::Component> ComponentFactory::create (const juce::var& node)
{
    if (! node.isObject())
        throw LayoutError ("A layout node must be a JSON object");

    auto type = node["type"].toString();

    if (type.isEmpty())
        throw LayoutError ("Layout node" + describe (node) + " has no \"type\"");

    auto it = creators.find (type);

    if (it == creators.end())
        throw LayoutError ("Unknown component type \"" + type + "\"" + describe (node));

    auto component = it->second (node, *this);
    jassert (component != nullptr);
    component->setComponentID (node["id"].toString());
    return component;
}

std::unique_ptr<juce::Component> ComponentFactory::createFromJson (const juce::String& json)
{
    juce::var root;

    if (auto r = juce::JSON::parse (json, root); r.failed())
        throw LayoutError ("Invalid layout JSON: " + r.getErrorMessage());

    return create (root);
}

} // namespace resamper
