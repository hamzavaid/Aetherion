#include "aetherion/renderer/picking.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace aetherion::renderer {

std::optional<PickResult> pickBody(const core::Scene& scene, const Ray& ray,
                                   const PickingSettings& settings) {
    if (!ray.origin.isFinite() || !ray.direction.isFinite()) {
        throw std::invalid_argument("picking ray must be finite");
    }
    if (!std::isfinite(settings.radius_scale) || settings.radius_scale <= 0.0 ||
        !std::isfinite(settings.minimum_radius_m) || settings.minimum_radius_m < 0.0) {
        throw std::invalid_argument(
            "picking radii require a positive scale and non-negative minimum");
    }
    const auto direction = ray.direction.normalized();
    std::optional<PickResult> nearest;
    double nearest_distance = std::numeric_limits<double>::infinity();
    for (const auto& body : scene.bodies()) {
        const auto offset = ray.origin - body.state.position_m;
        const double projection = math::dot(offset, direction);
        const double radius_m =
            std::max(body.radius_m * settings.radius_scale, settings.minimum_radius_m);
        const double discriminant =
            projection * projection - (offset.squaredNorm() - radius_m * radius_m);
        if (discriminant < 0.0) {
            continue;
        }
        const double root = std::sqrt(discriminant);
        double distance = -projection - root;
        if (distance < 0.0) {
            distance = -projection + root;
        }
        if (distance >= 0.0 && distance < nearest_distance) {
            nearest_distance = distance;
            nearest = PickResult{body.id, distance, ray.origin + direction * distance};
        }
    }
    return nearest;
}

std::optional<ProbePickResult> pickProbe(const std::vector<core::FieldProbe>& probes,
                                         const Ray& ray, double marker_radius_m) {
    if (!ray.origin.isFinite() || !ray.direction.isFinite() || !std::isfinite(marker_radius_m) ||
        marker_radius_m < 0.0)
        throw std::invalid_argument("probe picking requires a finite ray and marker radius");
    if (marker_radius_m == 0.0)
        return std::nullopt;
    const auto direction = ray.direction.normalized();
    std::optional<ProbePickResult> nearest;
    double nearest_distance = std::numeric_limits<double>::infinity();
    for (std::size_t index = 0; index < probes.size(); ++index) {
        const auto& probe = probes[index];
        if (!probe.visible || !probe.position_m.isFinite())
            continue;
        const auto offset = ray.origin - probe.position_m;
        const double projection = math::dot(offset, direction);
        const double discriminant =
            projection * projection - (offset.squaredNorm() - marker_radius_m * marker_radius_m);
        if (discriminant < 0.0)
            continue;
        const double root = std::sqrt(discriminant);
        double distance = -projection - root;
        if (distance < 0.0)
            distance = -projection + root;
        if (distance >= 0.0 && distance < nearest_distance) {
            nearest_distance = distance;
            nearest = ProbePickResult{index, distance};
        }
    }
    return nearest;
}

} // namespace aetherion::renderer
