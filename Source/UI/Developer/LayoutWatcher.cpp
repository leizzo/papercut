#include "LayoutWatcher.h"

#include <gin/gin.h>

namespace papercut
{

struct LayoutWatcher::Impl : private gin::FileSystemWatcher::Listener
{
    Impl (LayoutWatcher& o, const juce::File& folder)
        : owner (o)
    {
        watcher.addListener (this);
        watcher.addFolder (folder);
    }

    ~Impl() override
    {
        watcher.removeListener (this);
    }

    void fileChanged (const juce::File& file, gin::FileSystemWatcher::FileSystemEvent event) override
    {
        if (event == gin::FileSystemWatcher::fileUpdated
            && file.hasFileExtension (".json")
            && owner.onJsonUpdated)
            owner.onJsonUpdated (file);
    }

    LayoutWatcher& owner;
    gin::FileSystemWatcher watcher;
};

LayoutWatcher::LayoutWatcher (const juce::File& folder)
    : impl (std::make_unique<Impl> (*this, folder))
{
}

LayoutWatcher::~LayoutWatcher() = default;

} // namespace papercut
