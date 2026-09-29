#include "CommandRegistry.h"

namespace resamper
{

namespace
{
    class FunctionCommand : public Command
    {
    public:
        FunctionCommand (CommandInfo i, std::function<void (const juce::var&)> fn)
            : Command (std::move (i.id), std::move (i.name)),
              body (std::move (fn)), enabled (std::move (i.isEnabled)), ticked (std::move (i.isTicked)) {}

        void execute (const juce::var& args) override   { body (args); }
        bool isEnabled() const override                 { return enabled == nullptr || enabled(); }
        bool isTicked() const override                  { return ticked != nullptr && ticked(); }

    private:
        std::function<void (const juce::var&)> body;
        std::function<bool()> enabled, ticked;
    };
}

void CommandRegistry::add (std::unique_ptr<Command> command)
{
    jassert (command != nullptr);
    auto id = command->getId();
    [[maybe_unused]] auto [it, inserted] = commands.emplace (id, std::move (command));
    jassert (inserted);   // two Commands registered under one ID
}

void CommandRegistry::add (CommandInfo info, std::function<void (const juce::var& args)> execute)
{
    jassert (execute != nullptr);
    add (std::make_unique<FunctionCommand> (std::move (info), std::move (execute)));
}

void CommandRegistry::add (CommandInfo info, std::function<void()> execute)
{
    jassert (execute != nullptr);
    add (std::move (info), [fn = std::move (execute)] (const juce::var&) { fn(); });
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

} // namespace resamper
