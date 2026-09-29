#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <map>
#include <stdexcept>

namespace resamper
{

/** A malformed layout: unknown type, missing field, bad JSON. Thrown so a
    broken layout fails loudly instead of silently rendering nothing. */
struct LayoutError : std::runtime_error
{
    explicit LayoutError (const juce::String& message) : std::runtime_error (message.toStdString()) {}
};

/** Creates components from JSON layout nodes by their "type" name.

    Every node is an object with a "type" and an optional "id" (which becomes
    the component ID). Creators read their own fields and may recurse through
    the factory for children.
*/
class ComponentFactory
{
public:
    using Creator = std::function<std::unique_ptr<juce::Component> (const juce::var& node, ComponentFactory&)>;

    void registerType (const juce::String& type, Creator);

    /** Builds a component tree from a node. Throws LayoutError. */
    std::unique_ptr<juce::Component> create (const juce::var& node);

    /** Parses JSON text and builds its root node. Throws LayoutError. */
    std::unique_ptr<juce::Component> createFromJson (const juce::String& json);

private:
    std::map<juce::String, Creator> creators;
};

/** Reads a required string field, throwing LayoutError if it's missing. */
juce::String requireString (const juce::var& node, const char* field);

} // namespace resamper
