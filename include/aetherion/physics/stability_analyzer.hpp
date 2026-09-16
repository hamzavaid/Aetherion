#pragma once

#include <string>
#include <vector>

#include "aetherion/core/scene.hpp"

namespace aetherion::physics {

enum class StabilitySeverity { caution, unstable };

struct StabilityWarning {
    StabilitySeverity severity{};
    double characteristic_timescale_s{};
    double timestep_to_timescale_ratio{};
    std::string message;
};

/// Checks gravity orbital and crossing timescales; ratios are advisory, not adaptive stepping.
[[nodiscard]] std::vector<StabilityWarning> analyzeTimestep(const core::Scene& scene, double dt_s);

} // namespace aetherion::physics
