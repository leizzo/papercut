#pragma once

#include <juce_core/juce_core.h>
#include <map>
#include <memory>

namespace papercut
{

/** A named, user-triggerable operation keyed by a string ID (e.g. "transport.play").

    The only path by which UI mutates the model (ADR-0003). Commands never
    implement undo: model mutations are recorded by Engine Undo.
*/
class Command
{
public:
    Command (juce::String commandId, juce::String displayName)
        : id (std::move (commandId)), name (std::move (displayName)) {}

    virtual ~Command() = default;

    const juce::String& getId() const noexcept     { return id; }
    const juce::String& getName() const noexcept   { return name; }

    /** args carries what a gesture decided (e.g. which clip, where to); Commands
        invoked from menus, shortcuts and JSON buttons receive a void var. */
    virtual void execute (const juce::var& args) = 0;

    /** Whether invoking now would do anything (menus grey out disabled Commands). */
    virtual bool isEnabled() const   { return true; }

private:
    const juce::String id, name;

    JUCE_DECLARE_NON_COPYABLE (Command)
};

/** The single registry behind JSON buttons, menus and keyboard shortcuts (ADR-0006).
    If an action isn't registered here, no UI surface can reach it. */
class CommandRegistry
{
public:
    /** Registers a Command. IDs must be unique. */
    void add (std::unique_ptr<Command>);

    /** Executes the Command with this ID. Returns false (and asserts) for an unknown ID. */
    bool invoke (const juce::String& commandId, const juce::var& args = {});

    const Command* find (const juce::String& commandId) const;
    bool contains (const juce::String& commandId) const    { return find (commandId) != nullptr; }

private:
    std::map<juce::String, std::unique_ptr<Command>> commands;
};

} // namespace papercut
