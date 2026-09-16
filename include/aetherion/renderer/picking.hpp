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

/// Returns the nearest positive ray/sphere hit using physical body radii in world meters.
[[nodiscard]] std::optional<PickResult> pickBody(const core::Scene& scene, const Ray& ray);

} // namespace aetherion::renderer
