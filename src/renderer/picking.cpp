#include "aetherion/renderer/picking.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace aetherion::renderer {

std::optional<PickResult> pickBody(const core::Scene& scene, const Ray& ray) {
    if (!ray.origin.isFinite() || !ray.direction.isFinite()) {
        throw std::invalid_argument("picking ray must be finite");
    }
    const auto direction = ray.direction.normalized();
    std::optional<PickResult> nearest;
    double nearest_distance = std::numeric_limits<double>::infinity();
    for (const auto& body : scene.bodies()) {
        const auto offset = ray.origin - body.state.position_m;
        const double projection = math::dot(offset, direction);
        const double discriminant =
            projection * projection - (offset.squaredNorm() - body.radius_m * body.radius_m);
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
