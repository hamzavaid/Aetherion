#pragma once

#include <optional>
#include <vector>

#include "aetherion/core/scene.hpp"
#include "aetherion/core/scientific_analysis.hpp"
#include "aetherion/renderer/types.hpp"

namespace aetherion::renderer {

struct PickResult {
    core::EntityId id{};
    double distance_m{};
    math::Vec3d position_world_m;
};

struct PickingSettings {
    double radius_scale{1.0};
    double minimum_radius_m{};
};

struct ProbePickResult {
    std::size_t index{};
    double distance_m{};
};

/// Returns the nearest positive ray/sphere hit. Optional visual radii are expressed in world meters
/// and affect selection only; they never modify the physical scene.
[[nodiscard]] std::optional<PickResult> pickBody(const core::Scene& scene, const Ray& ray,
                                                 const PickingSettings& settings = {});

/// Ray-tests visible fixed probes using the apparent world-meter radius of their scene marker.
/// This affects selection only; a probe has no physical radius or source influence.
[[nodiscard]] std::optional<ProbePickResult> pickProbe(const std::vector<core::FieldProbe>& probes,
                                                       const Ray& ray, double marker_radius_m);

} // namespace aetherion::renderer
