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

double visualBodyRadiusMeters(const core::Body& body, const RenderSettings& settings) {
    if (!std::isfinite(body.radius_m) || body.radius_m <= 0.0 ||
        !std::isfinite(settings.meters_to_render_units) || settings.meters_to_render_units <= 0.0 ||
        !std::isfinite(settings.minimum_apparent_radius) ||
        settings.minimum_apparent_radius <= 0.0F || !std::isfinite(settings.body_radius_scale) ||
        settings.body_radius_scale <= 0.0F) {
        throw std::invalid_argument("display radius requires positive finite body/render scales");
    }
    const double scaled_radius_m = body.radius_m * static_cast<double>(settings.body_radius_scale);
    const double minimum_radius_m =
        static_cast<double>(settings.minimum_apparent_radius) / settings.meters_to_render_units;
    return std::max(scaled_radius_m, minimum_radius_m);
}

SceneFocusBounds calculateSceneFocusBounds(const core::Scene& scene,
                                           const RenderSettings& settings) {
    if (scene.bodies().empty())
        return {};
    auto minimum = scene.bodies().front().state.position_m;
    auto maximum = minimum;
    for (const auto& body : scene.bodies()) {
        const double radius_m = visualBodyRadiusMeters(body, settings);
        minimum.x = std::min(minimum.x, body.state.position_m.x - radius_m);
        minimum.y = std::min(minimum.y, body.state.position_m.y - radius_m);
        minimum.z = std::min(minimum.z, body.state.position_m.z - radius_m);
        maximum.x = std::max(maximum.x, body.state.position_m.x + radius_m);
        maximum.y = std::max(maximum.y, body.state.position_m.y + radius_m);
        maximum.z = std::max(maximum.z, body.state.position_m.z + radius_m);
    }
    const auto center = (minimum + maximum) * 0.5;
    const double radius_m = std::max((maximum - minimum).norm() * 0.5, 1.0e-12);
    return {.center_world_m = center, .radius_m = radius_m};
}

std::vector<BodyInstance> buildBodyInstances(const core::Scene& scene,
                                             const math::Vec3d& camera_origin_m,
                                             const RenderSettings& settings) {
    if (!std::isfinite(settings.meters_to_render_units) || settings.meters_to_render_units <= 0.0 ||
        !std::isfinite(settings.minimum_apparent_radius) ||
        settings.minimum_apparent_radius <= 0.0F || !std::isfinite(settings.body_radius_scale) ||
        settings.body_radius_scale <= 0.0F) {
        throw std::invalid_argument(
            "render settings require positive finite coordinate, radius, and apparent scales");
    }
    std::vector<BodyInstance> instances;
    instances.reserve(scene.size());
    for (const auto& body : scene.bodies()) {
        const float radius = static_cast<float>(visualBodyRadiusMeters(body, settings) *
                                                settings.meters_to_render_units);
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
