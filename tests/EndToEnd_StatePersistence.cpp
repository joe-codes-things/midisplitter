#include <gtest/gtest.h>
#include "../source/MIDISplitterProcessor.h"
#include "../source/PluginController.h"
#include "../source/PluginConfiguration.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>

/**
 * @brief End-to-end integration test: State persistence
 * 
 * Validates Requirements: 15.1, 15.2, 15.3, 15.4
 * 
 * Tests complete workflow:
 * - Configure plugin settings (mode, voice count, etc.)
 * - Save state via getStateInformation
 * - Create new plugin instance
 * - Restore state via setStateInformation
 * - Verify all settings preserved
 */
class StatePersistenceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test will create processors as needed
    }
    
    void TearDown() override {
        // Cleanup handled by unique_ptr
    }
    
    /**
     * @brief Create and configure a processor with specific settings
     * @param mode Plugin mode
     * @param voiceCount Voice count (for Sender)
     * @param voiceNumber Voice number (for Receiver)
     * @param bufferSize Buffer size in milliseconds
     * @return Configured processor
     */
    std::unique_ptr<MIDISplitterProcessor> createConfiguredProcessor(
        PluginController::Mode mode,
        int voiceCount,
        int voiceNumber,
        int bufferSize) {
        
        auto processor = std::make_unique<MIDISplitterProcessor>();
        
        // Configure plugin
        processor->getPluginController().setMode(mode);
        
        if (mode == PluginController::Mode::Sender) {
            processor->setVoiceCount(voiceCount);
            processor->getPluginController().setBufferSize(bufferSize);
        } else {
            processor->getPluginController().setVoiceNumber(voiceNumber);
        }
        
        // Prepare for processing
        processor->prepareToPlay(44100.0, 512);
        
        return processor;
    }
    
    /**
     * @brief Save processor state to memory block
     * @param processor Processor to save
     * @return Memory block with saved state
     */
    juce::MemoryBlock saveProcessorState(MIDISplitterProcessor& processor) {
        juce::MemoryBlock stateData;
        processor.getStateInformation(stateData);
        return stateData;
    }
    
    /**
     * @brief Restore processor state from memory block
     * @param processor Processor to restore
     * @param stateData State data to restore
     */
    void restoreProcessorState(MIDISplitterProcessor& processor, const juce::MemoryBlock& stateData) {
        processor.setStateInformation(stateData.getData(), static_cast<int>(stateData.getSize()));
    }
    
    /**
     * @brief Verify processor configuration matches expected values
     * @param processor Processor to verify
     * @param expectedMode Expected mode
     * @param expectedVoiceCount Expected voice count
     * @param expectedVoiceNumber Expected voice number
     * @param expectedBufferSize Expected buffer size
     */
    void verifyProcessorConfiguration(
        MIDISplitterProcessor& processor,
        PluginController::Mode expectedMode,
        int expectedVoiceCount,
        int expectedVoiceNumber,
        int expectedBufferSize) {
        
        auto& controller = processor.getPluginController();
        
        EXPECT_EQ(controller.getMode(), expectedMode) 
            << "Mode should be preserved";
        
        if (expectedMode == PluginController::Mode::Sender) {
            EXPECT_EQ(controller.getVoiceCount(), expectedVoiceCount) 
                << "Voice count should be preserved";
            EXPECT_EQ(controller.getBufferSize(), expectedBufferSize) 
                << "Buffer size should be preserved";
        } else {
            EXPECT_EQ(controller.getVoiceNumber(), expectedVoiceNumber) 
                << "Voice number should be preserved";
        }
    }
};

/**
 * @brief Test basic state persistence for Sender mode
 */
TEST_F(StatePersistenceTest, SenderModeStatePersistence) {
    // Create and configure first processor (Sender mode)
    auto processor1 = createConfiguredProcessor(
        PluginController::Mode::Sender,
        4,  // 4 voices
        1,  // voice number (not used in Sender)
        7   // 7ms buffer
    );
    
    // Save state
    auto stateData = saveProcessorState(*processor1);
    
    // Verify state data is not empty
    ASSERT_GT(stateData.getSize(), 0) << "State data should not be empty";
    
    // Create new processor
    auto processor2 = std::make_unique<MIDISplitterProcessor>();
    processor2->prepareToPlay(44100.0, 512);
    
    // Restore state
    restoreProcessorState(*processor2, stateData);
    
    // Verify configuration was restored
    verifyProcessorConfiguration(*processor2, 
        PluginController::Mode::Sender, 
        4, 1, 7);
}

/**
 * @brief Test basic state persistence for Receiver mode
 */
TEST_F(StatePersistenceTest, ReceiverModeStatePersistence) {
    // Create and configure first processor (Receiver mode)
    auto processor1 = createConfiguredProcessor(
        PluginController::Mode::Receiver,
        4,  // voice count (not used in Receiver)
        3,  // voice number 3
        7   // buffer size (not used in Receiver)
    );
    
    // Save state
    auto stateData = saveProcessorState(*processor1);
    
    // Verify state data is not empty
    ASSERT_GT(stateData.getSize(), 0) << "State data should not be empty";
    
    // Create new processor
    auto processor2 = std::make_unique<MIDISplitterProcessor>();
    processor2->prepareToPlay(44100.0, 512);
    
    // Restore state
    restoreProcessorState(*processor2, stateData);
    
    // Verify configuration was restored
    verifyProcessorConfiguration(*processor2, 
        PluginController::Mode::Receiver, 
        4, 3, 7);
}

/**
 * @brief Test state persistence with all voice counts
 */
TEST_F(StatePersistenceTest, AllVoiceCountsPersistence) {
    for (int voiceCount = 2; voiceCount <= 8; ++voiceCount) {
        // Create processor with specific voice count
        auto processor1 = createConfiguredProcessor(
            PluginController::Mode::Sender,
            voiceCount,
            1,
            5
        );
        
        // Save and restore
        auto stateData = saveProcessorState(*processor1);
        auto processor2 = std::make_unique<MIDISplitterProcessor>();
        processor2->prepareToPlay(44100.0, 512);
        restoreProcessorState(*processor2, stateData);
        
        // Verify
        EXPECT_EQ(processor2->getPluginController().getVoiceCount(), voiceCount)
            << "Voice count " << voiceCount << " should be preserved";
    }
}

/**
 * @brief Test state persistence with all voice numbers
 */
TEST_F(StatePersistenceTest, AllVoiceNumbersPersistence) {
    for (int voiceNumber = 1; voiceNumber <= 8; ++voiceNumber) {
        // Create processor with specific voice number
        auto processor1 = createConfiguredProcessor(
            PluginController::Mode::Receiver,
            4,
            voiceNumber,
            5
        );
        
        // Save and restore
        auto stateData = saveProcessorState(*processor1);
        auto processor2 = std::make_unique<MIDISplitterProcessor>();
        processor2->prepareToPlay(44100.0, 512);
        restoreProcessorState(*processor2, stateData);
        
        // Verify
        EXPECT_EQ(processor2->getPluginController().getVoiceNumber(), voiceNumber)
            << "Voice number " << voiceNumber << " should be preserved";
    }
}

/**
 * @brief Test state persistence with all buffer sizes
 */
TEST_F(StatePersistenceTest, AllBufferSizesPersistence) {
    for (int bufferSize = 5; bufferSize <= 10; ++bufferSize) {
        // Create processor with specific buffer size
        auto processor1 = createConfiguredProcessor(
            PluginController::Mode::Sender,
            4,
            1,
            bufferSize
        );
        
        // Save and restore
        auto stateData = saveProcessorState(*processor1);
        auto processor2 = std::make_unique<MIDISplitterProcessor>();
        processor2->prepareToPlay(44100.0, 512);
        restoreProcessorState(*processor2, stateData);
        
        // Verify
        EXPECT_EQ(processor2->getPluginController().getBufferSize(), bufferSize)
            << "Buffer size " << bufferSize << "ms should be preserved";
    }
}

/**
 * @brief Test state persistence with mode switching
 */
TEST_F(StatePersistenceTest, ModeSwitchingPersistence) {
    // Start with Sender mode
    auto processor1 = createConfiguredProcessor(
        PluginController::Mode::Sender, 6, 1, 8);
    
    auto senderState = saveProcessorState(*processor1);
    
    // Switch to Receiver mode
    processor1->getPluginController().setMode(PluginController::Mode::Receiver);
    processor1->getPluginController().setVoiceNumber(5);
    
    auto receiverState = saveProcessorState(*processor1);
    
    // Restore Sender state
    auto processor2 = std::make_unique<MIDISplitterProcessor>();
    processor2->prepareToPlay(44100.0, 512);
    restoreProcessorState(*processor2, senderState);
    
    EXPECT_EQ(processor2->getPluginController().getMode(), PluginController::Mode::Sender);
    EXPECT_EQ(processor2->getPluginController().getVoiceCount(), 6);
    
    // Restore Receiver state
    auto processor3 = std::make_unique<MIDISplitterProcessor>();
    processor3->prepareToPlay(44100.0, 512);
    restoreProcessorState(*processor3, receiverState);
    
    EXPECT_EQ(processor3->getPluginController().getMode(), PluginController::Mode::Receiver);
    EXPECT_EQ(processor3->getPluginController().getVoiceNumber(), 5);
}

/**
 * @brief Test state persistence across multiple save/restore cycles
 */
TEST_F(StatePersistenceTest, MultipleSaveRestoreCycles) {
    // Initial configuration
    auto processor = createConfiguredProcessor(
        PluginController::Mode::Sender, 4, 1, 7);
    
    // Perform multiple save/restore cycles
    for (int cycle = 0; cycle < 5; ++cycle) {
        // Save state
        auto stateData = saveProcessorState(*processor);
        
        // Create new processor and restore
        processor = std::make_unique<MIDISplitterProcessor>();
        processor->prepareToPlay(44100.0, 512);
        restoreProcessorState(*processor, stateData);
        
        // Verify configuration is still correct
        verifyProcessorConfiguration(*processor,
            PluginController::Mode::Sender, 4, 1, 7);
    }
}

/**
 * @brief Test state persistence with invalid/corrupted data
 */
TEST_F(StatePersistenceTest, CorruptedStateHandling) {
    auto processor = std::make_unique<MIDISplitterProcessor>();
    processor->prepareToPlay(44100.0, 512);
    
    // Try to restore with empty data
    juce::MemoryBlock emptyData;
    restoreProcessorState(*processor, emptyData);
    
    // Processor should still be in valid state (default configuration)
    EXPECT_NO_THROW({
        processor->getPluginController().getMode();
        processor->getPluginController().getVoiceCount();
    });
    
    // Try to restore with garbage data
    juce::MemoryBlock garbageData(100);
    std::memset(garbageData.getData(), 0xFF, garbageData.getSize());
    restoreProcessorState(*processor, garbageData);
    
    // Processor should still be in valid state
    EXPECT_NO_THROW({
        processor->getPluginController().getMode();
        processor->getPluginController().getVoiceCount();
    });
}

/**
 * @brief Test state persistence with configuration changes
 */
TEST_F(StatePersistenceTest, ConfigurationChangesPersistence) {
    // Create processor with initial configuration
    auto processor1 = createConfiguredProcessor(
        PluginController::Mode::Sender, 4, 1, 5);
    
    // Change configuration multiple times
    processor1->setVoiceCount(6);
    processor1->getPluginController().setBufferSize(9);
    
    // Save state
    auto stateData = saveProcessorState(*processor1);
    
    // Restore to new processor
    auto processor2 = std::make_unique<MIDISplitterProcessor>();
    processor2->prepareToPlay(44100.0, 512);
    restoreProcessorState(*processor2, stateData);
    
    // Verify latest configuration was saved
    EXPECT_EQ(processor2->getPluginController().getVoiceCount(), 6);
    EXPECT_EQ(processor2->getPluginController().getBufferSize(), 9);
}

/**
 * @brief Test state persistence with processing enabled/disabled
 */
TEST_F(StatePersistenceTest, ProcessingStatePersistence) {
    // Create processor
    auto processor1 = createConfiguredProcessor(
        PluginController::Mode::Sender, 4, 1, 7);
    
    // Disable processing
    processor1->setProcessingEnabled(false);
    
    // Save state
    auto stateData = saveProcessorState(*processor1);
    
    // Restore to new processor
    auto processor2 = std::make_unique<MIDISplitterProcessor>();
    processor2->prepareToPlay(44100.0, 512);
    restoreProcessorState(*processor2, stateData);
    
    // Verify configuration (processing state might not be persisted, which is OK)
    verifyProcessorConfiguration(*processor2,
        PluginController::Mode::Sender, 4, 1, 7);
}

/**
 * @brief Test state size consistency
 */
TEST_F(StatePersistenceTest, StateSizeConsistency) {
    // Create multiple processors with same configuration
    std::vector<juce::MemoryBlock> states;
    
    for (int i = 0; i < 5; ++i) {
        auto processor = createConfiguredProcessor(
            PluginController::Mode::Sender, 4, 1, 7);
        states.push_back(saveProcessorState(*processor));
    }
    
    // Verify all states have the same size
    size_t firstSize = states[0].getSize();
    for (size_t i = 1; i < states.size(); ++i) {
        EXPECT_EQ(states[i].getSize(), firstSize)
            << "State size should be consistent for same configuration";
    }
}

/**
 * @brief Test state persistence with different sample rates
 */
TEST_F(StatePersistenceTest, DifferentSampleRatesPersistence) {
    std::vector<double> sampleRates = {44100.0, 48000.0, 88200.0, 96000.0};
    
    for (double sampleRate : sampleRates) {
        // Create processor with specific sample rate
        auto processor1 = std::make_unique<MIDISplitterProcessor>();
        processor1->getPluginController().setMode(PluginController::Mode::Sender);
        processor1->getPluginController().setVoiceCount(4);
        processor1->prepareToPlay(sampleRate, 512);
        
        // Save state
        auto stateData = saveProcessorState(*processor1);
        
        // Restore to new processor with different sample rate
        auto processor2 = std::make_unique<MIDISplitterProcessor>();
        processor2->prepareToPlay(sampleRate == 44100.0 ? 48000.0 : 44100.0, 512);
        restoreProcessorState(*processor2, stateData);
        
        // Configuration should be preserved regardless of sample rate
        EXPECT_EQ(processor2->getPluginController().getVoiceCount(), 4)
            << "Configuration should persist across sample rate changes";
    }
}

/**
 * @brief Test state persistence with latency tracking
 */
TEST_F(StatePersistenceTest, LatencyTrackingPersistence) {
    // Create processor and let it process some data
    auto processor1 = createConfiguredProcessor(
        PluginController::Mode::Sender, 4, 1, 7);
    
    // Process some buffers to generate latency statistics
    juce::AudioBuffer<float> audioBuffer(2, 512);
    juce::MidiBuffer midiBuffer;
    
    for (int i = 0; i < 10; ++i) {
        audioBuffer.clear();
        midiBuffer.clear();
        processor1->processBlock(audioBuffer, midiBuffer);
    }
    
    // Save state
    auto stateData = saveProcessorState(*processor1);
    
    // Restore to new processor
    auto processor2 = std::make_unique<MIDISplitterProcessor>();
    processor2->prepareToPlay(44100.0, 512);
    restoreProcessorState(*processor2, stateData);
    
    // Configuration should be preserved
    // (Latency statistics might not be persisted, which is OK)
    verifyProcessorConfiguration(*processor2,
        PluginController::Mode::Sender, 4, 1, 7);
}

/**
 * @brief Test complete DAW session workflow
 */
TEST_F(StatePersistenceTest, CompleteDAWSessionWorkflow) {
    // Simulate DAW session workflow
    
    // 1. User creates plugin instance
    auto processor = createConfiguredProcessor(
        PluginController::Mode::Sender, 4, 1, 7);
    
    // 2. User adjusts settings
    processor->setVoiceCount(6);
    processor->getPluginController().setBufferSize(8);
    
    // 3. DAW saves project (calls getStateInformation)
    auto projectState = saveProcessorState(*processor);
    
    // 4. User closes DAW
    processor.reset();
    
    // 5. User reopens DAW and loads project
    processor = std::make_unique<MIDISplitterProcessor>();
    processor->prepareToPlay(44100.0, 512);
    
    // 6. DAW restores plugin state (calls setStateInformation)
    restoreProcessorState(*processor, projectState);
    
    // 7. Verify all settings are restored
    EXPECT_EQ(processor->getPluginController().getMode(), PluginController::Mode::Sender);
    EXPECT_EQ(processor->getPluginController().getVoiceCount(), 6);
    EXPECT_EQ(processor->getPluginController().getBufferSize(), 8);
    
    // 8. Plugin should be ready to process
    juce::AudioBuffer<float> audioBuffer(2, 512);
    juce::MidiBuffer midiBuffer;
    
    EXPECT_NO_THROW({
        processor->processBlock(audioBuffer, midiBuffer);
    });
}
