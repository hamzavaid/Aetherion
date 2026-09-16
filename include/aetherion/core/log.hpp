#pragma once

#include <functional>
#include <string_view>

namespace aetherion::core {

enum class LogLevel { debug, info, warning, error };
using LogSink = std::function<void(LogLevel, std::string_view)>;

/// Process-wide, thread-safe logger. Tests may replace the sink to capture diagnostics.
class Logger final {
  public:
    static void setSink(LogSink sink);
    static void write(LogLevel level, std::string_view message);
};

} // namespace aetherion::core
