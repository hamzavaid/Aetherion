#include "aetherion/core/log.hpp"

#include <iostream>
#include <mutex>
#include <string_view>
#include <utility>

namespace aetherion::core {
namespace {

std::mutex sink_mutex;

void defaultSink(LogLevel level, std::string_view message) {
    constexpr std::string_view names[] = {"debug", "info", "warning", "error"};
    std::clog << '[' << names[static_cast<unsigned int>(level)] << "] " << message << '\n';
}

LogSink sink = defaultSink;

} // namespace

void Logger::setSink(LogSink new_sink) {
    std::scoped_lock lock(sink_mutex);
    sink = new_sink ? std::move(new_sink) : LogSink{defaultSink};
}

void Logger::write(LogLevel level, std::string_view message) {
    std::scoped_lock lock(sink_mutex);
    sink(level, message);
}

} // namespace aetherion::core
