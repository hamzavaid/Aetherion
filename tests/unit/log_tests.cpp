// Structured message delivery and reentrant/custom sink behavior.
#include "aetherion/core/log.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

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

TEST(Logger, SinkCanReplaceItselfWithoutDeadlocking) {
    int calls = 0;
    Logger::setSink([&](LogLevel, std::string_view) {
        ++calls;
        Logger::setSink([&](LogLevel, std::string_view) { ++calls; });
        Logger::write(LogLevel::debug, "nested message");
    });
    Logger::write(LogLevel::info, "outer message");
    EXPECT_EQ(calls, 2);
    Logger::setSink({});
}

TEST(Logger, StatefulSinkKeepsItsIdentityAcrossMessages) {
    std::vector<int> counts;
    Logger::setSink(
        [count = 0, &counts](LogLevel, std::string_view) mutable { counts.push_back(++count); });
    Logger::write(LogLevel::info, "one");
    Logger::write(LogLevel::info, "two");
    EXPECT_EQ(counts, (std::vector<int>{1, 2}));
    Logger::setSink({});
}
