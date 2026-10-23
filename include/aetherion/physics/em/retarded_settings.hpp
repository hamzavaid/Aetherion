/** @file
 * Finite-propagation tolerances and explicit per-source and aggregate history-allocation limits.
 */
#pragma once

#include <cstddef>

#include "aetherion/core/error.hpp"

namespace aetherion::physics::em {

struct RetardedSettings {
    bool enabled{};
    double maximum_observable_distance_m{1000.0};
    double absolute_time_tolerance_s{1.0e-15};
    double relative_time_tolerance{1.0e-10};
    double minimum_distance_m{1.0e-9};
    std::size_t maximum_iterations{64};
    [[nodiscard]] bool operator==(const RetardedSettings&) const = default;
};

inline constexpr std::size_t maximum_history_samples_per_source = 65536;
inline constexpr std::size_t maximum_total_history_samples = 1048576;
inline constexpr std::size_t maximum_history_sources = 256;

[[nodiscard]] core::Status validateRetardedSettings(const RetardedSettings& settings);
/// ceil(D/(c*dt))+4 interpolation-margin samples; rejects unsafe allocations and invalid SI inputs.
[[nodiscard]] core::Result<std::size_t> historyCapacity(const RetardedSettings& settings,
                                                        double physics_dt_s);

} // namespace aetherion::physics::em
