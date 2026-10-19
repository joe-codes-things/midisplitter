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
if(DEFINED ENV{MIDI_SPLITTER_GIT_BRANCH} AND NOT "$ENV{MIDI_SPLITTER_GIT_BRANCH}" STREQUAL "")
    set(GIT_BRANCH "$ENV{MIDI_SPLITTER_GIT_BRANCH}")
else()
    execute_process(
        COMMAND git branch --show-current
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()

if(GIT_BRANCH STREQUAL "")
    execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    set(GIT_BRANCH "detached-${GIT_COMMIT}")
endif()

# Set product name based on branch
if(GIT_BRANCH STREQUAL "main")
    set(PLUGIN_PRODUCT_NAME "MIDI Splitter")
    set(PLUGIN_CODE Mdsp)
    set(PLUGIN_BUNDLE_ID "com.ninelivesaudio.midisplitter")
else()
    string(TOLOWER "${GIT_BRANCH}" BRANCH_NAME)
    string(REGEX REPLACE "[^a-z0-9]+" "-" BRANCH_NAME "${BRANCH_NAME}")
    string(REGEX REPLACE "^-|-$" "" BRANCH_NAME "${BRANCH_NAME}")
    set(PLUGIN_PRODUCT_NAME "nla_midisplitter_${BRANCH_NAME}_v0.0.1")
    set(PLUGIN_CODE MdDv)
    set(PLUGIN_BUNDLE_ID "com.ninelivesaudio.midisplitter.development")
endif()

message(STATUS "Git Branch: ${GIT_BRANCH}")
message(STATUS "Plugin Name: ${PLUGIN_PRODUCT_NAME}")

# Add JUCE subdirectory (local installation)
add_subdirectory(JUCE_framework)

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
    PLUGIN_CODE ${PLUGIN_CODE}
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "${PLUGIN_PRODUCT_NAME}"
    DESCRIPTION "Distribute MIDI notes across multiple plugin instances"
    VERSION "${PROJECT_VERSION}"
    # Keep MIDI processing enabled while categorizing the VST3 as an effect.
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES "Fx" "MIDI"
    # ⚠️  CRITICAL: DO NOT CHANGE AU_MAIN_TYPE FROM kAudioUnitType_MusicDevice
    # This MUST remain as MusicDevice for Logic Pro compatibility
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    AU_SANDBOX_SAFE TRUE
    BUNDLE_ID ${PLUGIN_BUNDLE_ID}
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

set(VST3_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/JUCE_framework/modules/juce_audio_processors/format_types/VST3_SDK")

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
    tests/test_IPCManagerVST3_Socket.cpp
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
        MIDISPLITTER_BUILD_DIR="${CMAKE_BINARY_DIR}"
        MIDISPLITTER_PLUGIN_NAME="${PLUGIN_PRODUCT_NAME}"
)

# Discover tests
include(GoogleTest)
gtest_discover_tests(MIDISplitterPluginTests)
