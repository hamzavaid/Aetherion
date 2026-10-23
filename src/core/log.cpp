/** @file
 * Process-wide structured logging with reentrant sink delivery and synchronized sink replacement.
 */
#include "aetherion/core/log.hpp"

#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <string_view>
#include <syncstream>
#include <utility>

namespace aetherion::core {
namespace {

std::mutex sink_mutex;

void defaultSink(LogLevel level, std::string_view message) {
    constexpr std::string_view names[] = {"debug", "info", "warning", "error"};
    const auto index = static_cast<unsigned int>(level);
    std::osyncstream(std::clog) << '[' << (index < std::size(names) ? names[index] : "unknown")
                                << "] " << message << '\n';
}

std::shared_ptr<LogSink> sink = std::make_shared<LogSink>(defaultSink);

} // namespace

void Logger::setSink(LogSink new_sink) {
    auto replacement =
        std::make_shared<LogSink>(new_sink ? std::move(new_sink) : LogSink{defaultSink});
    {
        std::scoped_lock lock(sink_mutex);
        sink.swap(replacement);
    }
    // Destruction of the old callable also occurs outside the lock; user-owned captures may log.
}

void Logger::write(LogLevel level, std::string_view message) {
    std::shared_ptr<LogSink> selected_sink;
    {
        std::scoped_lock lock(sink_mutex);
        selected_sink = sink;
    }
    // Never invoke application code under the configuration lock: sinks may replace themselves
    // or emit another diagnostic. The snapshot also keeps the old callable alive during delivery.
    (*selected_sink)(level, message);
}

} // namespace aetherion::core
