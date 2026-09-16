#include "aetherion/core/log.hpp"

#include <gtest/gtest.h>

#include <string>

using aetherion::core::Logger;
using aetherion::core::LogLevel;

TEST(Logger, SendsStructuredSeverityAndMessageToConfiguredSink) {
    LogLevel captured_level = LogLevel::debug;
    std::string captured_message;
    Logger::setSink([&](LogLevel level, std::string_view message) {
        captured_level = level;
        captured_message = message;
    });
    Logger::write(LogLevel::warning, "large timestep");
    EXPECT_EQ(captured_level, LogLevel::warning);
    EXPECT_EQ(captured_message, "large timestep");
    Logger::setSink({});
}
