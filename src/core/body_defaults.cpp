#include "aetherion/core/body_defaults.hpp"

#include <algorithm>
#include <utility>

namespace aetherion::core {

Body makeInteractiveBody(const Scene& scene, const RuntimeSettings& runtime, std::string name) {
    const bool electrostatic = runtime.electromagnetism.electrostatics_enabled;
    const bool magnetic = runtime.electromagnetism.magnetic_enabled;
    Body body{.name = std::move(name),
              .mass_kg = 1.0,
              .charge_C = electrostatic ? 1.0e-6 : (magnetic ? 1.0 : 0.0),
              .radius_m = (electrostatic || magnetic) ? 0.08 : 1.0};
    if (scene.bodies().empty())
        return body;
    double rightmost_edge_m =
        scene.bodies().front().state.position_m.x + scene.bodies().front().radius_m;
    for (const auto& existing : scene.bodies())
        rightmost_edge_m =
            std::max(rightmost_edge_m, existing.state.position_m.x + existing.radius_m);
    const double gap_m = std::max(4.0 * body.radius_m, 1.0);
    body.state.position_m.x = rightmost_edge_m + gap_m;
    return body;
}

} // namespace aetherion::core
