cmake_minimum_required(VERSION 3.22)

project(MIDISplitterPlugin VERSION 0.0.15)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add JUCE subdirectory (local installation)
add_subdirectory(JUCE)

# Define plugin formats to build
set(PLUGIN_FORMATS VST3 AU)

# Create the plugin target
juce_add_plugin(MIDISplitterPlugin
    COMPANY_NAME "Nine Lives Audio"
    PLUGIN_MANUFACTURER_CODE Nliv
    PLUGIN_CODE Mdsp
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "MIDI Splitter"
    DESCRIPTION "Distribute MIDI notes across multiple plugin instances"
    VERSION "0.0.15"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES "Instrument" "MIDI"
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    AU_SANDBOX_SAFE TRUE
    PLUGIN_AAX_IDENTIFIER com.ninelivesaudio.midisplitter
    BUNDLE_ID com.ninelivesaudio.midisplitter
)

# Add source files
target_sources(MIDISplitterPlugin
    PRIVATE
        Source/PluginEntry.cpp
        Source/MIDISplitterProcessor.h
        Source/MIDISplitterEditor.h
        Source/MIDIEvent.h
        Source/VoiceAssignment.h
        Source/PluginState.h
        Source/ActiveNote.h
        Source/IPCMessage.h
        Source/MIDIParser.h
        Source/BufferManager.h
        Source/PitchSorter.h
        Source/DistributionAlgorithm.h
        Source/VoiceAssignmentTracker.h
        Source/MIDIProcessor.h
        Source/IPCManagerVST3.h
        Source/IPCManagerAU.h
        Source/LockFreeQueue.h
        Source/PluginController.h
        Source/PluginConfiguration.h
        Source/IPCErrorHandler.h
        Source/MIDIParsingErrorHandler.h
        Source/BufferOverflowHandler.h
        Source/ConfigurationValidator.h
        Source/SenderPipeline.h
        Source/ReceiverPipeline.h
        Source/VoiceCountSelector.h
        Source/LatencyTracker.h
        Source/IPCTraceLogger.h
        Source/VST3DiscoveryManager.h
        Source/IPCManagerVST3_Socket.h
        Source/CCFilterCheckbox.h
)

set(VST3_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_audio_processors/format_types/VST3_SDK")

target_sources(MIDISplitterPlugin
    PRIVATE
        "${VST3_SDK_DIR}/pluginterfaces/base/coreiids.cpp"
        "${VST3_SDK_DIR}/public.sdk/source/vst/vstinitiids.cpp"
)

target_include_directories(MIDISplitterPlugin
    PRIVATE
        "${VST3_SDK_DIR}"
)

# Set compile definitions
target_compile_definitions(MIDISplitterPlugin
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
)

# Link JUCE modules
target_link_libraries(MIDISplitterPlugin
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Enable testing
enable_testing()

# Add Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Create test executable
add_executable(MIDISplitterPluginTests
    Tests/test_main.cpp
    Tests/test_MIDIEvent.cpp
    Tests/test_IPCMessage.cpp
    Tests/PropertyTest_ControlChangeBroadcasting.cpp
    Tests/ValidationTests.cpp
    Tests/EndToEnd_SenderToReceiver_VST3.cpp
    Tests/EndToEnd_SenderToReceiver_AU.cpp
    Tests/EndToEnd_SustainedNotes.cpp
    Tests/EndToEnd_StatePersistence.cpp
    "${VST3_SDK_DIR}/pluginterfaces/base/coreiids.cpp"
    "${VST3_SDK_DIR}/public.sdk/source/vst/vstinitiids.cpp"
)

target_link_libraries(MIDISplitterPluginTests
    PRIVATE
        GTest::gtest_main
        juce::juce_audio_utils
        juce::juce_audio_processors
)

target_include_directories(MIDISplitterPluginTests
    PRIVATE
        "${VST3_SDK_DIR}"
)

target_compile_definitions(MIDISplitterPluginTests
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

# Discover tests
include(GoogleTest)
gtest_discover_tests(MIDISplitterPluginTests)
