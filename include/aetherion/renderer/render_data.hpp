#pragma once

#include <optional>
#include <vector>

#include "aetherion/core/scene.hpp"
#include "aetherion/renderer/field_visualization.hpp"
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
    FieldVisualizationSettings field_visualization;
};

struct BodyInstance {
    Vec3f position;
    float radius{};
    Vec3f color;
    core::EntityId id{};
    bool highlighted{};
};

struct SceneFocusBounds {
    math::Vec3d center_world_m;
    double radius_m{1.0};
};

/// Subtracts the double-precision camera origin before checked conversion to GPU float units.
[[nodiscard]] Vec3f toCameraRelative(const math::Vec3d& world_position_m,
                                     const math::Vec3d& camera_origin_m,
                                     double meters_to_render_units);
/// Returns the body's effective display radius converted back to world meters for camera framing.
[[nodiscard]] double visualBodyRadiusMeters(const core::Body& body, const RenderSettings& settings);
/// Computes an axis-aligned display-aware scene bound suitable for Camera::focus.
[[nodiscard]] SceneFocusBounds calculateSceneFocusBounds(const core::Scene& scene,
                                                         const RenderSettings& settings);
[[nodiscard]] std::vector<BodyInstance> buildBodyInstances(const core::Scene& scene,
                                                           const math::Vec3d& camera_origin_m,
                                                           const RenderSettings& settings);

} // namespace aetherion::renderer
