#pragma once

#include <optional>

#include "aetherion/core/scene.hpp"
#include "aetherion/renderer/camera.hpp"

namespace aetherion::renderer {

/// Binds an orbit camera target to a stable scene entity while preserving user-controlled zoom and
/// orbit orientation. Missing entities automatically detach the tracker.
class CameraTracker final {
  public:
    void follow(core::EntityId entity) noexcept { followed_entity_ = entity; }
    void stop() noexcept { followed_entity_.reset(); }
    [[nodiscard]] std::optional<core::EntityId> followedEntity() const noexcept {
        return followed_entity_;
    }

    /// Updates the target to the body's inertial-world position in meters. Returns true while a
    /// valid entity is actively followed; no camera distance or angle is changed.
    [[nodiscard]] bool update(const core::Scene& scene, Camera& camera);

  private:
    std::optional<core::EntityId> followed_entity_;
};

} // namespace aetherion::renderer
