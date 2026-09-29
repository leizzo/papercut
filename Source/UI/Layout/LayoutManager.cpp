#include "LayoutManager.h"
#include "LayoutSource.h"
#include "UI/State/UIStateStore.h"

namespace resamper
{

namespace
{
    const juce::Identifier focusProperty ("focus");
    constexpr const char* windowStateId = "window";

    class LayoutErrorDisplay : public juce::Label
    {
    public:
        explicit LayoutErrorDisplay (const juce::String& message)
        {
            setText ("Layout error: " + message, juce::dontSendNotification);
            setJustificationType (juce::Justification::centred);
        }
    };

    /** Keeps the ID of the focused component in UI State as focus moves, so a
        rebuilt layout can hand focus back without capturing anything at reload. */
    struct FocusTracker : juce::FocusChangeListener
    {
        explicit FocusTracker (UIStateStore& s) : uiState (s)   { juce::Desktop::getInstance().addFocusChangeListener (this); }
        ~FocusTracker() override                                 { juce::Desktop::getInstance().removeFocusChangeListener (this); }

        void globalFocusChanged (juce::Component* focused) override
        {
            if (focused != nullptr && focused->getComponentID().isNotEmpty())
                uiState.getState (windowStateId).setProperty (focusProperty, focused->getComponentID(), nullptr);
        }

        UIStateStore& uiState;
    };
}

//==============================================================================
LayoutHost::LayoutHost (const juce::String& hostId, juce::String file)
    : layoutFile (std::move (file))
{
    setComponentID (hostId);
}

juce::Component* LayoutHost::findById (const juce::String& componentId) const
{
    if (content == nullptr)
        return nullptr;

    if (content->getComponentID() == componentId)
        return content.get();

    std::function<juce::Component* (juce::Component&)> search = [&] (juce::Component& c) -> juce::Component*
    {
        for (auto* child : c.getChildren())
        {
            if (child->getComponentID() == componentId)
                return child;

            if (auto* found = search (*child))
                return found;
        }

        return nullptr;
    };

    return search (*content);
}

void LayoutHost::setContent (std::unique_ptr<juce::Component> newContent)
{
    content = std::move (newContent);
    addAndMakeVisible (*content);
    resized();

    if (onBuilt)
        onBuilt();
}

void LayoutHost::resized()
{
    if (content != nullptr)
        content->setBounds (getLocalBounds());
}

//==============================================================================
LayoutManager::LayoutManager (const LayoutSource& s, ComponentFactory& f, UIStateStore& state)
    : source (s), factory (f), uiState (state),
      focusTracker (std::make_unique<FocusTracker> (state))
{
}

LayoutManager::~LayoutManager() = default;

bool LayoutManager::build (Entry& entry, const juce::String& text)
{
    try
    {
        entry.host->setContent (factory.createFromJson (text));
        entry.builtFrom = text;
        restoreFocus (*entry.host);
        return true;
    }
    catch (const LayoutError& e)
    {
        auto message = entry.host->getLayoutFile() + ": " + juce::String (e.what());
        DBG (message);

        if (onError)
            onError (message);

        return false;
    }
}

void LayoutManager::addHost (LayoutHost& host)
{
    auto& entry = entries.emplace_back (Entry { &host, {} });
    juce::String text;

    if (auto r = source.read (host.getLayoutFile(), text); r.failed())
    {
        host.setContent (std::make_unique<LayoutErrorDisplay> (r.getErrorMessage()));

        if (onError)
            onError (r.getErrorMessage());

        return;
    }

    if (! build (entry, text))
        host.setContent (std::make_unique<LayoutErrorDisplay> (host.getLayoutFile() + " is invalid; see the error report"));
}

juce::StringArray LayoutManager::reloadChanged()
{
    juce::StringArray rebuilt;

    for (auto& entry : entries)
    {
        juce::String text;

        if (auto r = source.read (entry.host->getLayoutFile(), text); r.failed())
        {
            if (onError)
                onError (r.getErrorMessage());

            continue;
        }

        if (text != entry.builtFrom && build (entry, text))
            rebuilt.add (entry.host->getComponentID());
    }

    return rebuilt;
}

void LayoutManager::restoreFocus (LayoutHost& host)
{
    auto focusedId = uiState.getState (windowStateId)[focusProperty].toString();

    if (focusedId.isNotEmpty())
        if (auto* c = host.findById (focusedId); c != nullptr && c->getWantsKeyboardFocus())
            c->grabKeyboardFocus();
}

} // namespace resamper
