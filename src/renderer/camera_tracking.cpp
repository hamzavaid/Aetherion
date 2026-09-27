#include "aetherion/renderer/camera_tracking.hpp"

#include "aetherion/core/scientific_analysis.hpp"

namespace aetherion::renderer {

bool CameraTracker::update(const core::Scene& scene, Camera& camera) {
    if (center_of_mass_) {
        const auto center = core::centerOfMass(scene);
        if (!center) {
            stop();
            return false;
        }
        camera.setTargetWorld(center->position_m);
        return true;
    }
    if (!followed_entity_)
        return false;
    const auto* body = scene.find(*followed_entity_);
    if (body == nullptr) {
        followed_entity_.reset();
        return false;
    }
    camera.setTargetWorld(body->state.position_m);
    return true;
}

} // namespace aetherion::renderer
