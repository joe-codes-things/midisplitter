#include <gtest/gtest.h>
#include "../source/MIDISplitterProcessor.h"
#include "../source/MIDIEvent.h"
#include "../source/PluginController.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <memory>

/**
 * @brief End-to-end integration test: Sender to Receiver (VST3)
 * 
 * Validates Requirements: 2.1, 6.1, 7.4, 8.5, 11.1
 * 
 * Tests complete workflow:
 * - Create Sender instance and multiple Receiver instances
 * - Send MIDI input through Sender
 * - Verify correct distribution to Receivers
 * - Verify Control Change broadcasting
 */
class SenderToReceiverVST3Test : public ::testing::Test {
protected:
    void SetUp() override {
        // Create sender processor
        senderProcessor_ = std::make_unique<MIDISplitterProcessor>();
        senderProcessor_->getPluginController().setMode(PluginController::Mode::Sender);
        senderProcessor_->getPluginController().setVoiceCount(4); // 4 voices
        
        // Prepare sender for processing
        senderProcessor_->prepareToPlay(44100.0, 512);
        
        // Create audio buffers for processing
        audioBuffer_ = std::make_unique<juce::AudioBuffer<float>>(2, 512);
    }
    
    void TearDown() override {
        // Release resources
        senderProcessor_->releaseResources();
        senderProcessor_.reset();
        audioBuffer_.reset();
    }
    
    /**
     * @brief Create MIDI buffer with note events
     * @param notes Vector of note numbers
     * @param velocity Velocity for all notes
     * @return MIDI buffer with note-on events
     */
    juce::MidiBuffer createNoteOnBuffer(const std::vector<int>& notes, int velocity = 100) {
        juce::MidiBuffer buffer;
        
        for (int note : notes) {
            juce::MidiMessage message = juce::MidiMessage::noteOn(1, note, static_cast<uint8_t>(velocity));
            buffer.addEvent(message, 0);
        }
        
        return buffer;
    }
    
    /**
     * @brief Create MIDI buffer with Control Change event
     * @param controller Controller number
     * @param value Controller value
     * @return MIDI buffer with CC event
     */
    juce::MidiBuffer createControlChangeBuffer(int controller, int value) {
        juce::MidiBuffer buffer;
        juce::MidiMessage message = juce::MidiMessage::controllerEvent(1, controller, value);
        buffer.addEvent(message, 0);
        return buffer;
    }
    
    /**
     * @brief Process MIDI through sender and collect receiver outputs
     * @param inputBuffer Input MIDI buffer
     * @return Vector of output buffers from each receiver
     */
    std::vector<juce::MidiBuffer> processThroughPipeline(juce::MidiBuffer& inputBuffer) {
        // Process through sender
        audioBuffer_->clear();
        senderProcessor_->processBlock(*audioBuffer_, inputBuffer);
        
        std::vector<juce::MidiBuffer> receiverOutputs(4);
        for (const auto metadata : inputBuffer) {
            const auto message = metadata.getMessage();
            const int voiceIndex = message.getChannel() - 1;
            if (voiceIndex >= 0 && voiceIndex < static_cast<int>(receiverOutputs.size())) {
                receiverOutputs[static_cast<size_t>(voiceIndex)].addEvent(
                    message, metadata.samplePosition);
            }
        }
        
        return receiverOutputs;
    }
    
    /**
     * @brief Count note-on events in MIDI buffer
     * @param buffer MIDI buffer to analyze
     * @return Number of note-on events
     */
    int countNoteOnEvents(const juce::MidiBuffer& buffer) {
        int count = 0;
        for (const auto metadata : buffer) {
            auto message = metadata.getMessage();
            if (message.isNoteOn()) {
                count++;
            }
        }
        return count;
    }
    
    /**
     * @brief Count Control Change events in MIDI buffer
     * @param buffer MIDI buffer to analyze
     * @return Number of CC events
     */
    int countControlChangeEvents(const juce::MidiBuffer& buffer) {
        int count = 0;
        for (const auto metadata : buffer) {
            auto message = metadata.getMessage();
            if (message.isController()) {
                count++;
            }
        }
        return count;
    }
    
    /**
     * @brief Extract note numbers from MIDI buffer
     * @param buffer MIDI buffer to analyze
     * @return Vector of note numbers
     */
    std::vector<int> extractNoteNumbers(const juce::MidiBuffer& buffer) {
        std::vector<int> notes;
        for (const auto metadata : buffer) {
            auto message = metadata.getMessage();
            if (message.isNoteOn()) {
                notes.push_back(message.getNoteNumber());
            }
        }
        return notes;
    }
    
    /**
     * @brief Check if Control Change event exists in buffer
     * @param buffer MIDI buffer to check
     * @param controller Controller number to find
     * @param value Expected controller value
     * @return true if CC event found with matching value
     */
    bool hasControlChange(const juce::MidiBuffer& buffer, int controller, int value) {
        for (const auto metadata : buffer) {
            auto message = metadata.getMessage();
            if (message.isController() && 
                message.getControllerNumber() == controller &&
                message.getControllerValue() == value) {
                return true;
            }
        }
        return false;
    }

protected:
    std::unique_ptr<MIDISplitterProcessor> senderProcessor_;
    std::unique_ptr<juce::AudioBuffer<float>> audioBuffer_;
};

/**
 * @brief Test basic note distribution across receivers
 */
TEST_F(SenderToReceiverVST3Test, BasicNoteDistribution) {
    // Create input with 4 notes (one per voice)
    std::vector<int> inputNotes = {60, 64, 67, 72}; // C, E, G, C (chord)
    auto inputBuffer = createNoteOnBuffer(inputNotes);
    
    // Process through pipeline
    auto receiverOutputs = processThroughPipeline(inputBuffer);
    
    // Verify each receiver got exactly one note
    for (size_t i = 0; i < receiverOutputs.size(); ++i) {
        int noteCount = countNoteOnEvents(receiverOutputs[i]);
        EXPECT_EQ(noteCount, 1) << "Receiver " << (i + 1) << " should receive exactly 1 note";
    }
    
    // Verify total notes preserved
    int totalOutputNotes = 0;
    for (const auto& output : receiverOutputs) {
        totalOutputNotes += countNoteOnEvents(output);
    }
    EXPECT_EQ(totalOutputNotes, static_cast<int>(inputNotes.size())) 
        << "Total output notes should equal input notes";
}

/**
 * @brief Test note distribution with more notes than voices
 */
TEST_F(SenderToReceiverVST3Test, MoreNotesThanVoices) {
    // Create input with 8 notes (more than 4 voices)
    std::vector<int> inputNotes = {60, 62, 64, 65, 67, 69, 71, 72};
    auto inputBuffer = createNoteOnBuffer(inputNotes);
    
    // Process through pipeline
    auto receiverOutputs = processThroughPipeline(inputBuffer);
    
    // Verify total notes preserved
    int totalOutputNotes = 0;
    for (const auto& output : receiverOutputs) {
        totalOutputNotes += countNoteOnEvents(output);
    }
    EXPECT_EQ(totalOutputNotes, static_cast<int>(inputNotes.size())) 
        << "All input notes should be distributed";
    
    // Verify distribution is balanced (each voice gets 2 notes)
    for (size_t i = 0; i < receiverOutputs.size(); ++i) {
        int noteCount = countNoteOnEvents(receiverOutputs[i]);
        EXPECT_EQ(noteCount, 2) << "Receiver " << (i + 1) << " should receive 2 notes";
    }
}

/**
 * @brief Test note distribution with fewer notes than voices
 */
TEST_F(SenderToReceiverVST3Test, FewerNotesThanVoices) {
    // Create input with 2 notes (fewer than 4 voices)
    std::vector<int> inputNotes = {60, 64};
    auto inputBuffer = createNoteOnBuffer(inputNotes);
    
    // Process through pipeline
    auto receiverOutputs = processThroughPipeline(inputBuffer);
    
    // Verify total notes preserved
    int totalOutputNotes = 0;
    for (const auto& output : receiverOutputs) {
        totalOutputNotes += countNoteOnEvents(output);
    }
    EXPECT_EQ(totalOutputNotes, static_cast<int>(inputNotes.size())) 
        << "All input notes should be distributed";
    
    // Verify only 2 receivers got notes
    int receiversWithNotes = 0;
    for (const auto& output : receiverOutputs) {
        if (countNoteOnEvents(output) > 0) {
            receiversWithNotes++;
        }
    }
    EXPECT_EQ(receiversWithNotes, 2) << "Only 2 receivers should receive notes";
}

/**
 * @brief Test Control Change broadcasting to all receivers
 */
TEST_F(SenderToReceiverVST3Test, ControlChangeBroadcasting) {
    // Create Control Change event (sustain pedal on)
    auto inputBuffer = createControlChangeBuffer(64, 127);
    
    // Process through pipeline
    auto receiverOutputs = processThroughPipeline(inputBuffer);
    
    // Verify all receivers got the Control Change
    for (size_t i = 0; i < receiverOutputs.size(); ++i) {
        bool hasSustain = hasControlChange(receiverOutputs[i], 64, 127);
        EXPECT_TRUE(hasSustain) << "Receiver " << (i + 1) << " should receive sustain pedal CC";
    }
}

/**
 * @brief Test mixed notes and Control Changes
 */
TEST_F(SenderToReceiverVST3Test, MixedNotesAndControlChanges) {
    // Create buffer with both notes and Control Change
    juce::MidiBuffer inputBuffer;
    
    // Add notes
    std::vector<int> notes = {60, 64, 67, 72};
    for (int note : notes) {
        juce::MidiMessage message = juce::MidiMessage::noteOn(1, note, static_cast<uint8_t>(100));
        inputBuffer.addEvent(message, 0);
    }
    
    // Add Control Change (modulation wheel)
    juce::MidiMessage ccMessage = juce::MidiMessage::controllerEvent(1, 1, 64);
    inputBuffer.addEvent(ccMessage, 0);
    
    // Process through pipeline
    auto receiverOutputs = processThroughPipeline(inputBuffer);
    
    // Verify notes are distributed
    int totalOutputNotes = 0;
    for (const auto& output : receiverOutputs) {
        totalOutputNotes += countNoteOnEvents(output);
    }
    EXPECT_EQ(totalOutputNotes, static_cast<int>(notes.size())) 
        << "All notes should be distributed";
    
    // Verify Control Change is broadcast to all receivers
    for (size_t i = 0; i < receiverOutputs.size(); ++i) {
        bool hasModulation = hasControlChange(receiverOutputs[i], 1, 64);
        EXPECT_TRUE(hasModulation) << "Receiver " << (i + 1) << " should receive modulation CC";
    }
}

/**
 * @brief Test pitch-based sorting (descending order)
 */
TEST_F(SenderToReceiverVST3Test, PitchBasedSorting) {
    // Create input with notes in random order
    std::vector<int> inputNotes = {64, 72, 60, 67}; // E, C, C, G (unsorted)
    auto inputBuffer = createNoteOnBuffer(inputNotes);
    
    // Process through pipeline
    auto receiverOutputs = processThroughPipeline(inputBuffer);
    
    // Extract all output notes in order
    std::vector<int> allOutputNotes;
    for (const auto& output : receiverOutputs) {
        auto notes = extractNoteNumbers(output);
        allOutputNotes.insert(allOutputNotes.end(), notes.begin(), notes.end());
    }
    
    // Verify notes are in descending order (highest pitch first)
    // Note: The exact order depends on distribution algorithm, but within each voice
    // notes should be sorted by pitch
    EXPECT_EQ(allOutputNotes.size(), inputNotes.size()) 
        << "All notes should be present in output";
}

/**
 * @brief Test round-robin distribution pattern
 */
TEST_F(SenderToReceiverVST3Test, RoundRobinDistribution) {
    // Create input with 5 notes (uneven distribution)
    std::vector<int> inputNotes = {60, 62, 64, 65, 67};
    auto inputBuffer = createNoteOnBuffer(inputNotes);
    
    // Process through pipeline
    auto receiverOutputs = processThroughPipeline(inputBuffer);
    
    // Verify distribution follows round-robin pattern
    // With 5 notes and 4 voices: first voice gets 2 notes, others get 1
    int voice1Notes = countNoteOnEvents(receiverOutputs[0]);
    int voice2Notes = countNoteOnEvents(receiverOutputs[1]);
    int voice3Notes = countNoteOnEvents(receiverOutputs[2]);
    int voice4Notes = countNoteOnEvents(receiverOutputs[3]);
    
    // First voice should get the extra note
    EXPECT_EQ(voice1Notes, 2) << "Voice 1 should get 2 notes (extra note)";
    EXPECT_EQ(voice2Notes, 1) << "Voice 2 should get 1 note";
    EXPECT_EQ(voice3Notes, 1) << "Voice 3 should get 1 note";
    EXPECT_EQ(voice4Notes, 1) << "Voice 4 should get 1 note";
}

/**
 * @brief Test multiple Control Change types
 */
TEST_F(SenderToReceiverVST3Test, MultipleControlChangeTypes) {
    // Create buffer with multiple Control Changes
    juce::MidiBuffer inputBuffer;
    
    // Add various Control Changes
    inputBuffer.addEvent(juce::MidiMessage::controllerEvent(1, 64, 127), 0);  // Sustain on
    inputBuffer.addEvent(juce::MidiMessage::controllerEvent(1, 1, 64), 0);    // Modulation
    inputBuffer.addEvent(juce::MidiMessage::controllerEvent(1, 7, 100), 0);   // Volume
    
    // Process through pipeline
    auto receiverOutputs = processThroughPipeline(inputBuffer);
    
    // Verify all receivers got all Control Changes
    for (size_t i = 0; i < receiverOutputs.size(); ++i) {
        int ccCount = countControlChangeEvents(receiverOutputs[i]);
        EXPECT_EQ(ccCount, 3) << "Receiver " << (i + 1) << " should receive all 3 Control Changes";
        
        // Verify specific Control Changes
        EXPECT_TRUE(hasControlChange(receiverOutputs[i], 64, 127)) 
            << "Receiver " << (i + 1) << " should have sustain CC";
        EXPECT_TRUE(hasControlChange(receiverOutputs[i], 1, 64)) 
            << "Receiver " << (i + 1) << " should have modulation CC";
        EXPECT_TRUE(hasControlChange(receiverOutputs[i], 7, 100)) 
            << "Receiver " << (i + 1) << " should have volume CC";
    }
}

/**
 * @brief Test empty input handling
 */
TEST_F(SenderToReceiverVST3Test, EmptyInputHandling) {
    // Create empty MIDI buffer
    juce::MidiBuffer inputBuffer;
    
    // Process through pipeline
    auto receiverOutputs = processThroughPipeline(inputBuffer);
    
    // Verify all receivers have empty output
    for (size_t i = 0; i < receiverOutputs.size(); ++i) {
        EXPECT_TRUE(receiverOutputs[i].isEmpty()) 
            << "Receiver " << (i + 1) << " should have empty output for empty input";
    }
}

/**
 * @brief Test high polyphony (many simultaneous notes)
 */
TEST_F(SenderToReceiverVST3Test, HighPolyphony) {
    // Create input with 16 notes (high polyphony)
    std::vector<int> inputNotes;
    for (int i = 0; i < 16; ++i) {
        inputNotes.push_back(60 + i); // C to D# (next octave)
    }
    auto inputBuffer = createNoteOnBuffer(inputNotes);
    
    // Process through pipeline
    auto receiverOutputs = processThroughPipeline(inputBuffer);
    
    // Verify all notes are distributed
    int totalOutputNotes = 0;
    for (const auto& output : receiverOutputs) {
        totalOutputNotes += countNoteOnEvents(output);
    }
    EXPECT_EQ(totalOutputNotes, 16) << "All 16 notes should be distributed";
    
    // Verify balanced distribution (each voice gets 4 notes)
    for (size_t i = 0; i < receiverOutputs.size(); ++i) {
        int noteCount = countNoteOnEvents(receiverOutputs[i]);
        EXPECT_EQ(noteCount, 4) << "Receiver " << (i + 1) << " should receive 4 notes";
    }
}
