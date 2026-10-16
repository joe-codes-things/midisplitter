#include "../Source/IPCMessage.h"
#include <gtest/gtest.h>

/**
 * @brief Unit tests for IPCMessage structure
 * 
 * Tests verify the basic requirements from task 2.7:
 * - IPCMessage struct with magic, version, voiceNumber, eventCount, events fields
 * - calculateSize() method
 * - Protocol constants (MAGIC = 0x4D494449, VERSION = 1)
 * 
 * Requirements: 8.4, 8.7, 9.4
 */

TEST(IPCMessageTest, ProtocolConstantsMagic) {
    EXPECT_EQ(IPCMessage::MAGIC, 0x4D494449);
}

TEST(IPCMessageTest, ProtocolConstantsVersion) {
    EXPECT_EQ(IPCMessage::VERSION, 1);
}

TEST(IPCMessageTest, HasMagicField) {
    IPCMessage msg(3);
    EXPECT_TRUE(msg.isValid()); // isValid checks magic
}

TEST(IPCMessageTest, HasVersionField) {
    IPCMessage msg(3);
    EXPECT_TRUE(msg.isValid()); // isValid checks version
}

TEST(IPCMessageTest, HasVoiceNumberField) {
    IPCMessage msg(3);
    EXPECT_EQ(msg.getVoiceNumber(), 3);
    
    msg.setVoiceNumber(5);
    EXPECT_EQ(msg.getVoiceNumber(), 5);
}

TEST(IPCMessageTest, HasEventCountField) {
    IPCMessage msg(3);
    EXPECT_EQ(msg.getEventCount(), 0);
    
    MIDIEvent event(0x90, 60, 100, 0, 1000);
    msg.addEvent(event);
    
    EXPECT_EQ(msg.getEventCount(), 1);
}

TEST(IPCMessageTest, HasEventsField) {
    IPCMessage msg(3);
    MIDIEvent event1(0x90, 60, 100, 0, 1000);
    MIDIEvent event2(0x90, 64, 100, 0, 1000);
    
    msg.addEvent(event1);
    msg.addEvent(event2);
    
    const auto& events = msg.getEvents();
    EXPECT_EQ(events.size(), 2);
    EXPECT_EQ(events[0].note, 60);
    EXPECT_EQ(events[1].note, 64);
}

TEST(IPCMessageTest, CalculateSizeEmptyMessage) {
    IPCMessage msg(1);
    size_t emptySize = msg.calculateSize();
    EXPECT_EQ(emptySize, sizeof(IPCMessage::Header));
}

TEST(IPCMessageTest, CalculateSizeIncreasesWithEvents) {
    IPCMessage msg(1);
    size_t initialSize = msg.calculateSize();
    
    MIDIEvent event(0x90, 60, 100, 0, 1000);
    msg.addEvent(event);
    
    size_t sizeWithOneEvent = msg.calculateSize();
    EXPECT_EQ(sizeWithOneEvent, initialSize + sizeof(MIDIEvent));
    
    msg.addEvent(event);
    size_t sizeWithTwoEvents = msg.calculateSize();
    EXPECT_EQ(sizeWithTwoEvents, initialSize + 2 * sizeof(MIDIEvent));
}

TEST(IPCMessageTest, CalculateSizeMatchesSerializedSize) {
    IPCMessage msg(1);
    MIDIEvent event1(0x90, 60, 100, 0, 1000);
    MIDIEvent event2(0x90, 64, 100, 0, 1000);
    MIDIEvent event3(0x90, 67, 100, 0, 1000);
    
    msg.addEvent(event1);
    msg.addEvent(event2);
    msg.addEvent(event3);
    
    size_t calculatedSize = msg.calculateSize();
    
    // Allocate buffer and serialize
    std::vector<uint8_t> buffer(calculatedSize);
    size_t serializedSize = msg.serialize(buffer.data());
    
    EXPECT_EQ(serializedSize, calculatedSize);
}

TEST(IPCMessageTest, DefaultConstructorInitializesVoice1) {
    IPCMessage msg;
    EXPECT_EQ(msg.getVoiceNumber(), 1);
    EXPECT_EQ(msg.getEventCount(), 0);
    EXPECT_TRUE(msg.isEmpty());
}

TEST(IPCMessageTest, ConstructorWithVoiceNumberSetsVoice) {
    for (uint32_t voice = 1; voice <= 8; ++voice) {
        IPCMessage msg(voice);
        EXPECT_EQ(msg.getVoiceNumber(), voice);
    }
}

TEST(IPCMessageTest, AddingEventsUpdatesEventCount) {
    IPCMessage msg(2);
    
    for (int i = 0; i < 5; ++i) {
        MIDIEvent event(0x90, 60 + i, 100, 0, 1000 * i);
        EXPECT_TRUE(msg.addEvent(event));
        EXPECT_EQ(msg.getEventCount(), i + 1);
    }
}

TEST(IPCMessageTest, ClearEventsResetsMessage) {
    IPCMessage msg(3);
    
    MIDIEvent event(0x90, 60, 100, 0, 1000);
    msg.addEvent(event);
    msg.addEvent(event);
    
    EXPECT_EQ(msg.getEventCount(), 2);
    EXPECT_FALSE(msg.isEmpty());
    
    msg.clearEvents();
    
    EXPECT_EQ(msg.getEventCount(), 0);
    EXPECT_TRUE(msg.isEmpty());
}

TEST(IPCMessageTest, RoundTripSerializationPreservesData) {
    IPCMessage originalMsg(4);
    
    // Add some events
    MIDIEvent event1(0x90, 60, 100, 0, 1000);
    MIDIEvent event2(0x90, 64, 110, 1, 2000);
    MIDIEvent event3(0x80, 60, 0, 0, 3000);
    
    originalMsg.addEvent(event1);
    originalMsg.addEvent(event2);
    originalMsg.addEvent(event3);
    
    // Serialize
    size_t bufferSize = originalMsg.calculateSize();
    std::vector<uint8_t> buffer(bufferSize);
    size_t serializedSize = originalMsg.serialize(buffer.data());
    
    EXPECT_EQ(serializedSize, bufferSize);
    
    // Deserialize
    IPCMessage deserializedMsg;
    bool success = deserializedMsg.deserialize(buffer.data(), bufferSize);
    
    EXPECT_TRUE(success);
    EXPECT_EQ(deserializedMsg.getVoiceNumber(), originalMsg.getVoiceNumber());
    EXPECT_EQ(deserializedMsg.getEventCount(), originalMsg.getEventCount());
    
    const auto& originalEvents = originalMsg.getEvents();
    const auto& deserializedEvents = deserializedMsg.getEvents();
    
    EXPECT_EQ(deserializedEvents.size(), originalEvents.size());
    
    for (size_t i = 0; i < originalEvents.size(); ++i) {
        EXPECT_EQ(deserializedEvents[i].status, originalEvents[i].status);
        EXPECT_EQ(deserializedEvents[i].note, originalEvents[i].note);
        EXPECT_EQ(deserializedEvents[i].velocity, originalEvents[i].velocity);
        EXPECT_EQ(deserializedEvents[i].channel, originalEvents[i].channel);
        EXPECT_EQ(deserializedEvents[i].timestamp, originalEvents[i].timestamp);
    }
}

TEST(IPCMessageTest, ValidMessagePassesValidation) {
    IPCMessage msg(5);
    MIDIEvent event(0x90, 60, 100, 0, 1000);
    msg.addEvent(event);
    
    EXPECT_TRUE(msg.isValid());
}

TEST(IPCMessageTest, EmptyMessageIsValid) {
    IPCMessage msg(1);
    EXPECT_TRUE(msg.isValid());
}

TEST(IPCMessageTest, MessageWithMaximumEventsIsValid) {
    IPCMessage msg(1);
    
    for (size_t i = 0; i < IPCMessage::getMaxEventsPerMessage(); ++i) {
        MIDIEvent event(0x90, 60, 100, 0, i * 1000);
        if (!msg.addEvent(event)) {
            break; // Message full
        }
    }
    
    EXPECT_TRUE(msg.isValid());
}

TEST(IPCMessageTest, CannotAddMoreThanMaxEvents) {
    IPCMessage msg(1);
    
    size_t maxEvents = IPCMessage::getMaxEventsPerMessage();
    
    // Fill to capacity
    for (size_t i = 0; i < maxEvents; ++i) {
        MIDIEvent event(0x90, 60, 100, 0, i * 1000);
        EXPECT_TRUE(msg.addEvent(event));
    }
    
    // Try to add one more
    MIDIEvent extraEvent(0x90, 60, 100, 0, 999999);
    EXPECT_FALSE(msg.addEvent(extraEvent));
    
    EXPECT_EQ(msg.getEventCount(), maxEvents);
}

TEST(IPCMessageTest, VoiceNumberRangeIsValidated) {
    IPCMessage msg(1);
    
    // Valid voice numbers (1-8)
    for (uint32_t voice = 1; voice <= 8; ++voice) {
        msg.setVoiceNumber(voice);
        EXPECT_TRUE(msg.isValid());
    }
    
    // Invalid voice numbers
    msg.setVoiceNumber(0);
    EXPECT_FALSE(msg.isValid());
    
    msg.setVoiceNumber(9);
    EXPECT_FALSE(msg.isValid());
}

TEST(IPCMessageTest, TimestampCanBeSetAndRetrieved) {
    IPCMessage msg(1);
    
    uint32_t testTimestamp = 12345678;
    msg.setTimestamp(testTimestamp);
    
    EXPECT_EQ(msg.getTimestamp(), testTimestamp);
}
