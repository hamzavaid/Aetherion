#pragma once

#include <optional>

#include "aetherion/core/scene.hpp"
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

/// Returns the nearest positive ray/sphere hit. Optional visual radii are expressed in world meters
/// and affect selection only; they never modify the physical scene.
[[nodiscard]] std::optional<PickResult> pickBody(const core::Scene& scene, const Ray& ray,
                                                 const PickingSettings& settings = {});

} // namespace aetherion::renderer
