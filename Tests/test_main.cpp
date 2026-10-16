#include <gtest/gtest.h>

/**
 * Test suite entry point for MIDI Splitter Plugin
 * 
 * This file serves as the main entry point for all unit tests and
 * property-based tests using Google Test framework.
 */

// Placeholder test to verify test framework is working
TEST(BasicTest, FrameworkWorks) {
    EXPECT_EQ(1 + 1, 2);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
