#include "aetherion/core/log.hpp"

int main() {
    aetherion::core::Logger::write(aetherion::core::LogLevel::info,
                                   "Aetherion engineering runtime initialized");
    return 0;
}
