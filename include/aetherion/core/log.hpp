/** @file
 * Process-wide structured logging with reentrant sink delivery and synchronized sink replacement.
 */
#pragma once

#include <functional>
#include <string_view>

namespace aetherion::core {

enum class LogLevel { debug, info, warning, error };
using LogSink = std::function<void(LogLevel, std::string_view)>;

/// Process-wide logger with synchronized sink replacement. Delivery occurs outside that lock so
/// callbacks may reenter logging; custom sinks synchronize their own shared output state.
class Logger final {
  public:
    static void setSink(LogSink sink);
    static void write(LogLevel level, std::string_view message);
};

} // namespace aetherion::core
