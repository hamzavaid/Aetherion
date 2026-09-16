#pragma once

#include <cstddef>
#include <unordered_map>
#include <vector>

#include "aetherion/core/scene.hpp"

namespace aetherion::renderer {

struct TrailPoint {
    double time_s{};
    math::Vec3d position_world_m;
};

struct BodyTrail {
    core::EntityId id{};
    std::vector<TrailPoint> points;
};

/// Bounded per-body trajectory history retained by physical simulation time, never frame count.
class TrailHistory final {
  public:
    explicit TrailHistory(std::size_t maximum_points_per_body = 8192);
    void sample(const core::Scene& scene, double simulation_time_s, double duration_s);
    void clear() noexcept;
    [[nodiscard]] const BodyTrail* find(core::EntityId id) const noexcept;
    [[nodiscard]] const std::unordered_map<core::EntityId, BodyTrail>& trails() const noexcept {
        return trails_;
    }

  private:
    std::size_t maximum_points_per_body_;
    std::unordered_map<core::EntityId, BodyTrail> trails_;
    double latest_time_s_{};
    bool has_time_{};
};

} // namespace aetherion::renderer
