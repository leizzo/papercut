#pragma once

#include "Engine/ApplicationModel.h"
#include "Engine/Session.h"
#include "UI/Theme/ThemeManager.h"

#include <memory>
#include <vector>

namespace papercut
{

class CommandRegistry;
class SceneHeader;
class Session;
class SlotComponent;

/** Session grid: one column per track, one row per scene, with a scene header on each row.

    Mutations go through Session commands. Slot launch state is polled, because a
    launch only queues on this thread; playing flips when the audio thread advances it.
*/
class SessionView : public juce::Component,
                    private ApplicationModel::Listener,
                    private ThemeManager::Listener,
                    private juce::Timer
{
public:
    static constexpr const char* componentId = "session";

    SessionView (ApplicationModel&, Session&, CommandRegistry&, ThemeManager&);
    ~SessionView() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Cell
    {
        juce::String trackId;
        int column = 0;
        int sceneIndex = 0;
        std::unique_ptr<SlotComponent> component;
    };

    ApplicationModel& model;
    Session& session;
    CommandRegistry& commands;
    ThemeManager& themeManager;

    juce::TextButton addSceneButton { "+" }, stopButton { "Stop" }, recordButton { "Rec" };
    std::vector<std::unique_ptr<SceneHeader>> headers;
    std::vector<juce::String> columnTracks;
    std::vector<Cell> cells;

    void refresh();
    void rebuild (const std::vector<TrackInfo>&, const std::vector<SceneInfo>&);
    void updateContents (const std::vector<TrackInfo>&, const std::vector<SceneInfo>&);
    bool shapeMatches (const std::vector<TrackInfo>&, const std::vector<SceneInfo>&) const;
    void applyTheme();

    void modelChanged() override   { refresh(); }
    void themeChanged() override   { applyTheme(); }
    void timerCallback() override;
};

} // namespace papercut
