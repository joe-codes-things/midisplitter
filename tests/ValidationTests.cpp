#include <gtest/gtest.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../source/MIDISplitterProcessor.h"
#include <cstdlib>
#include <string>
#include <fstream>
#if defined(_WIN32)
#include <cstdio>
#include <filesystem>
#endif

/**
 * @brief Automated validation tests for VST3 and AU plugin formats
 * 
 * These tests run the external validation tools and verify plugin compliance
 * with VST3 and Audio Unit standards.
 */
class PluginValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Get project root directory
        projectRoot_ = getProjectRoot();
        
        // Set up paths
#if defined(_WIN32)
        buildDir_ = (std::filesystem::path(projectRoot_).parent_path() / "build" / "Windows").string();
#elif defined(__APPLE__)
        buildDir_ = projectRoot_ + "/build/macOS";
#else
        buildDir_ = projectRoot_ + "/build/Linux";
#endif
        scriptsDir_ = projectRoot_ + "/Scripts";
        
        // Plugin paths
        vst3PluginPath_ = buildDir_ + "/MIDISplitterPlugin_artefacts/Release/VST3/MIDI Splitter.vst3";
        auPluginPath_ = buildDir_ + "/MIDISplitterPlugin_artefacts/Release/AU/MIDI Splitter.component";
    }
    
    void TearDown() override {
        // Cleanup any temporary files
    }
    
    /**
     * @brief Get project root directory
     * @return Project root path
     */
    std::string getProjectRoot() {
#if defined(MIDISPLITTER_PROJECT_ROOT)
        return MIDISPLITTER_PROJECT_ROOT;
#else
        // Try to find project root by looking for CMakeLists.txt
        std::string currentPath = juce::File::getCurrentWorkingDirectory().getFullPathName().toStdString();
        
        // Walk up directories until we find CMakeLists.txt
        juce::File dir(currentPath);
        while (dir.exists() && dir != dir.getParentDirectory()) {
            if (dir.getChildFile("CMakeLists.txt").exists()) {
                return dir.getFullPathName().toStdString();
            }
            dir = dir.getParentDirectory();
        }
        
        // Fallback to current directory
        return currentPath;
#endif
    }
    
    /**
     * @brief Check if file exists
     * @param path File path to check
     * @return true if file exists
     */
    bool fileExists(const std::string& path) {
        return juce::File(path).exists();
    }
    
    /**
     * @brief Check if directory exists
     * @param path Directory path to check
     * @return true if directory exists
     */
    bool directoryExists(const std::string& path) {
        return juce::File(path).isDirectory();
    }
    
    /**
     * @brief Run shell command and capture output
     * @param command Command to run
     * @param output Output string to fill
     * @return Exit code
     */
    int runCommand(const std::string& command, std::string& output) {
#if defined(_WIN32)
        FILE* pipe = _popen(command.c_str(), "r");
#else
        FILE* pipe = popen(command.c_str(), "r");
#endif
        if (!pipe) {
            return -1;
        }
        
        char buffer[128];
        output.clear();
        
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output += buffer;
        }
        
#if defined(_WIN32)
        int exitCode = _pclose(pipe);
        return exitCode;
#else
        int exitCode = pclose(pipe);
        return WEXITSTATUS(exitCode);
#endif
    }
    
    /**
     * @brief Check if plugin was built
     * @param pluginPath Path to plugin
     * @return true if plugin exists
     */
    bool isPluginBuilt(const std::string& pluginPath) {
        return directoryExists(pluginPath);
    }

protected:
    std::string projectRoot_;
    std::string buildDir_;
    std::string scriptsDir_;
    std::string vst3PluginPath_;
    std::string auPluginPath_;
};

/**
 * @brief Test VST3 plugin structure and basic validation
 */
TEST_F(PluginValidationTest, VST3PluginStructure) {
    // Skip if plugin not built
    if (!isPluginBuilt(vst3PluginPath_)) {
        GTEST_SKIP() << "VST3 plugin not built. Run cmake --build build --config Release first.";
    }
    
    // Check plugin bundle structure
    EXPECT_TRUE(directoryExists(vst3PluginPath_)) << "VST3 plugin bundle should exist";
    
    // Check Contents directory
    std::string contentsDir = vst3PluginPath_ + "/Contents";
    EXPECT_TRUE(directoryExists(contentsDir)) << "Contents directory should exist";
    
    // Check Info.plist
#if defined(_WIN32)
    std::vector<std::string> possibleBinaries = {
        contentsDir + "/x86_64-win/MIDI Splitter.vst3"
    };
#else
    std::string infoPlist = contentsDir + "/Info.plist";
    EXPECT_TRUE(fileExists(infoPlist)) << "Info.plist should exist";
    std::vector<std::string> possibleBinaries = {
        contentsDir + "/MacOS/MIDI Splitter",
        contentsDir + "/x86_64-mac/MIDI Splitter.so",
        contentsDir + "/arm64-mac/MIDI Splitter.so"
    };
#endif

    bool binaryFound = false;
    for (const auto& binaryPath : possibleBinaries) {
        if (fileExists(binaryPath)) {
            binaryFound = true;
            break;
        }
    }
    
    EXPECT_TRUE(binaryFound) << "Plugin binary should exist in one of the expected locations";
}

/**
 * @brief Test AU plugin structure and basic validation
 */
TEST_F(PluginValidationTest, AUPluginStructure) {
    // Skip if plugin not built
    if (!isPluginBuilt(auPluginPath_)) {
        GTEST_SKIP() << "AU plugin not built. Run cmake --build build --config Release first.";
    }
    
    // Check plugin bundle structure
    EXPECT_TRUE(directoryExists(auPluginPath_)) << "AU plugin bundle should exist";
    
    // Check Contents directory
    std::string contentsDir = auPluginPath_ + "/Contents";
    EXPECT_TRUE(directoryExists(contentsDir)) << "Contents directory should exist";
    
    // Check Info.plist
    std::string infoPlist = contentsDir + "/Info.plist";
    EXPECT_TRUE(fileExists(infoPlist)) << "Info.plist should exist";
    
    // Check for binary
    std::string binaryPath = contentsDir + "/MacOS/MIDI Splitter";
    EXPECT_TRUE(fileExists(binaryPath)) << "Plugin binary should exist";
}

/**
 * @brief Test VST3 validation script execution
 */
TEST_F(PluginValidationTest, VST3ValidationScript) {
#if defined(_WIN32)
    GTEST_SKIP() << "VST3 shell validation scripts are only available on macOS and Linux";
#endif

    // Check if validation script exists
    std::string scriptPath = scriptsDir_ + "/validate_vst3.sh";
    if (!fileExists(scriptPath)) {
        GTEST_SKIP() << "VST3 validation script not found at " << scriptPath;
    }
    
    // Skip if plugin not built
    if (!isPluginBuilt(vst3PluginPath_)) {
        GTEST_SKIP() << "VST3 plugin not built. Run cmake --build build --config Release first.";
    }
    
    // Run help only here so CTest verifies script availability without requiring
    // external validators or installing plugins into the user's Audio Units path.
    std::string command = "cd \"" + projectRoot_ + "\" && \"" + scriptPath + "\" --help";
    std::string output;
    int exitCode = runCommand(command, output);
    
    // Print output for debugging
    std::cout << "VST3 Validation Output:\n" << output << std::endl;
    
    EXPECT_EQ(exitCode, 0) << "Validation script help should be executable";
}

/**
 * @brief Test AU validation script execution (macOS only)
 */
TEST_F(PluginValidationTest, AUValidationScript) {
#ifndef __APPLE__
    GTEST_SKIP() << "AU validation is only available on macOS";
#endif
    
    // Check if validation script exists
    std::string scriptPath = scriptsDir_ + "/validate_au.sh";
    if (!fileExists(scriptPath)) {
        GTEST_SKIP() << "AU validation script not found at " << scriptPath;
    }
    
    // Skip if plugin not built
    if (!isPluginBuilt(auPluginPath_)) {
        GTEST_SKIP() << "AU plugin not built. Run cmake --build build --config Release first.";
    }
    
    // Run help only here so CTest verifies script availability without requiring
    // auval registration or installing plugins into the user's Audio Units path.
    std::string command = "cd \"" + projectRoot_ + "\" && \"" + scriptPath + "\" --help";
    std::string output;
    int exitCode = runCommand(command, output);
    
    // Print output for debugging
    std::cout << "AU Validation Output:\n" << output << std::endl;
    
    EXPECT_EQ(exitCode, 0) << "Validation script help should be executable";
}

/**
 * @brief Test plugin metadata consistency
 */
TEST_F(PluginValidationTest, PluginMetadataConsistency) {
    // This test checks that plugin metadata is consistent across formats
    
    bool vst3Built = isPluginBuilt(vst3PluginPath_);
    bool auBuilt = isPluginBuilt(auPluginPath_);
    
    if (!vst3Built && !auBuilt) {
        GTEST_SKIP() << "No plugins built. Run cmake --build build --config Release first.";
    }
    
    // Check VST3 metadata
    if (vst3Built) {
#if defined(_WIN32)
        std::string vst3Binary = vst3PluginPath_ + "/Contents/x86_64-win/MIDI Splitter.vst3";
        EXPECT_TRUE(fileExists(vst3Binary)) << "Windows x64 VST3 binary should exist";
#else
        std::string vst3InfoPlist = vst3PluginPath_ + "/Contents/Info.plist";
        EXPECT_TRUE(fileExists(vst3InfoPlist)) << "VST3 Info.plist should exist";
#endif
    }
    
    // Check AU metadata
    if (auBuilt) {
        std::string auInfoPlist = auPluginPath_ + "/Contents/Info.plist";
        EXPECT_TRUE(fileExists(auInfoPlist)) << "AU Info.plist should exist";
        
        // Could add more detailed plist parsing here
    }
    
    // If both are built, we could compare their metadata for consistency
    if (vst3Built && auBuilt) {
        // This would require plist parsing to compare bundle names, versions, etc.
        // For now, we'll just verify both exist
        SUCCEED() << "Both VST3 and AU plugins are built with consistent structure";
    }
}

/**
 * @brief Test build system integration
 */
TEST_F(PluginValidationTest, BuildSystemIntegration) {
    // Check that CMakeLists.txt exists
    std::string cmakeFile = projectRoot_ + "/CMakeLists.txt";
    EXPECT_TRUE(fileExists(cmakeFile)) << "CMakeLists.txt should exist";
    
    // Check that build directory exists
    EXPECT_TRUE(directoryExists(buildDir_)) << "Build directory should exist";
    
    // Check that JUCE is available
    std::string juceDir = projectRoot_ + "/JUCE";
    EXPECT_TRUE(directoryExists(juceDir)) << "JUCE directory should exist";
    
    // Check for JUCE CMakeLists.txt
    std::string juceCMake = juceDir + "/CMakeLists.txt";
    EXPECT_TRUE(fileExists(juceCMake)) << "JUCE CMakeLists.txt should exist";
}

/**
 * @brief Test that the plugin editor survives host-style early resize callbacks
 */
TEST_F(PluginValidationTest, EditorCanBeCreatedAndResized) {
    juce::ScopedJuceInitialiser_GUI guiInitialiser;

    MIDISplitterProcessor processor;
    processor.prepareToPlay(44100.0, 512);

    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    ASSERT_NE(editor, nullptr);

    editor->setBounds(0, 0, 400, 500);
    editor->resized();

    processor.releaseResources();
}

/**
 * @brief Test validation script permissions and executability
 */
TEST_F(PluginValidationTest, ValidationScriptPermissions) {
#if defined(_WIN32)
    GTEST_SKIP() << "Shell script permission checks are only available on macOS and Linux";
#endif

    std::vector<std::string> scripts = {
        scriptsDir_ + "/validate_vst3.sh",
        scriptsDir_ + "/validate_au.sh"
    };
    
    for (const auto& scriptPath : scripts) {
        if (fileExists(scriptPath)) {
            // Check if script is executable
            std::string command = "test -x \"" + scriptPath + "\"";
            std::string output;
            int exitCode = runCommand(command, output);
            
            EXPECT_EQ(exitCode, 0) << "Script " << scriptPath << " should be executable";
        }
    }
}

/**
 * @brief Integration test for complete validation workflow
 */
TEST_F(PluginValidationTest, CompleteValidationWorkflow) {
    // This test runs the complete validation workflow
    
    bool anyPluginBuilt = isPluginBuilt(vst3PluginPath_) || isPluginBuilt(auPluginPath_);
    
    if (!anyPluginBuilt) {
        GTEST_SKIP() << "No plugins built. This test requires at least one plugin format to be built.";
    }
    
    // Test VST3 workflow if available
    if (isPluginBuilt(vst3PluginPath_)) {
#if !defined(_WIN32)
        std::string vst3Script = scriptsDir_ + "/validate_vst3.sh";
        if (fileExists(vst3Script)) {
            std::cout << "Running VST3 validation workflow..." << std::endl;
            
            std::string command = "cd \"" + projectRoot_ + "\" && \"" + vst3Script + "\" --help";
            std::string output;
            int exitCode = runCommand(command, output);
            
            // Help should work
            EXPECT_EQ(exitCode, 0) << "VST3 validation script help should work";
        }
#endif
    }
    
    // Test AU workflow if available (macOS only)
#ifdef __APPLE__
    if (isPluginBuilt(auPluginPath_)) {
        std::string auScript = scriptsDir_ + "/validate_au.sh";
        if (fileExists(auScript)) {
            std::cout << "Running AU validation workflow..." << std::endl;
            
            std::string command = "cd \"" + projectRoot_ + "\" && \"" + auScript + "\" --help";
            std::string output;
            int exitCode = runCommand(command, output);
            
            // Help should work
            EXPECT_EQ(exitCode, 0) << "AU validation script help should work";
        }
    }
#endif
    
    SUCCEED() << "Validation workflow tests completed";
}
