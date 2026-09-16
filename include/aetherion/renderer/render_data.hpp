#pragma once

#include <optional>
#include <vector>

#include "aetherion/core/scene.hpp"
#include "aetherion/renderer/types.hpp"

namespace aetherion::renderer {

struct RenderSettings {
    double meters_to_render_units{1.0};
    float minimum_apparent_radius{0.01F};
    float body_radius_scale{1.0F};
    bool show_grid{true};
    std::optional<core::EntityId> selected_entity;
    bool trails_enabled{};
    double trail_duration_s{2'592'000.0};
    double simulation_time_s{};
};

struct BodyInstance {
    Vec3f position;
    float radius{};
    Vec3f color;
    core::EntityId id{};
    bool highlighted{};
};

/// Subtracts the double-precision camera origin before checked conversion to GPU float units.
[[nodiscard]] Vec3f toCameraRelative(const math::Vec3d& world_position_m,
                                     const math::Vec3d& camera_origin_m,
                                     double meters_to_render_units);
[[nodiscard]] std::vector<BodyInstance> buildBodyInstances(const core::Scene& scene,
                                                           const math::Vec3d& camera_origin_m,
                                                           const RenderSettings& settings);

} // namespace aetherion::renderer
