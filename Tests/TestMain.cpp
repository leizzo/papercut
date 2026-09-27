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

    for (auto* t : juce::UnitTest::getTestsInCategory ("Papercut"))
        if (argc < 2 || t->getName().containsIgnoreCase (argv[1]))
            selected.add (t);

    runner.runTests (selected);

    int failures = 0;

    for (int i = 0; i < runner.getNumResults(); ++i)
        failures += runner.getResult (i)->failures;

    papercut::test::engineManager = nullptr;
    std::cout << (failures == 0 ? "ALL TESTS PASSED" : juce::String (failures) + " FAILURE(S)") << std::endl;
    return failures == 0 ? 0 : 1;
}
