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

} // namespace aetherion::renderer
