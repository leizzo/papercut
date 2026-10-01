#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <vector>

namespace resamper
{

/** Out-of-process plug-in scanning (PRD §19: per-plug-in timeout, never
    blocking startup).

    Each plug-in file is scanned by a fresh scan worker: this executable run
    again with workerFlag, which loads only that file and writes what it found
    to a result file. A worker that crashes, or that is still running after the
    timeout (it is killed), leaves no result, and the file is reported as failed
    to scan; the scan moves on to the next file.

    The scan worker never starts the app: main() hands its arguments to
    runWorker first.
*/
namespace PluginScanner
{
    /** The first argument of a scan worker's command line. */
    inline constexpr const char* workerFlag = "--resamper-scan-plugin";

    /** A scan that takes longer than this fails: the worker is killed. */
    inline constexpr int defaultTimeoutMs = 30000;

    /** Whether this process was started as a scan worker. */
    bool isWorker (int argc, const char* const* argv);

    /** Runs a scan worker: scans one file with the named format (one of the
        default formats or extraFormats) off the message thread, which keeps
        running meanwhile, and writes the result file. Returns the exit code.
        Call with JUCE's GUI already initialised. */
    int runWorker (int argc, const char* const* argv,
                   std::vector<std::unique_ptr<juce::AudioPluginFormat>> extraFormats = {});

    /** A scanner for a KnownPluginList: every file in its own worker, killed
        after timeoutMs. A file that fails is blacklisted by the list (see
        KnownPluginList::scanAndAddFile), which is how a failed scan is kept. */
    std::unique_ptr<juce::KnownPluginList::CustomScanner> createScanner (int timeoutMs = defaultTimeoutMs);
}

} // namespace resamper
