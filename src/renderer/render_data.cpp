#include "aetherion/renderer/render_data.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace aetherion::renderer {

Vec3f toCameraRelative(const math::Vec3d& world_position_m, const math::Vec3d& camera_origin_m,
                       double meters_to_render_units) {
    if (!world_position_m.isFinite() || !camera_origin_m.isFinite() ||
        !std::isfinite(meters_to_render_units) || meters_to_render_units <= 0.0) {
        throw std::invalid_argument(
            "camera-relative conversion requires finite values and positive scale");
    }
    const auto relative = (world_position_m - camera_origin_m) * meters_to_render_units;
    constexpr double maximum_float = static_cast<double>(std::numeric_limits<float>::max());
    if (std::abs(relative.x) > maximum_float || std::abs(relative.y) > maximum_float ||
        std::abs(relative.z) > maximum_float) {
        throw std::overflow_error("camera-relative position exceeds GPU float range");
    }
    return {static_cast<float>(relative.x), static_cast<float>(relative.y),
            static_cast<float>(relative.z)};
}

std::vector<BodyInstance> buildBodyInstances(const core::Scene& scene,
                                             const math::Vec3d& camera_origin_m,
                                             const RenderSettings& settings) {
    if (!std::isfinite(settings.meters_to_render_units) || settings.meters_to_render_units <= 0.0 ||
        !std::isfinite(settings.minimum_apparent_radius) ||
        settings.minimum_apparent_radius <= 0.0F) {
        throw std::invalid_argument(
            "render settings require positive finite scale and apparent radius");
    }
    std::vector<BodyInstance> instances;
    instances.reserve(scene.size());
    for (const auto& body : scene.bodies()) {
        const float physical_radius =
            static_cast<float>(body.radius_m * settings.meters_to_render_units);
        const float radius = std::max(physical_radius, settings.minimum_apparent_radius);
        const float mass_tint = static_cast<float>(
            std::clamp(std::log10(std::max(body.mass_kg, 1.0)) / 32.0, 0.0, 1.0));
        instances.push_back({toCameraRelative(body.state.position_m, camera_origin_m,
                                              settings.meters_to_render_units),
                             radius,
                             {0.25F + 0.7F * mass_tint, 0.55F, 1.0F - 0.5F * mass_tint},
                             body.id,
                             settings.selected_entity == body.id});
    }
    return instances;
}

} // namespace aetherion::renderer
