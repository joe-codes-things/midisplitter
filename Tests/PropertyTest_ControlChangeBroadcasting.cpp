#include <gtest/gtest.h>
#include "../Source/SenderPipeline.h"
#include "../Source/ReceiverPipeline.h"
#include "../Source/MIDIEvent.h"
#include "../Source/PluginConfiguration.h"
#include "../Source/PluginController.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <random>

/**
 * @brief Property Test 17: Control Change Message Broadcasting
 * 
 * Validates Requirements 7.4, 7.5:
 * - Control Change messages are broadcast to all receiver instances
 * - Control Change messages preserve their original channel and value
 * - All receivers receive identical Control Change messages
 */
class ControlChangeBroadcastingPropertyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize configuration
        config_ = PluginConfiguration::getDefault();
        config_.bufferSizeMs = 5; // Minimum buffer for fast testing
        
        // Create sender pipeline
        senderPipeline_ = std::make_unique<SenderPipeline>(config_, PluginController::PluginFormat::VST3);
        
        // Initialize with voice count
        voiceCount_ = 4;
        senderPipeline_->initialize(voiceCount_);
        
        // Create receiver pipelines
        for (int i = 1; i <= voiceCount_; ++i) {
            auto receiver = std::make_unique<ReceiverPipeline>(config_, PluginController::PluginFormat::VST3);
            receiver->initialize(i); // Voice number 1-4
            receiverPipelines_.push_back(std::move(receiver));
        }
        
        // Setup random number generator
        rng_.seed(std::random_device{}());
    }
    
    void TearDown() override {
        receiverPipelines_.clear();
        senderPipeline_.reset();
    }
    
    /**
     * @brief Generate random Control Change event
     * @return Random CC event
     */
    MIDIEvent generateRandomControlChange() {
        std::uniform_int_distribution<int> channelDist(0, 15);
        std::uniform_int_distribution<int> controllerDist(0, 127);
        std::uniform_int_distribution<int> valueDist(0, 127);
        
        MIDIEvent event;
        event.status = 0xB0 | channelDist(rng_); // Control Change + channel
        event.note = controllerDist(rng_);        // Controller number
        event.velocity = valueDist(rng_);         // Controller value
        event.channel = event.status & 0x0F;
        event.timestamp = juce::Time::getMillisecondCounterHiRes();
        
        return event;
    }
    
    /**
     * @brief Generate random Note On event for mixing with CC
     * @return Random Note On event
     */
    MIDIEvent generateRandomNoteOn() {
        std::uniform_int_distribution<int> channelDist(0, 15);
        std::uniform_int_distribution<int> noteDist(21, 108); // Piano range
        std::uniform_int_distribution<int> velocityDist(1, 127);
        
        MIDIEvent event;
        event.status = 0x90 | channelDist(rng_); // Note On + channel
        event.note = noteDist(rng_);
        event.velocity = velocityDist(rng_);
        event.channel = event.status & 0x0F;
        event.timestamp = juce::Time::getMillisecondCounterHiRes();
        
        return event;
    }
    
    /**
     * @brief Process events through sender and collect receiver outputs
     * @param inputEvents Events to process
     * @return Vector of receiver outputs (one per receiver)
     */
    std::vector<std::vector<MIDIEvent>> processEventsAndCollectOutputs(
        const std::vector<MIDIEvent>& inputEvents) {
        
        // Create MIDI buffer with input events
        juce::MidiBuffer inputBuffer;
        for (const auto& event : inputEvents) {
            juce::MidiMessage message(event.status, event.note, event.velocity);
            inputBuffer.addEvent(message, 0);
        }
        
        // Process through sender
        senderPipeline_->processBlock(inputBuffer, 512, 44100.0);
        
        // Wait for IPC transmission (simulate network delay)
        juce::Thread::sleep(10);
        
        // Collect outputs from all receivers
        std::vector<std::vector<MIDIEvent>> receiverOutputs;
        
        for (auto& receiver : receiverPipelines_) {
            juce::MidiBuffer outputBuffer;
            receiver->processBlock(outputBuffer, 512, 44100.0);
            
            // Convert output buffer to events
            std::vector<MIDIEvent> receiverEvents;
            for (const auto metadata : outputBuffer) {
                auto message = metadata.getMessage();
                
                MIDIEvent event;
                event.status = message.getRawData()[0];
                if (message.isController()) {
                    event.note = static_cast<uint8_t>(message.getControllerNumber());
                    event.velocity = static_cast<uint8_t>(message.getControllerValue());
                } else {
                    event.note = static_cast<uint8_t>(message.getNoteNumber());
                    event.velocity = message.getVelocity();
                }
                event.channel = message.getChannel() - 1; // JUCE uses 1-based channels
                event.timestamp = static_cast<double>(metadata.samplePosition);
                
                receiverEvents.push_back(event);
            }
            
            receiverOutputs.push_back(receiverEvents);
        }
        
        return receiverOutputs;
    }
    
    /**
     * @brief Check if event is Control Change
     * @param event Event to check
     * @return true if CC event
     */
    bool isControlChange(const MIDIEvent& event) {
        return (event.status & 0xF0) == 0xB0;
    }
    
    /**
     * @brief Check if two CC events are identical
     * @param event1 First event
     * @param event2 Second event
     * @return true if identical
     */
    bool areControlChangesIdentical(const MIDIEvent& event1, const MIDIEvent& event2) {
        return event1.status == event2.status &&
               event1.note == event2.note &&     // Controller number
               event1.velocity == event2.velocity && // Controller value
               event1.channel == event2.channel;
    }

protected:
    PluginConfiguration config_;
    std::unique_ptr<SenderPipeline> senderPipeline_;
    std::vector<std::unique_ptr<ReceiverPipeline>> receiverPipelines_;
    int voiceCount_;
    std::mt19937 rng_;
};

/**
 * @brief Property: Single Control Change is broadcast to all receivers
 */
TEST_F(ControlChangeBroadcastingPropertyTest, SingleControlChangeIsBroadcastToAllReceivers) {
    for (int iteration = 0; iteration < 100; ++iteration) {
        // Generate random Control Change event
        MIDIEvent ccEvent = generateRandomControlChange();
        std::vector<MIDIEvent> inputEvents = {ccEvent};
        
        // Process through pipeline
        auto receiverOutputs = processEventsAndCollectOutputs(inputEvents);
        
        // Verify all receivers got the Control Change
        for (size_t receiverIndex = 0; receiverIndex < receiverOutputs.size(); ++receiverIndex) {
            const auto& receiverEvents = receiverOutputs[receiverIndex];
            
            // Find Control Change events in receiver output
            std::vector<MIDIEvent> receivedCCs;
            for (const auto& event : receiverEvents) {
                if (isControlChange(event)) {
                    receivedCCs.push_back(event);
                }
            }
            
            // Should have exactly one Control Change
            ASSERT_EQ(receivedCCs.size(), 1) 
                << "Receiver " << receiverIndex << " should receive exactly one Control Change event";
            
            // Control Change should be identical to original
            EXPECT_TRUE(areControlChangesIdentical(ccEvent, receivedCCs[0]))
                << "Receiver " << receiverIndex << " received modified Control Change event";
        }
        
        // Verify all receivers got identical Control Changes
        if (receiverOutputs.size() > 1) {
            const auto& firstReceiverCCs = receiverOutputs[0];
            
            for (size_t i = 1; i < receiverOutputs.size(); ++i) {
                const auto& currentReceiverCCs = receiverOutputs[i];
                
                // Find CC events in both outputs
                std::vector<MIDIEvent> firstCCs, currentCCs;
                
                for (const auto& event : firstReceiverCCs) {
                    if (isControlChange(event)) {
                        firstCCs.push_back(event);
                    }
                }
                
                for (const auto& event : currentReceiverCCs) {
                    if (isControlChange(event)) {
                        currentCCs.push_back(event);
                    }
                }
                
                ASSERT_EQ(firstCCs.size(), currentCCs.size())
                    << "All receivers should have same number of Control Change events";
                
                for (size_t j = 0; j < firstCCs.size(); ++j) {
                    EXPECT_TRUE(areControlChangesIdentical(firstCCs[j], currentCCs[j]))
                        << "Control Change " << j << " should be identical across all receivers";
                }
            }
        }
    }
}

/**
 * @brief Property: Multiple Control Changes are all broadcast
 */
TEST_F(ControlChangeBroadcastingPropertyTest, MultipleControlChangesAreBroadcast) {
    for (int iteration = 0; iteration < 50; ++iteration) {
        // Generate multiple Control Change events
        std::uniform_int_distribution<int> countDist(2, 8);
        int ccCount = countDist(rng_);
        
        std::vector<MIDIEvent> inputEvents;
        std::vector<MIDIEvent> originalCCs;
        
        for (int i = 0; i < ccCount; ++i) {
            MIDIEvent ccEvent = generateRandomControlChange();
            inputEvents.push_back(ccEvent);
            originalCCs.push_back(ccEvent);
        }
        
        // Process through pipeline
        auto receiverOutputs = processEventsAndCollectOutputs(inputEvents);
        
        // Verify all receivers got all Control Changes
        for (size_t receiverIndex = 0; receiverIndex < receiverOutputs.size(); ++receiverIndex) {
            const auto& receiverEvents = receiverOutputs[receiverIndex];
            
            // Find Control Change events in receiver output
            std::vector<MIDIEvent> receivedCCs;
            for (const auto& event : receiverEvents) {
                if (isControlChange(event)) {
                    receivedCCs.push_back(event);
                }
            }
            
            // Should have all Control Changes
            EXPECT_EQ(receivedCCs.size(), originalCCs.size())
                << "Receiver " << receiverIndex << " should receive all Control Change events";
            
            // Each Control Change should match an original (order may vary)
            for (const auto& receivedCC : receivedCCs) {
                bool foundMatch = false;
                for (const auto& originalCC : originalCCs) {
                    if (areControlChangesIdentical(receivedCC, originalCC)) {
                        foundMatch = true;
                        break;
                    }
                }
                EXPECT_TRUE(foundMatch)
                    << "Receiver " << receiverIndex << " received unrecognized Control Change";
            }
        }
    }
}

/**
 * @brief Property: Control Changes mixed with Note events are properly separated
 */
TEST_F(ControlChangeBroadcastingPropertyTest, ControlChangesWithNotesAreSeparated) {
    for (int iteration = 0; iteration < 50; ++iteration) {
        // Generate mixed events (Notes and Control Changes)
        std::uniform_int_distribution<int> eventCountDist(3, 10);
        int eventCount = eventCountDist(rng_);
        
        std::vector<MIDIEvent> inputEvents;
        std::vector<MIDIEvent> originalCCs;
        std::vector<MIDIEvent> originalNotes;
        
        for (int i = 0; i < eventCount; ++i) {
            std::uniform_int_distribution<int> typeDist(0, 1);
            
            if (typeDist(rng_) == 0) {
                // Add Control Change
                MIDIEvent ccEvent = generateRandomControlChange();
                inputEvents.push_back(ccEvent);
                originalCCs.push_back(ccEvent);
            } else {
                // Add Note On
                MIDIEvent noteEvent = generateRandomNoteOn();
                inputEvents.push_back(noteEvent);
                originalNotes.push_back(noteEvent);
            }
        }
        
        // Skip if no Control Changes
        if (originalCCs.empty()) {
            continue;
        }
        
        // Process through pipeline
        auto receiverOutputs = processEventsAndCollectOutputs(inputEvents);
        
        // Verify Control Changes are broadcast to all receivers
        for (size_t receiverIndex = 0; receiverIndex < receiverOutputs.size(); ++receiverIndex) {
            const auto& receiverEvents = receiverOutputs[receiverIndex];
            
            // Find Control Change events in receiver output
            std::vector<MIDIEvent> receivedCCs;
            for (const auto& event : receiverEvents) {
                if (isControlChange(event)) {
                    receivedCCs.push_back(event);
                }
            }
            
            // Should have all original Control Changes
            EXPECT_EQ(receivedCCs.size(), originalCCs.size())
                << "Receiver " << receiverIndex << " should receive all Control Change events";
            
            // Each received CC should match an original
            for (const auto& receivedCC : receivedCCs) {
                bool foundMatch = false;
                for (const auto& originalCC : originalCCs) {
                    if (areControlChangesIdentical(receivedCC, originalCC)) {
                        foundMatch = true;
                        break;
                    }
                }
                EXPECT_TRUE(foundMatch)
                    << "Receiver " << receiverIndex << " received unrecognized Control Change";
            }
        }
        
        // Verify Note events are distributed (not broadcast)
        if (!originalNotes.empty()) {
            // Count total Note events across all receivers
            int totalReceivedNotes = 0;
            for (const auto& receiverEvents : receiverOutputs) {
                for (const auto& event : receiverEvents) {
                    if ((event.status & 0xF0) == 0x90) { // Note On
                        totalReceivedNotes++;
                    }
                }
            }
            
            // Should equal original note count (distributed, not broadcast)
            EXPECT_EQ(totalReceivedNotes, static_cast<int>(originalNotes.size()))
                << "Note events should be distributed, not broadcast";
        }
    }
}

/**
 * @brief Property: Control Change channel preservation
 */
TEST_F(ControlChangeBroadcastingPropertyTest, ControlChangeChannelIsPreserved) {
    for (int iteration = 0; iteration < 100; ++iteration) {
        // Generate Control Change on specific channel
        std::uniform_int_distribution<int> channelDist(0, 15);
        int originalChannel = channelDist(rng_);
        
        MIDIEvent ccEvent = generateRandomControlChange();
        ccEvent.status = 0xB0 | originalChannel; // Force specific channel
        ccEvent.channel = originalChannel;
        
        std::vector<MIDIEvent> inputEvents = {ccEvent};
        
        // Process through pipeline
        auto receiverOutputs = processEventsAndCollectOutputs(inputEvents);
        
        // Verify channel preservation in all receivers
        for (size_t receiverIndex = 0; receiverIndex < receiverOutputs.size(); ++receiverIndex) {
            const auto& receiverEvents = receiverOutputs[receiverIndex];
            
            for (const auto& event : receiverEvents) {
                if (isControlChange(event)) {
                    EXPECT_EQ(event.channel, originalChannel)
                        << "Receiver " << receiverIndex << " should preserve Control Change channel";
                    EXPECT_EQ(event.status & 0x0F, originalChannel)
                        << "Receiver " << receiverIndex << " should preserve Control Change status channel";
                }
            }
        }
    }
}

/**
 * @brief Property: Sustain pedal (CC 64) broadcasting
 */
TEST_F(ControlChangeBroadcastingPropertyTest, SustainPedalIsBroadcast) {
    for (int iteration = 0; iteration < 50; ++iteration) {
        // Generate sustain pedal Control Change (CC 64)
        std::uniform_int_distribution<int> channelDist(0, 15);
        std::uniform_int_distribution<int> valueDist(0, 127);
        
        MIDIEvent sustainEvent;
        sustainEvent.status = 0xB0 | channelDist(rng_);
        sustainEvent.note = 64; // Sustain pedal controller
        sustainEvent.velocity = valueDist(rng_);
        sustainEvent.channel = sustainEvent.status & 0x0F;
        sustainEvent.timestamp = juce::Time::getMillisecondCounterHiRes();
        
        std::vector<MIDIEvent> inputEvents = {sustainEvent};
        
        // Process through pipeline
        auto receiverOutputs = processEventsAndCollectOutputs(inputEvents);
        
        // Verify sustain pedal is broadcast to all receivers
        for (size_t receiverIndex = 0; receiverIndex < receiverOutputs.size(); ++receiverIndex) {
            const auto& receiverEvents = receiverOutputs[receiverIndex];
            
            // Find sustain pedal events
            bool foundSustain = false;
            for (const auto& event : receiverEvents) {
                if (isControlChange(event) && event.note == 64) {
                    foundSustain = true;
                    EXPECT_TRUE(areControlChangesIdentical(sustainEvent, event))
                        << "Receiver " << receiverIndex << " should receive identical sustain pedal event";
                }
            }
            
            EXPECT_TRUE(foundSustain)
                << "Receiver " << receiverIndex << " should receive sustain pedal Control Change";
        }
    }
}
