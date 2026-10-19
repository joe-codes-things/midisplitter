cmake_minimum_required(VERSION 3.22)

project(MIDISplitterPlugin VERSION 0.0.17)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Force Release build if not specified (prevents debug bloat)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    message(STATUS "No build type specified, defaulting to Release")
endif()

# Verify we're building in Release mode
if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    message(WARNING "Building in ${CMAKE_BUILD_TYPE} mode - binary size will be larger than Release")
endif()

message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")

# Detect current git branch for beta/experimental naming
execute_process(
    COMMAND git branch --show-current
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

# Set product name based on branch
if(GIT_BRANCH STREQUAL "main" OR GIT_BRANCH STREQUAL "")
    set(PLUGIN_PRODUCT_NAME "MIDI Splitter")
else()
    # Extract feature name from branch (e.g., "feature/daw-automation" -> "daw-automation")
    string(REGEX REPLACE "^(feature|fix|experiment)/(.+)$" "\\2" BRANCH_NAME "${GIT_BRANCH}")
    # Use NLA naming convention: nla_midisplitter_branch-name_v0.0.1
    set(PLUGIN_PRODUCT_NAME "nla_midisplitter_${BRANCH_NAME}_v0.0.1")
endif()

message(STATUS "Git Branch: ${GIT_BRANCH}")
message(STATUS "Plugin Name: ${PLUGIN_PRODUCT_NAME}")

# Add JUCE subdirectory (local installation)
add_subdirectory(JUCE)

# Audio Units remain part of the Apple build; Windows supports VST3 only.
if(WIN32)
    set(PLUGIN_FORMATS VST3)
else()
    set(PLUGIN_FORMATS VST3 AU)
endif()

# Create the plugin target
# 
# ⚠️  CRITICAL: AU_MAIN_TYPE MUST BE kAudioUnitType_MusicDevice
# DO NOT change this to kAudioUnitType_MIDIProcessor or any other type.
# Logic and other DAWs require this to be a MusicDevice (instrument) type.
# Changing this will break AU functionality in Logic Pro.
# 
juce_add_plugin(MIDISplitterPlugin
    COMPANY_NAME "Nine Lives Audio"
    PLUGIN_MANUFACTURER_CODE Nliv
    PLUGIN_CODE Mdsp
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "${PLUGIN_PRODUCT_NAME}"
    DESCRIPTION "Distribute MIDI notes across multiple plugin instances"
    VERSION "${PROJECT_VERSION}"
    # IS_MIDI_EFFECT affects JUCE's MIDI handling and VST3 categorization
    # It does NOT control AU categorization (that's handled by AU_MAIN_TYPE)
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES "Instrument" "MIDI"
    # ⚠️  CRITICAL: DO NOT CHANGE AU_MAIN_TYPE FROM kAudioUnitType_MusicDevice
    # This MUST remain as MusicDevice for Logic Pro compatibility
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    AU_SANDBOX_SAFE TRUE
    PLUGIN_AAX_IDENTIFIER com.ninelivesaudio.midisplitter
    BUNDLE_ID com.ninelivesaudio.midisplitter
)

# Add source files
target_sources(MIDISplitterPlugin
    PRIVATE
        source/PluginEntry.cpp
        source/MIDISplitterProcessor.h
        source/MIDISplitterEditor.h
        source/MIDIEvent.h
        source/VoiceAssignment.h
        source/PluginState.h
        source/ActiveNote.h
        source/IPCMessage.h
        source/MIDIParser.h
        source/BufferManager.h
        source/PitchSorter.h
        source/DistributionAlgorithm.h
        source/VoiceAssignmentTracker.h
        source/MIDIProcessor.h
        source/IPCManagerVST3.h
        source/IPCManagerAU.h
        source/LockFreeQueue.h
        source/PluginController.h
        source/PluginConfiguration.h
        source/IPCErrorHandler.h
        source/MIDIParsingErrorHandler.h
        source/BufferOverflowHandler.h
        source/ConfigurationValidator.h
        source/SenderPipeline.h
        source/ReceiverPipeline.h
        source/VoiceCountSelector.h
        source/LatencyTracker.h
        source/IPCTraceLogger.h
        source/VST3DiscoveryManager.h
        source/IPCManagerVST3_Socket.h
        source/CCFilterCheckbox.h
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
set(PLUGIN_TEST_SOURCES
    tests/test_main.cpp
    tests/test_MIDIEvent.cpp
    tests/test_IPCMessage.cpp
    tests/PropertyTest_ControlChangeBroadcasting.cpp
    tests/ValidationTests.cpp
    tests/EndToEnd_SenderToReceiver_VST3.cpp
    tests/EndToEnd_SustainedNotes.cpp
    tests/EndToEnd_StatePersistence.cpp
    "${VST3_SDK_DIR}/pluginterfaces/base/coreiids.cpp"
    "${VST3_SDK_DIR}/public.sdk/source/vst/vstinitiids.cpp"
)

# The AU integration test constructs raw AudioProcessor instances and therefore
# never exercises the Audio Unit wrapper or AU-specific host communication.

add_executable(MIDISplitterPluginTests ${PLUGIN_TEST_SOURCES})

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
        MIDISPLITTER_PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
)

# Discover tests
include(GoogleTest)
gtest_discover_tests(MIDISplitterPluginTests)
