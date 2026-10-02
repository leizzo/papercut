#include "PluginScanner.h"

#include <atomic>

namespace resamper::PluginScanner
{

namespace
{
    /** The result file's root; one PluginDescription element per plug-in found. */
    const juce::Identifier resultTag { "RESAMPER_SCAN" };

    constexpr int pollMs = 20, reapMs = 1000;

    /** A worker's exit codes. */
    enum ExitCode { scanned = 0, badArguments = 2, unknownFormat = 3, noThread = 4, unwritable = 5 };

    /** Scans on its own thread, so the message thread stays free for a
        plug-in that is created on it (an AUv3). */
    struct ScanJob : juce::Thread
    {
        ScanJob (juce::AudioPluginFormat& f, juce::String file)
            : juce::Thread ("Plugin Scan Worker"), format (f), fileOrIdentifier (std::move (file)) {}

        void run() override
        {
            format.findAllTypesForFile (found, fileOrIdentifier);
            done.store (true);
        }

        juce::AudioPluginFormat& format;
        const juce::String fileOrIdentifier;
        juce::OwnedArray<juce::PluginDescription> found;
        std::atomic<bool> done { false };
    };

    struct OutOfProcessScanner : juce::KnownPluginList::CustomScanner
    {
        explicit OutOfProcessScanner (int timeout) : timeoutMs (timeout) {}

        /** False (the list blacklists the file) when the worker crashed or timed out. */
        bool findPluginTypesFor (juce::AudioPluginFormat& format, juce::OwnedArray<juce::PluginDescription>& result,
                                 const juce::String& fileOrIdentifier) override
        {
            const auto resultFile = juce::File::createTempFile (".xml");
            const auto worker = juce::File::getSpecialLocation (juce::File::currentExecutableFile);
            juce::ChildProcess process;

            // No output streams: an unread pipe fills up and stalls the worker.
            if (! process.start (juce::StringArray { worker.getFullPathName(), workerFlag, format.getName(),
                                                     fileOrIdentifier, resultFile.getFullPathName() }, 0))
                return scanHere (format, result, fileOrIdentifier);

            const auto started = juce::Time::getMillisecondCounter();

            while (process.isRunning())
            {
                const auto abandoned = cancelled();

                if (abandoned || juce::Time::getMillisecondCounter() - started > (juce::uint32) timeoutMs)
                {
                    process.kill();
                    process.waitForProcessToFinish (reapMs);
                    resultFile.deleteFile();

                    // An abandoned scan says nothing about the plug-in, so it isn't failed.
                    return abandoned;
                }

                juce::Thread::sleep (pollMs);
            }

            // A format the host registered itself (not one of the defaults) is
            // unknown to the worker.
            if (process.getExitCode() == unknownFormat)
                return scanHere (format, result, fileOrIdentifier);

            // A worker that crashed wrote nothing.
            const auto xml = juce::parseXML (resultFile);
            resultFile.deleteFile();

            if (xml == nullptr || ! xml->hasTagName (resultTag))
                return false;

            for (auto* element : xml->getChildIterator())
                if (juce::PluginDescription desc; desc.loadFromXml (*element))
                    result.add (new juce::PluginDescription (desc));

            return true;
        }

        /** When no worker can scan the file: here, rather than not at all. */
        static bool scanHere (juce::AudioPluginFormat& format, juce::OwnedArray<juce::PluginDescription>& result,
                              const juce::String& fileOrIdentifier)
        {
            format.findAllTypesForFile (result, fileOrIdentifier);
            return true;
        }

        bool cancelled() const
        {
            return shouldExit() || juce::Thread::currentThreadShouldExit();
        }

        const int timeoutMs;
    };
}

bool isWorker (int argc, const char* const* argv)
{
    return argc >= 2 && juce::String (argv[1]) == workerFlag;
}

int runWorker (int argc, const char* const* argv, std::vector<std::unique_ptr<juce::AudioPluginFormat>> extraFormats)
{
    if (! isWorker (argc, argv) || argc < 5)
        return badArguments;

   #if JUCE_MAC
    juce::Process::setDockIconVisible (false);
   #endif

    const juce::String formatName (argv[2]), fileOrIdentifier (argv[3]);
    const juce::File resultFile { juce::String (argv[4]) };

    juce::AudioPluginFormatManager formats;
    juce::addDefaultFormatsToManager (formats);

    for (auto& format : extraFormats)
        formats.addFormat (std::move (format));

    juce::AudioPluginFormat* format = nullptr;

    for (auto* candidate : formats.getFormats())
        if (candidate->getName() == formatName)
            format = candidate;

    if (format == nullptr)
        return unknownFormat;

    ScanJob job (*format, fileOrIdentifier);

    if (! job.startThread())
        return noThread;

    auto* messages = juce::MessageManager::getInstance();

    while (! job.done.load())
        messages->runDispatchLoopUntil (pollMs);

    job.stopThread (reapMs);

    juce::XmlElement xml (resultTag);

    for (auto* desc : job.found)
        xml.addChildElement (desc->createXml().release());

    return xml.writeTo (resultFile) ? scanned : unwritable;
}

std::unique_ptr<juce::KnownPluginList::CustomScanner> createScanner (int timeoutMs)
{
    return std::make_unique<OutOfProcessScanner> (timeoutMs);
}

} // namespace resamper::PluginScanner
