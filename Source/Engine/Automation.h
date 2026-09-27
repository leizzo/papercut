#pragma once

#include "Engine/ProjectManager.h"

#include <vector>

namespace papercut
{

/** One breakpoint on a parameter's automation curve.

    value is the parameter's native unit: dB for "volume"
    (ApplicationModel::minVolumeDb..maxVolumeDb), -1..1 for "pan",
    and 0..1 for a plug-in parameter.
*/
struct AutomationPointInfo
{
    int index = 0;
    double timeSeconds = 0;
    float value = 0;
};

/** A parameter that can be automated on a track. */
struct ParameterInfo
{
    juce::String key;
    juce::String name;
};

/** Facade over one parameter's Tracktion AutomationCurve (ADR-0001, ADR-0012).

    A target parameter is "volume", "pan", or "plugin:<pluginItemId>:<parameterID>"
    for an insert ahead of the track's volume plug-in. Every call re-reads
    ProjectManager::getEdit(). An unknown track or key changes nothing and
    records no undo step.
*/
class Automation
{
public:
    explicit Automation (ProjectManager&);

    std::vector<ParameterInfo> getTargets (const juce::String& trackId) const;
    std::vector<AutomationPointInfo> getPoints (const juce::String& trackId, const juce::String& parameterKey) const;

    bool addPoint (const juce::String& trackId, const juce::String& parameterKey, double timeSeconds, float value);
    bool movePoint (const juce::String& trackId, const juce::String& parameterKey, int index, double timeSeconds, float value);
    bool removePoint (const juce::String& trackId, const juce::String& parameterKey, int index);
    bool clear (const juce::String& trackId, const juce::String& parameterKey);

private:
    ProjectManager& projects;

    JUCE_DECLARE_NON_COPYABLE (Automation)
};

} // namespace papercut
