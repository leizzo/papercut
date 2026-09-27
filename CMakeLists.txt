cmake_minimum_required(VERSION 3.22)

project(Papercut VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")
endif()

# ------------------------------------------------------------------------------
# Pinned dependencies (git submodules, see README.md)
#   external/tracktion_engine  Tracktion Engine 3.5.0 (develop 964583ee: the 3.5.0
#                              release plus an EditSnapshot crash fix needed for Save)
#     -> modules/juce          JUCE 8.0.13-7-g37c894f8, as pinned by Tracktion
#   external/gin               GIN ea795541
# ------------------------------------------------------------------------------
set(TE_ADD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(external/tracktion_engine/modules/juce EXCLUDE_FROM_ALL)
add_subdirectory(external/tracktion_engine EXCLUDE_FROM_ALL)

# Plugin / synth UI uses gin_plugin (ADR-0008). It hard-depends on gin_dsp,
# gin_graphics, and gin_simd, so those modules are registered with it.
juce_add_module(external/gin/modules/gin)
juce_add_module(external/gin/modules/gin_simd)
juce_add_module(external/gin/modules/gin_dsp)
juce_add_module(external/gin/modules/gin_graphics)
juce_add_module(external/gin/modules/gin_gui)
juce_add_module(external/gin/modules/gin_svg)
juce_add_module(external/gin/modules/gin_plugin)

# ------------------------------------------------------------------------------
# papercut_engine: the only target that sees Tracktion headers.
# It compiles every JUCE/Tracktion/GIN module exactly once; all other targets
# link it and borrow its include paths with the Tracktion path filtered out,
# which turns ADR-0001 ("UI never includes Tracktion headers") into a compile error.
# ------------------------------------------------------------------------------
add_library(papercut_engine STATIC
    Source/Engine/EngineManager.cpp
    Source/Engine/ProjectManager.cpp
    Source/Engine/ApplicationModel.cpp
    Source/Engine/ClipWaveform.cpp
    Source/Engine/Production.cpp
    Source/Engine/PluginRack.cpp
    Source/Engine/Mixer.cpp
    Source/Engine/Session.cpp
    Source/Engine/Automation.cpp
    Source/Engine/Shaper.cpp)

target_compile_definitions(papercut_engine PUBLIC
    JUCE_STANDALONE_APPLICATION=1
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_MODAL_LOOPS_PERMITTED=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    # Phase 6 hosts VST3 everywhere and AU on macOS (brief §24).
    JUCE_PLUGINHOST_AU=$<IF:$<BOOL:$<PLATFORM_ID:Darwin>>,1,0>
    JUCE_PLUGINHOST_VST3=1
    # Tempo-tagged loops (e.g. ACID WAVs) are time-stretched to the Edit tempo;
    # without a stretcher their clips are silent and have no waveform.
    # Signalsmith Stretch ships with Tracktion (MIT, header-only).
    TRACKTION_ENABLE_TIMESTRETCH_SIGNALSMITH=1)

target_link_libraries(papercut_engine
    PRIVATE
        tracktion::tracktion_core
        tracktion::tracktion_engine
        tracktion::tracktion_graph
        juce::juce_audio_devices
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_gui_extra
        gin
        gin_simd
        gin_dsp
        gin_graphics
        gin_gui
        gin_svg
        gin_plugin
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)

target_include_directories(papercut_engine PUBLIC Source)
set_target_properties(papercut_engine PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(APPLE)
    target_compile_options(papercut_engine PRIVATE -fno-aligned-allocation)
endif()

set(papercut_module_includes $<TARGET_PROPERTY:papercut_engine,INCLUDE_DIRECTORIES>)
set(papercut_module_definitions $<TARGET_PROPERTY:papercut_engine,COMPILE_DEFINITIONS>)
set(papercut_module_includes_without_tracktion
    $<FILTER:${papercut_module_includes},EXCLUDE,tracktion_engine/modules$>)

# Links a target against papercut_engine with JUCE/GIN headers visible and
# Tracktion headers hidden.
function(papercut_use_engine_without_tracktion target)
    target_include_directories(${target} PRIVATE ${papercut_module_includes_without_tracktion})
    target_compile_definitions(${target} PRIVATE ${papercut_module_definitions})
    target_link_libraries(${target} PRIVATE
        papercut_engine
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
endfunction()

# ------------------------------------------------------------------------------
# UI resources: layouts and themes embedded for production builds.
# Debug builds read the same files from the source tree (dev-mode override).
# ------------------------------------------------------------------------------
juce_add_binary_data(papercut_resources
    HEADER_NAME PapercutResources.h
    NAMESPACE PapercutResources
    SOURCES
        UI/layouts/transport.json
        UI/layouts/statusbar.json
        UI/themes/dark.json
        UI/themes/light.json)

set_target_properties(papercut_resources PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ------------------------------------------------------------------------------
# Commands + UI: compiled without Tracktion on the include path.
# ------------------------------------------------------------------------------
add_library(papercut_commands STATIC
    Source/Commands/CommandRegistry.cpp
    Source/Commands/AppCommands.cpp
    Source/Commands/ApplicationCommandTable.cpp
    Source/Commands/PluginCommands.cpp
    Source/Commands/MixerCommands.cpp
    Source/Commands/SessionCommands.cpp
    Source/Commands/AutomationCommands.cpp)
papercut_use_engine_without_tracktion(papercut_commands)

add_library(papercut_ui STATIC
    Source/UI/State/UIStateStore.cpp
    Source/UI/State/ArrangementViewState.cpp
    Source/UI/Theme/ThemeManager.cpp
    Source/UI/Layout/LayoutSource.cpp
    Source/UI/Layout/ComponentFactory.cpp
    Source/UI/Layout/Primitives.cpp
    Source/UI/Layout/LayoutManager.cpp
    Source/UI/Developer/DeveloperCommands.cpp
    Source/UI/Developer/Inspector.cpp
    Source/UI/Developer/DeveloperOverlay.cpp
    Source/UI/Developer/LayoutWatcher.cpp
    Source/Commands/ProductionCommands.cpp
    Source/UI/Plugins/PluginBrowser.cpp
    Source/UI/Plugins/InsertStrip.cpp
    Source/UI/Plugins/PluginEditorWindow.cpp
    Source/UI/Mixer/ChannelStrip.cpp
    Source/UI/Mixer/MasterStrip.cpp
    Source/UI/Mixer/MixerView.cpp
    Source/UI/Session/SceneHeader.cpp
    Source/UI/Session/SlotComponent.cpp
    Source/UI/Session/SessionView.cpp
    Source/UI/Arrangement/AutomationLane.cpp
    Source/UI/Arrangement/ShaperPanel.cpp
    Source/UI/Arrangement/ArrangementView.cpp
    Source/UI/Arrangement/TimelineHeader.cpp
    Source/UI/Arrangement/TrackList.cpp
    Source/UI/Arrangement/TrackHeader.cpp
    Source/UI/Arrangement/TrackLanes.cpp
    Source/UI/Arrangement/ClipComponent.cpp
    Source/UI/Arrangement/Playhead.cpp
    Source/UI/PianoRoll/NoteComponent.cpp
    Source/UI/PianoRoll/PianoKeyboard.cpp
    Source/UI/PianoRoll/BeatRuler.cpp
    Source/UI/PianoRoll/NoteGrid.cpp
    Source/UI/PianoRoll/VelocityEditor.cpp
    Source/UI/PianoRoll/PianoRollView.cpp
    Source/UI/MainWindow/MainComponent.cpp
    Source/UI/MainWindow/MainWindow.cpp)
papercut_use_engine_without_tracktion(papercut_ui)
target_link_libraries(papercut_ui PUBLIC papercut_commands PRIVATE papercut_resources gin)
target_compile_definitions(papercut_ui PRIVATE
    $<$<CONFIG:Debug>:PAPERCUT_DEV_UI_DIR="${CMAKE_CURRENT_SOURCE_DIR}/UI">)

# ------------------------------------------------------------------------------
# The app
# ------------------------------------------------------------------------------
juce_add_gui_app(Papercut
    PRODUCT_NAME "Papercut"
    BUNDLE_ID "com.papercut.Papercut"
    COMPANY_NAME "Papercut"
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Papercut records audio from your inputs onto armed tracks."
    NEEDS_CURL FALSE
    NEEDS_WEB_BROWSER FALSE)

target_sources(Papercut PRIVATE Source/App/Main.cpp)
papercut_use_engine_without_tracktion(Papercut)
target_link_libraries(Papercut PRIVATE papercut_ui)

# ------------------------------------------------------------------------------
# Headless tests over the Command registry / Application Model seam.
# ------------------------------------------------------------------------------
enable_testing()

juce_add_console_app(PapercutTests PRODUCT_NAME "PapercutTests")

target_sources(PapercutTests PRIVATE
    Tests/TestMain.cpp
    Tests/TestFixture.cpp
    Tests/EngineManagerTests.cpp
    Tests/ProjectTests.cpp
    Tests/TrackCommandTests.cpp
    Tests/TrackChannelTests.cpp
    Tests/TransportCommandTests.cpp
    Tests/ClipCommandTests.cpp
    Tests/ClipEditingTests.cpp
    Tests/RecordingTests.cpp
    Tests/RenderTests.cpp
    Tests/MidiTrackTests.cpp
    Tests/NoteEditingTests.cpp
    Tests/ComponentFactoryTests.cpp
    Tests/ProductionTests.cpp
    Tests/PluginRackTests.cpp
    Tests/MixerTests.cpp
    Tests/SessionTests.cpp
    Tests/AutomationTests.cpp
    Tests/ShaperTests.cpp)

# Tests may reach into the engine to verify rendered audio, so they get the
# full include path, Tracktion included.
target_include_directories(PapercutTests PRIVATE ${papercut_module_includes} Tests)
target_compile_definitions(PapercutTests PRIVATE ${papercut_module_definitions})
target_link_libraries(PapercutTests PRIVATE
    papercut_ui
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

add_test(NAME PapercutTests COMMAND PapercutTests)

