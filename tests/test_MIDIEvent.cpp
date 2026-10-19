#include <gtest/gtest.h>
#include "../source/MIDIEvent.h"

/**
 * @brief Test fixture for MIDIEvent tests
 */
class MIDIEventTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @brief Test default constructor
 */
TEST_F(MIDIEventTest, DefaultConstructor) {
    MIDIEvent event;
    
    EXPECT_EQ(event.status, 0);
    EXPECT_EQ(event.note, 0);
    EXPECT_EQ(event.velocity, 0);
    EXPECT_EQ(event.channel, 0);
    EXPECT_EQ(event.timestamp, 0);
}

/**
 * @brief Test parameterized constructor
 */
TEST_F(MIDIEventTest, ParameterizedConstructor) {
    MIDIEvent event(0x90, 60, 100, 0, 1000);
    
    EXPECT_EQ(event.status, 0x90);
    EXPECT_EQ(event.note, 60);
    EXPECT_EQ(event.velocity, 100);
    EXPECT_EQ(event.channel, 0);
    EXPECT_EQ(event.timestamp, 1000);
}

/**
 * @brief Test isNoteOn() method
 */
TEST_F(MIDIEventTest, IsNoteOn) {
    // Note-on with velocity > 0
    MIDIEvent noteOn(0x90, 60, 100, 0, 1000);
    EXPECT_TRUE(noteOn.isNoteOn());
    
    // Note-on with velocity = 0 (actually note-off)
    MIDIEvent noteOnZeroVel(0x90, 60, 0, 0, 1000);
    EXPECT_FALSE(noteOnZeroVel.isNoteOn());
    
    // Note-off
    MIDIEvent noteOff(0x80, 60, 0, 0, 1000);
    EXPECT_FALSE(noteOff.isNoteOn());
    
    // Control Change
    MIDIEvent cc(0xB0, 64, 127, 0, 1000);
    EXPECT_FALSE(cc.isNoteOn());
    
    // Test all MIDI channels for note-on
    for (uint8_t channel = 0; channel < 16; ++channel) {
        MIDIEvent channelNoteOn(0x90 | channel, 60, 100, channel, 1000);
        EXPECT_TRUE(channelNoteOn.isNoteOn()) << "Channel " << static_cast<int>(channel);
    }
}

/**
 * @brief Test isNoteOff() method
 */
TEST_F(MIDIEventTest, IsNoteOff) {
    // Traditional note-off (0x80-0x8F)
    MIDIEvent noteOff(0x80, 60, 0, 0, 1000);
    EXPECT_TRUE(noteOff.isNoteOff());
    
    // Note-on with velocity = 0 (also note-off)
    MIDIEvent noteOnZeroVel(0x90, 60, 0, 0, 1000);
    EXPECT_TRUE(noteOnZeroVel.isNoteOff());
    
    // Note-on with velocity > 0
    MIDIEvent noteOn(0x90, 60, 100, 0, 1000);
    EXPECT_FALSE(noteOn.isNoteOff());
    
    // Control Change
    MIDIEvent cc(0xB0, 64, 127, 0, 1000);
    EXPECT_FALSE(cc.isNoteOff());
    
    // Test all MIDI channels for note-off
    for (uint8_t channel = 0; channel < 16; ++channel) {
        MIDIEvent channelNoteOff(0x80 | channel, 60, 0, channel, 1000);
        EXPECT_TRUE(channelNoteOff.isNoteOff()) << "Channel " << static_cast<int>(channel);
    }
}

/**
 * @brief Test isControlChange() method
 */
TEST_F(MIDIEventTest, IsControlChange) {
    // Control Change
    MIDIEvent cc(0xB0, 64, 127, 0, 1000);
    EXPECT_TRUE(cc.isControlChange());
    
    // Note-on
    MIDIEvent noteOn(0x90, 60, 100, 0, 1000);
    EXPECT_FALSE(noteOn.isControlChange());
    
    // Note-off
    MIDIEvent noteOff(0x80, 60, 0, 0, 1000);
    EXPECT_FALSE(noteOff.isControlChange());
    
    // Test all MIDI channels for Control Change
    for (uint8_t channel = 0; channel < 16; ++channel) {
        MIDIEvent channelCC(0xB0 | channel, 64, 127, channel, 1000);
        EXPECT_TRUE(channelCC.isControlChange()) << "Channel " << static_cast<int>(channel);
    }
}

/**
 * @brief Test getChannel() method
 */
TEST_F(MIDIEventTest, GetChannel) {
    // Test all MIDI channels
    for (uint8_t channel = 0; channel < 16; ++channel) {
        MIDIEvent event(0x90 | channel, 60, 100, channel, 1000);
        EXPECT_EQ(event.getChannel(), channel) << "Channel " << static_cast<int>(channel);
    }
    
    // Test with different message types
    MIDIEvent noteOn(0x95, 60, 100, 5, 1000);  // Channel 5
    EXPECT_EQ(noteOn.getChannel(), 5);
    
    MIDIEvent noteOff(0x87, 60, 0, 7, 1000);   // Channel 7
    EXPECT_EQ(noteOff.getChannel(), 7);
    
    MIDIEvent cc(0xBC, 64, 127, 12, 1000);     // Channel 12
    EXPECT_EQ(cc.getChannel(), 12);
}

/**
 * @brief Test edge cases and boundary values
 */
TEST_F(MIDIEventTest, EdgeCases) {
    // Test maximum values
    MIDIEvent maxEvent(0xFF, 127, 127, 15, INT64_MAX);
    EXPECT_EQ(maxEvent.status, 0xFF);
    EXPECT_EQ(maxEvent.note, 127);
    EXPECT_EQ(maxEvent.velocity, 127);
    EXPECT_EQ(maxEvent.channel, 15);
    EXPECT_EQ(maxEvent.timestamp, INT64_MAX);
    
    // Test minimum values
    MIDIEvent minEvent(0x80, 0, 0, 0, 0);
    EXPECT_EQ(minEvent.status, 0x80);
    EXPECT_EQ(minEvent.note, 0);
    EXPECT_EQ(minEvent.velocity, 0);
    EXPECT_EQ(minEvent.channel, 0);
    EXPECT_EQ(minEvent.timestamp, 0);
    
    // Test velocity = 1 (minimum for note-on)
    MIDIEvent minVelNoteOn(0x90, 60, 1, 0, 1000);
    EXPECT_TRUE(minVelNoteOn.isNoteOn());
    EXPECT_FALSE(minVelNoteOn.isNoteOff());
}

/**
 * @brief Test MIDI specification compliance
 */
TEST_F(MIDIEventTest, MIDISpecCompliance) {
    // Test note-on range (0x90-0x9F)
    for (uint8_t status = 0x90; status <= 0x9F; ++status) {
        MIDIEvent event(status, 60, 100, 0, 1000);
        EXPECT_TRUE(event.isNoteOn()) << "Status: 0x" << std::hex << static_cast<int>(status);
        EXPECT_FALSE(event.isNoteOff());
        EXPECT_FALSE(event.isControlChange());
    }
    
    // Test note-off range (0x80-0x8F)
    for (uint8_t status = 0x80; status <= 0x8F; ++status) {
        MIDIEvent event(status, 60, 0, 0, 1000);
        EXPECT_FALSE(event.isNoteOn()) << "Status: 0x" << std::hex << static_cast<int>(status);
        EXPECT_TRUE(event.isNoteOff());
        EXPECT_FALSE(event.isControlChange());
    }
    
    // Test Control Change range (0xB0-0xBF)
    for (uint8_t status = 0xB0; status <= 0xBF; ++status) {
        MIDIEvent event(status, 64, 127, 0, 1000);
        EXPECT_FALSE(event.isNoteOn()) << "Status: 0x" << std::hex << static_cast<int>(status);
        EXPECT_FALSE(event.isNoteOff());
        EXPECT_TRUE(event.isControlChange());
    }
}

/**
 * @brief Test common MIDI scenarios
 */
TEST_F(MIDIEventTest, CommonMIDIScenarios) {
    // Middle C note-on
    MIDIEvent middleC(0x90, 60, 100, 0, 1000);
    EXPECT_TRUE(middleC.isNoteOn());
    EXPECT_EQ(middleC.note, 60);
    EXPECT_EQ(middleC.velocity, 100);
    
    // Sustain pedal on
    MIDIEvent sustainOn(0xB0, 64, 127, 0, 2000);
    EXPECT_TRUE(sustainOn.isControlChange());
    EXPECT_EQ(sustainOn.note, 64);  // CC 64 = sustain pedal
    EXPECT_EQ(sustainOn.velocity, 127);
    
    // Sustain pedal off
    MIDIEvent sustainOff(0xB0, 64, 0, 0, 3000);
    EXPECT_TRUE(sustainOff.isControlChange());
    EXPECT_EQ(sustainOff.note, 64);
    EXPECT_EQ(sustainOff.velocity, 0);
    
    // Note-off using velocity 0 method
    MIDIEvent noteOffVel0(0x90, 60, 0, 0, 4000);
    EXPECT_TRUE(noteOffVel0.isNoteOff());
    EXPECT_FALSE(noteOffVel0.isNoteOn());
}