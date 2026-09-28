#include "TestFixture.h"

#include <iostream>

namespace papercut::test
{

namespace
{
    EngineManager* engineManager = nullptr;
}

EngineManager& getEngineManager()
{
    jassert (engineManager != nullptr);
    return *engineManager;
}

} // namespace papercut::test

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    // The whole run shares one headless engine, as Tracktion's own TestRunner does.
    papercut::EngineManager engine ("PapercutTests", papercut::EngineManager::AudioDevice::none);
    papercut::test::engineManager = &engine;

    juce::UnitTestRunner runner;
    runner.setAssertOnFailure (false);

    // Optional argument: run only the suites whose name contains it.
    juce::Array<juce::UnitTest*> selected;

    // "--snapshot [name]" renders the UI to PNGs instead (Tests/Snapshots.cpp).
    const auto snapshot = argc >= 2 && juce::String (argv[1]) == "--snapshot";
    const auto filter = snapshot ? (argc >= 3 ? juce::String (argv[2]) : juce::String()) : (argc >= 2 ? juce::String (argv[1]) : juce::String());

    for (auto* t : juce::UnitTest::getTestsInCategory (snapshot ? "Snapshot" : "Papercut"))
        if (filter.isEmpty() || t->getName().containsIgnoreCase (filter))
            selected.add (t);

    runner.runTests (selected);

    int failures = 0;

    for (int i = 0; i < runner.getNumResults(); ++i)
        failures += runner.getResult (i)->failures;

    papercut::test::engineManager = nullptr;
    std::cout << (failures == 0 ? "ALL TESTS PASSED" : juce::String (failures) + " FAILURE(S)") << std::endl;
    return failures == 0 ? 0 : 1;
}
