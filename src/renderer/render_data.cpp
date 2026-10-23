/** @file
 * Validated presentation settings and checked world-double to camera-relative GPU-float conversion.
 */
#include "aetherion/renderer/render_data.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace aetherion::renderer {

core::Status validateRenderSettings(const RenderSettings& settings) {
    const auto positive = [](double value) { return std::isfinite(value) && value > 0.0; };
    const auto& motion = settings.motion_glyphs;
    if (!positive(settings.meters_to_render_units) || !positive(settings.minimum_apparent_radius) ||
        !positive(settings.body_radius_scale) || !positive(settings.trail_duration_s) ||
        !std::isfinite(settings.simulation_time_s) || settings.simulation_time_s < 0.0 ||
        !positive(motion.length_fraction) || motion.length_fraction > 1.0 ||
        !positive(motion.velocity_reference_mps) || !positive(motion.force_reference_N) ||
        !positive(motion.acceleration_reference_mps2) || static_cast<int>(motion.scaling) < 0 ||
        static_cast<int>(motion.scaling) > 2 || static_cast<int>(settings.reference_frame) < 0 ||
        static_cast<int>(settings.reference_frame) > 2 || settings.probes.size() > 16U)
        return core::Error{core::ErrorCode::invalid_argument,
                           "invalid finite render settings or presentation work budget"};
    for (const auto& probe : settings.probes)
        if (probe.name.empty() || !probe.position_m.isFinite() ||
            !positive(probe.vector_length_fraction) || probe.vector_length_fraction > 1.0)
            return core::Error{core::ErrorCode::invalid_argument, "invalid field probe definition"};
    return validateFieldVisualizationSettings(settings.field_visualization);
}

Vec3f toCameraRelative(const math::Vec3d& world_position_m, const math::Vec3d& camera_origin_m,
                       double meters_to_render_units) {
    if (!world_position_m.isFinite() || !camera_origin_m.isFinite() ||
        !std::isfinite(meters_to_render_units) || meters_to_render_units <= 0.0) {
        throw std::invalid_argument(
            "camera-relative conversion requires finite values and positive scale");
    }
    const auto relative = (world_position_m - camera_origin_m) * meters_to_render_units;
    constexpr double maximum_float = static_cast<double>(std::numeric_limits<float>::max());
    if (!relative.isFinite() || std::abs(relative.x) > maximum_float ||
        std::abs(relative.y) > maximum_float || std::abs(relative.z) > maximum_float) {
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
    const double radius_m = std::max(scaled_radius_m, minimum_radius_m);
    if (!std::isfinite(radius_m))
        throw std::overflow_error("visual body radius exceeds finite world-space range");
    return radius_m;
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
    const math::Vec3d center{std::midpoint(minimum.x, maximum.x),
                             std::midpoint(minimum.y, maximum.y),
                             std::midpoint(minimum.z, maximum.z)};
    const double radius_m = std::max((maximum * 0.5 - minimum * 0.5).norm(), 1.0e-12);
    if (!center.isFinite() || !std::isfinite(radius_m))
        throw std::overflow_error("scene focus bounds exceed finite world-space range");
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
        if (!std::isfinite(radius) || radius <= 0.0F)
            throw std::overflow_error("body radius is outside the GPU float range");
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
