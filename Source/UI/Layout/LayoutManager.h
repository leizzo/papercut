#pragma once

#include "ComponentFactory.h"

namespace resamper
{

class LayoutSource;
class UIStateStore;

/** A window region whose content is built from one top-level layout file.
    Its component ID is stable across reloads; only its content is replaced. */
class LayoutHost : public juce::Component
{
public:
    LayoutHost (const juce::String& hostId, juce::String layoutFile);

    const juce::String& getLayoutFile() const noexcept   { return layoutFile; }
    juce::Component* getContent() const noexcept         { return content.get(); }

    /** Finds a component in the current content by component ID. */
    juce::Component* findById (const juce::String& componentId) const;

    /** Called after every (re)build, e.g. to fill in dynamic label text. */
    std::function<void()> onBuilt;

    void setContent (std::unique_ptr<juce::Component>);
    void resized() override;

private:
    const juce::String layoutFile;
    std::unique_ptr<juce::Component> content;
};

/** Maps each top-level layout file 1:1 to a LayoutHost and (re)builds hosts
    through the ComponentFactory. Reloading rebuilds only the hosts whose file
    changed; everything else in the window is untouched. */
class LayoutManager
{
public:
    LayoutManager (const LayoutSource&, ComponentFactory&, UIStateStore&);
    ~LayoutManager();

    /** Registers a host and builds its content. A broken layout shows the error
        in the host and is reported through onError. */
    void addHost (LayoutHost&);

    /** Re-reads every layout file and rebuilds the hosts whose file changed.
        A host whose new layout fails keeps its current content.
        Returns the IDs of the rebuilt hosts. */
    juce::StringArray reloadChanged();

    std::function<void (const juce::String& message)> onError;

private:
    struct Entry
    {
        LayoutHost* host;
        juce::String builtFrom;
    };

    const LayoutSource& source;
    ComponentFactory& factory;
    UIStateStore& uiState;
    std::vector<Entry> entries;
    std::unique_ptr<juce::FocusChangeListener> focusTracker;

    bool build (Entry&, const juce::String& text);
    void restoreFocus (LayoutHost&);
};

} // namespace resamper
