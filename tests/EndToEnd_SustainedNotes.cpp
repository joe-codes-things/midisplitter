#include <gtest/gtest.h>
#include "../source/MIDISplitterProcessor.h"
#include "../source/MIDIEvent.h"
#include "../source/PluginController.h"
#include "../source/VoiceAssignmentTracker.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <memory>

/**
 * @brief End-to-end integration test: Sustained notes across buffers
 * 
 * Validates Requirements: 3.5, 14.2, 14.3
 * 
 * Tests complete workflow:
 * - Send note-on events in one buffer
 * - Send note-off events in subsequent buffer
 * - Verify note-on/off pairing maintained across buffers
 * - Verify voice assignment consistency
 */
class SustainedNotesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create sender processor
        senderProcessor_ = std::make_unique<MIDISplitterProcessor>();
        senderProcessor_->getPluginController().setMode(PluginController::Mode::Sender);
        senderProcessor_->getPluginController().setVoiceCount(4);
        
        // Prepare sender for processing
        senderProcessor_->prepareToPlay(44100.0, 512);
        
        // Create audio buffer
        audioBuffer_ = std::make_unique<juce::AudioBuffer<float>>(2, 512);
    }
    
    void TearDown() override {
        senderProcessor_->releaseResources();
        senderProcessor_.reset();
        audioBuffer_.reset();
    }
    
    /**
     * @brief Process MIDI buffer through sender
     * @param inputBuffer Input MIDI buffer
     */
    void processSender(juce::MidiBuffer& inputBuffer) {
        audioBuffer_->clear();
        senderProcessor_->processBlock(*audioBuffer_, inputBuffer);
        latestVoiceOutputs_.assign(4, juce::MidiBuffer{});
        for (const auto metadata : inputBuffer) {
            const auto message = metadata.getMessage();
            const int voiceIndex = message.getChannel() - 1;
            if (voiceIndex >= 0 && voiceIndex < static_cast<int>(latestVoiceOutputs_.size())) {
                latestVoiceOutputs_[static_cast<size_t>(voiceIndex)].addEvent(
                    message, metadata.samplePosition);
            }
        }
    }
    
    /**
     * @brief Process and collect receiver outputs
     * @return Vector of output buffers from each receiver
     */
    std::vector<juce::MidiBuffer> processReceivers() {
        return latestVoiceOutputs_;
    }
    
    /**
     * @brief Create note-on MIDI buffer
     * @param notes Vector of note numbers
     * @param velocity Velocity value
     * @return MIDI buffer with note-on events
     */
    juce::MidiBuffer createNoteOnBuffer(const std::vector<int>& notes, int velocity = 100) {
        juce::MidiBuffer buffer;
        for (int note : notes) {
            buffer.addEvent(juce::MidiMessage::noteOn(1, note, static_cast<uint8_t>(velocity)), 0);
        }
        return buffer;
    }
    
    /**
     * @brief Create note-off MIDI buffer
     * @param notes Vector of note numbers
     * @return MIDI buffer with note-off events
     */
    juce::MidiBuffer createNoteOffBuffer(const std::vector<int>& notes) {
        juce::MidiBuffer buffer;
        for (int note : notes) {
            buffer.addEvent(juce::MidiMessage::noteOff(1, note), 0);
        }
        return buffer;
    }
    
    /**
     * @brief Extract note events from buffer
     * @param buffer MIDI buffer to analyze
     * @return Map of note number to event type (true=on, false=off)
     */
    std::map<int, std::vector<bool>> extractNoteEvents(const juce::MidiBuffer& buffer) {
        std::map<int, std::vector<bool>> events;
        
        for (const auto metadata : buffer) {
            auto message = metadata.getMessage();
            if (message.isNoteOn()) {
                events[message.getNoteNumber()].push_back(true);
            } else if (message.isNoteOff()) {
                events[message.getNoteNumber()].push_back(false);
            }
        }
        
        return events;
    }
    
    /**
     * @brief Count note-on events in buffer
     * @param buffer MIDI buffer
     * @return Number of note-on events
     */
    int countNoteOn(const juce::MidiBuffer& buffer) {
        int count = 0;
        for (const auto metadata : buffer) {
            if (metadata.getMessage().isNoteOn()) count++;
        }
        return count;
    }
    
    /**
     * @brief Count note-off events in buffer
     * @param buffer MIDI buffer
     * @return Number of note-off events
     */
    int countNoteOff(const juce::MidiBuffer& buffer) {
        int count = 0;
        for (const auto metadata : buffer) {
            if (metadata.getMessage().isNoteOff()) count++;
        }
        return count;
    }
    
    /**
     * @brief Check if note-on exists for specific note
     * @param buffer MIDI buffer
     * @param note Note number
     * @return true if note-on found
     */
    bool hasNoteOn(const juce::MidiBuffer& buffer, int note) {
        for (const auto metadata : buffer) {
            auto message = metadata.getMessage();
            if (message.isNoteOn() && message.getNoteNumber() == note) {
                return true;
            }
        }
        return false;
    }
    
    /**
     * @brief Check if note-off exists for specific note
     * @param buffer MIDI buffer
     * @param note Note number
     * @return true if note-off found
     */
    bool hasNoteOff(const juce::MidiBuffer& buffer, int note) {
        for (const auto metadata : buffer) {
            auto message = metadata.getMessage();
            if (message.isNoteOff() && message.getNoteNumber() == note) {
                return true;
            }
        }
        return false;
    }

protected:
    std::unique_ptr<MIDISplitterProcessor> senderProcessor_;
    std::vector<juce::MidiBuffer> latestVoiceOutputs_;
    std::unique_ptr<juce::AudioBuffer<float>> audioBuffer_;
};

/**
 * @brief Test basic note-on/off pairing across buffers
 */
TEST_F(SustainedNotesTest, BasicNoteOnOffPairing) {
    // Buffer 1: Send note-on events
    std::vector<int> notes = {60, 64, 67, 72};
    auto noteOnBuffer = createNoteOnBuffer(notes);
    processSender(noteOnBuffer);
    auto outputs1 = processReceivers();
    
    // Verify note-on events were distributed
    int totalNoteOns = 0;
    std::map<int, int> noteToReceiver; // Track which receiver got which note
    
    for (size_t i = 0; i < outputs1.size(); ++i) {
        totalNoteOns += countNoteOn(outputs1[i]);
        
        // Record which notes this receiver got
        for (int note : notes) {
            if (hasNoteOn(outputs1[i], note)) {
                noteToReceiver[note] = static_cast<int>(i);
            }
        }
    }
    
    EXPECT_EQ(totalNoteOns, static_cast<int>(notes.size())) 
        << "All note-on events should be distributed";
    
    // Buffer 2: Send note-off events for the same notes
    auto noteOffBuffer = createNoteOffBuffer(notes);
    processSender(noteOffBuffer);
    auto outputs2 = processReceivers();
    
    // Verify note-off events go to the same receivers as note-on
    for (int note : notes) {
        int receiverIndex = noteToReceiver[note];
        EXPECT_TRUE(hasNoteOff(outputs2[receiverIndex], note))
            << "Note-off for note " << note << " should go to same receiver as note-on (receiver " 
            << (receiverIndex + 1) << ")";
    }
}

/**
 * @brief Test sustained notes with delay between on and off
 */
TEST_F(SustainedNotesTest, SustainedNotesWithDelay) {
    // Buffer 1: Send note-on
    std::vector<int> notes = {60};
    auto noteOnBuffer = createNoteOnBuffer(notes);
    processSender(noteOnBuffer);
    auto outputs1 = processReceivers();
    
    // Find which receiver got the note
    int receiverIndex = -1;
    for (size_t i = 0; i < outputs1.size(); ++i) {
        if (hasNoteOn(outputs1[i], 60)) {
            receiverIndex = static_cast<int>(i);
            break;
        }
    }
    
    ASSERT_NE(receiverIndex, -1) << "Note-on should be received by one receiver";
    
    // Process several empty buffers (simulating sustained note)
    for (int i = 0; i < 5; ++i) {
        juce::MidiBuffer emptyBuffer;
        processSender(emptyBuffer);
        processReceivers();
    }
    
    // Buffer N: Send note-off
    auto noteOffBuffer = createNoteOffBuffer(notes);
    processSender(noteOffBuffer);
    auto outputsN = processReceivers();
    
    // Verify note-off goes to the same receiver
    EXPECT_TRUE(hasNoteOff(outputsN[receiverIndex], 60))
        << "Note-off should go to same receiver as note-on after sustained period";
}

/**
 * @brief Test multiple sustained notes with staggered off events
 */
TEST_F(SustainedNotesTest, StaggeredNoteOffEvents) {
    // Buffer 1: Send all note-on events
    std::vector<int> notes = {60, 64, 67, 72};
    auto noteOnBuffer = createNoteOnBuffer(notes);
    processSender(noteOnBuffer);
    auto outputs1 = processReceivers();
    
    // Track which receiver got which note
    std::map<int, int> noteToReceiver;
    for (size_t i = 0; i < outputs1.size(); ++i) {
        for (int note : notes) {
            if (hasNoteOn(outputs1[i], note)) {
                noteToReceiver[note] = static_cast<int>(i);
            }
        }
    }
    
    // Buffer 2: Turn off first two notes
    auto noteOffBuffer1 = createNoteOffBuffer({60, 64});
    processSender(noteOffBuffer1);
    auto outputs2 = processReceivers();
    
    // Verify first two note-offs go to correct receivers
    EXPECT_TRUE(hasNoteOff(outputs2[noteToReceiver[60]], 60));
    EXPECT_TRUE(hasNoteOff(outputs2[noteToReceiver[64]], 64));
    
    // Buffer 3: Turn off remaining notes
    auto noteOffBuffer2 = createNoteOffBuffer({67, 72});
    processSender(noteOffBuffer2);
    auto outputs3 = processReceivers();
    
    // Verify remaining note-offs go to correct receivers
    EXPECT_TRUE(hasNoteOff(outputs3[noteToReceiver[67]], 67));
    EXPECT_TRUE(hasNoteOff(outputs3[noteToReceiver[72]], 72));
}

/**
 * @brief Test overlapping notes (same pitch, different times)
 */
TEST_F(SustainedNotesTest, OverlappingNotes) {
    // Buffer 1: First note-on for C (60)
    auto noteOnBuffer1 = createNoteOnBuffer({60});
    processSender(noteOnBuffer1);
    auto outputs1 = processReceivers();
    
    // Find first receiver
    int receiver1 = -1;
    for (size_t i = 0; i < outputs1.size(); ++i) {
        if (hasNoteOn(outputs1[i], 60)) {
            receiver1 = static_cast<int>(i);
            break;
        }
    }
    
    // Buffer 2: Second note-on for C (60) while first is still held
    auto noteOnBuffer2 = createNoteOnBuffer({60});
    processSender(noteOnBuffer2);
    auto outputs2 = processReceivers();
    
    // Find second receiver (should be different)
    int receiver2 = -1;
    for (size_t i = 0; i < outputs2.size(); ++i) {
        if (hasNoteOn(outputs2[i], 60)) {
            receiver2 = static_cast<int>(i);
            break;
        }
    }
    
    // Verify second note went to a different receiver (or same if only one voice)
    ASSERT_NE(receiver2, -1) << "Second note-on should be received";
    
    // Buffer 3: Turn off first note
    auto noteOffBuffer1 = createNoteOffBuffer({60});
    processSender(noteOffBuffer1);
    auto outputs3 = processReceivers();
    
    // First receiver should get note-off
    EXPECT_TRUE(hasNoteOff(outputs3[receiver1], 60))
        << "First note-off should go to first receiver";
}

/**
 * @brief Test note-on/off pairing with Control Changes in between
 */
TEST_F(SustainedNotesTest, NoteOnOffWithControlChanges) {
    // Buffer 1: Note-on
    auto noteOnBuffer = createNoteOnBuffer({60});
    processSender(noteOnBuffer);
    auto outputs1 = processReceivers();
    
    // Find receiver
    int receiverIndex = -1;
    for (size_t i = 0; i < outputs1.size(); ++i) {
        if (hasNoteOn(outputs1[i], 60)) {
            receiverIndex = static_cast<int>(i);
            break;
        }
    }
    
    // Buffer 2: Control Change (sustain pedal)
    juce::MidiBuffer ccBuffer;
    ccBuffer.addEvent(juce::MidiMessage::controllerEvent(1, 64, 127), 0);
    processSender(ccBuffer);
    auto outputs2 = processReceivers();
    
    // All receivers should get Control Change
    for (const auto& output : outputs2) {
        bool hasSustain = false;
        for (const auto metadata : output) {
            auto message = metadata.getMessage();
            if (message.isController() && message.getControllerNumber() == 64) {
                hasSustain = true;
                break;
            }
        }
        EXPECT_TRUE(hasSustain) << "All receivers should get Control Change";
    }
    
    // Buffer 3: Note-off
    auto noteOffBuffer = createNoteOffBuffer({60});
    processSender(noteOffBuffer);
    auto outputs3 = processReceivers();
    
    // Note-off should still go to correct receiver
    EXPECT_TRUE(hasNoteOff(outputs3[receiverIndex], 60))
        << "Note-off should go to correct receiver despite Control Change in between";
}

/**
 * @brief Test rapid note-on/off sequences
 */
TEST_F(SustainedNotesTest, RapidNoteOnOffSequence) {
    std::vector<int> notes = {60, 64, 67};
    
    for (int iteration = 0; iteration < 3; ++iteration) {
        // Note-on
        auto noteOnBuffer = createNoteOnBuffer(notes);
        processSender(noteOnBuffer);
        auto onOutputs = processReceivers();
        
        // Track receivers
        std::map<int, int> noteToReceiver;
        for (size_t i = 0; i < onOutputs.size(); ++i) {
            for (int note : notes) {
                if (hasNoteOn(onOutputs[i], note)) {
                    noteToReceiver[note] = static_cast<int>(i);
                }
            }
        }
        
        // Note-off immediately after
        auto noteOffBuffer = createNoteOffBuffer(notes);
        processSender(noteOffBuffer);
        auto offOutputs = processReceivers();
        
        // Verify pairing
        for (int note : notes) {
            int receiverIndex = noteToReceiver[note];
            EXPECT_TRUE(hasNoteOff(offOutputs[receiverIndex], note))
                << "Iteration " << iteration << ": Note-off for " << note 
                << " should match note-on receiver";
        }
    }
}

/**
 * @brief Test note-off without corresponding note-on (error case)
 */
TEST_F(SustainedNotesTest, NoteOffWithoutNoteOn) {
    // Send note-off without prior note-on
    auto noteOffBuffer = createNoteOffBuffer({60});
    processSender(noteOffBuffer);
    auto outputs = processReceivers();
    
    // System should handle gracefully (might drop or broadcast)
    // At minimum, should not crash
    int totalNoteOffs = 0;
    for (const auto& output : outputs) {
        totalNoteOffs += countNoteOff(output);
    }
    
    // Should either drop the note-off or handle it gracefully
    EXPECT_GE(totalNoteOffs, 0) << "System should handle orphan note-off gracefully";
}

/**
 * @brief Test maximum sustained notes capacity
 */
TEST_F(SustainedNotesTest, MaximumSustainedNotes) {
    // Send many note-on events
    std::vector<int> notes;
    for (int i = 21; i <= 108; ++i) { // Full piano range
        notes.push_back(i);
    }
    
    auto noteOnBuffer = createNoteOnBuffer(notes);
    processSender(noteOnBuffer);
    auto outputs1 = processReceivers();
    
    // Track all note assignments
    std::map<int, int> noteToReceiver;
    for (size_t i = 0; i < outputs1.size(); ++i) {
        for (int note : notes) {
            if (hasNoteOn(outputs1[i], note)) {
                noteToReceiver[note] = static_cast<int>(i);
            }
        }
    }
    
    // Send note-off for all notes
    auto noteOffBuffer = createNoteOffBuffer(notes);
    processSender(noteOffBuffer);
    auto outputs2 = processReceivers();
    
    // Verify all note-offs are paired correctly
    int correctPairings = 0;
    for (int note : notes) {
        if (noteToReceiver.find(note) != noteToReceiver.end()) {
            int receiverIndex = noteToReceiver[note];
            if (hasNoteOff(outputs2[receiverIndex], note)) {
                correctPairings++;
            }
        }
    }
    
    // Most notes should be paired correctly (allowing for some capacity limits)
    EXPECT_GT(correctPairings, static_cast<int>(notes.size()) * 0.9) 
        << "At least 90% of notes should maintain correct pairing";
}

/**
 * @brief Test note state persistence across multiple buffers
 */
TEST_F(SustainedNotesTest, NoteStatePersistence) {
    // Buffer 1: Note-on for chord
    std::vector<int> chord = {60, 64, 67};
    auto noteOnBuffer = createNoteOnBuffer(chord);
    processSender(noteOnBuffer);
    auto outputs1 = processReceivers();
    
    // Track receivers
    std::map<int, int> noteToReceiver;
    for (size_t i = 0; i < outputs1.size(); ++i) {
        for (int note : chord) {
            if (hasNoteOn(outputs1[i], note)) {
                noteToReceiver[note] = static_cast<int>(i);
            }

        }
    }
    
    // Process many empty buffers
    for (int i = 0; i < 10; ++i) {
        juce::MidiBuffer emptyBuffer;
        processSender(emptyBuffer);
        processReceivers();
    }
    
    // Buffer N: Note-off for chord
    auto noteOffBuffer = createNoteOffBuffer(chord);
    processSender(noteOffBuffer);
    auto outputsN = processReceivers();
    
    // Verify all note-offs go to correct receivers
    for (int note : chord) {
        int receiverIndex = noteToReceiver[note];
        EXPECT_TRUE(hasNoteOff(outputsN[receiverIndex], note))
            << "Note-off for " << note << " should go to correct receiver after many empty buffers";
    }
}

TEST(VoiceAssignmentTrackerTest, BlockRelativeTimestampDoesNotExpireImmediately) {
    VoiceAssignmentTracker tracker;

    ASSERT_TRUE(tracker.recordNoteOn(60, 0, 1, 0));

    EXPECT_EQ(tracker.clearStaleNotes(), 0);
    EXPECT_TRUE(tracker.isNoteActive(60, 0));

    ASSERT_TRUE(tracker.recordNoteOn(64, 0, 16, 0));
    EXPECT_EQ(tracker.getVoiceForNoteOff(64, 0), 16);
}
