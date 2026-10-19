#include <gtest/gtest.h>
#include "../source/IPCManagerVST3_Socket.h"
#include "../source/SenderPipeline.h"
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <vector>

TEST(VST3IPCManagerTest, DeliversQueuedMidiEventsOnceToTheSelectedVoice)
{
    const auto sessionId = "vst3-ipc-test-" +
        juce::String(juce::Time::getMillisecondCounter()) + "-" +
        juce::String(juce::Random::getSystemRandom().nextInt());

    IPCManagerVST3_Socket sender;
    sender.setMode(true);
    ASSERT_TRUE(sender.startServer(sessionId));

    std::mutex eventsMutex;
    std::condition_variable eventsReceived;
    std::vector<MIDIEvent> receivedEvents;

    IPCManagerVST3_Socket receiver;
    receiver.setMode(false);
    receiver.setVoiceNumber(3);
    receiver.setReceiveCallback([&](const std::vector<MIDIEvent>& events, uint8_t voiceNumber) {
        if (voiceNumber != 3)
            return;

        {
            std::lock_guard<std::mutex> lock(eventsMutex);
            receivedEvents.insert(receivedEvents.end(), events.begin(), events.end());
        }
        eventsReceived.notify_one();
    });
    ASSERT_TRUE(receiver.connectToServer(sessionId));

    VoiceAssignment assignment(3);
    assignment.notes.emplace_back(0x90, 60, 100, 0, 128);
    assignment.notes.emplace_back(0x80, 60, 0, 0, 640);
    ASSERT_TRUE(sender.sendVoiceData({ assignment }));

    {
        std::unique_lock<std::mutex> lock(eventsMutex);
        ASSERT_TRUE(eventsReceived.wait_for(lock, std::chrono::seconds(2), [&] {
            return receivedEvents.size() == 2;
        }));
    }

    std::lock_guard<std::mutex> lock(eventsMutex);
    ASSERT_EQ(receivedEvents.size(), 2u);
    EXPECT_EQ(receivedEvents[0].status, 0x90);
    EXPECT_EQ(receivedEvents[0].note, 60);
    EXPECT_EQ(receivedEvents[0].velocity, 100);
    EXPECT_EQ(receivedEvents[0].timestamp, 128);
    EXPECT_EQ(receivedEvents[1].status, 0x80);
    EXPECT_EQ(receivedEvents[1].timestamp, 640);
}

TEST(VST3SenderPipelineTest, SendsDistributedMidiToRemoteReceiver)
{
    PluginConfiguration configuration = PluginConfiguration::getDefault();
    SenderPipeline sender(configuration, PluginController::PluginFormat::VST3);
    ASSERT_TRUE(sender.initialize(4));

    std::mutex eventsMutex;
    std::condition_variable eventsReceived;
    std::vector<MIDIEvent> receivedEvents;

    IPCManagerVST3_Socket receiver;
    receiver.setMode(false);
    receiver.setVoiceNumber(1);
    receiver.setReceiveCallback([&](const std::vector<MIDIEvent>& events, uint8_t voiceNumber) {
        if (voiceNumber != 1)
            return;

        {
            std::lock_guard<std::mutex> lock(eventsMutex);
            receivedEvents.insert(receivedEvents.end(), events.begin(), events.end());
        }
        eventsReceived.notify_one();
    });
    ASSERT_TRUE(receiver.connectToServer(IPCManagerVST3_Socket::EXPERIMENT_SESSION_ID));

    juce::MidiBuffer input;
    input.addEvent(juce::MidiMessage::noteOn(1, 64, static_cast<uint8_t>(100)), 0);
    const auto assignments = sender.processBlockAndGetAssignments(input, 512, 44100.0);

    ASSERT_EQ(assignments.size(), 4u);
    ASSERT_EQ(assignments.front().notes.size(), 1u);

    {
        std::unique_lock<std::mutex> lock(eventsMutex);
        ASSERT_TRUE(eventsReceived.wait_for(lock, std::chrono::seconds(2), [&] {
            return receivedEvents.size() == 1;
        }));
    }

    std::lock_guard<std::mutex> lock(eventsMutex);
    ASSERT_EQ(receivedEvents.size(), 1u);
    EXPECT_TRUE(receivedEvents.front().isNoteOn());
    EXPECT_EQ(receivedEvents.front().note, 64);
    EXPECT_EQ(receivedEvents.front().velocity, 100);
}
