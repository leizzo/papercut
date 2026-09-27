#include "CommandRegistry.h"

namespace papercut
{

void CommandRegistry::add (std::unique_ptr<Command> command)
{
    jassert (command != nullptr);
    auto id = command->getId();
    [[maybe_unused]] auto [it, inserted] = commands.emplace (id, std::move (command));
    jassert (inserted);   // two Commands registered under one ID
}

bool CommandRegistry::invoke (const juce::String& commandId, const juce::var& args)
{
    if (auto it = commands.find (commandId); it != commands.end())
    {
        it->second->execute (args);
        return true;
    }

    DBG ("Unknown Command: " << commandId);
    jassertfalse;
    return false;
}

const Command* CommandRegistry::find (const juce::String& commandId) const
{
    auto it = commands.find (commandId);
    return it != commands.end() ? it->second.get() : nullptr;
}

} // namespace papercut
