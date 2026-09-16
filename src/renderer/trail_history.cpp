#include "aetherion/renderer/trail_history.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <unordered_set>

namespace aetherion::renderer {

TrailHistory::TrailHistory(std::size_t maximum_points_per_body)
    : maximum_points_per_body_(maximum_points_per_body) {
    if (maximum_points_per_body == 0U) {
        throw std::invalid_argument("trail history capacity must be non-zero");
    }
}

void TrailHistory::sample(const core::Scene& scene, double simulation_time_s, double duration_s) {
    if (!std::isfinite(simulation_time_s) || !std::isfinite(duration_s) || duration_s <= 0.0) {
        throw std::invalid_argument(
            "trail sampling requires finite time and positive duration in s");
    }
    if (has_time_ && simulation_time_s < latest_time_s_)
        clear();
    has_time_ = true;
    latest_time_s_ = simulation_time_s;

    std::unordered_set<core::EntityId> live_ids;
    live_ids.reserve(scene.size());
    for (const auto& body : scene.bodies()) {
        live_ids.insert(body.id);
        auto [position, inserted] = trails_.try_emplace(body.id, BodyTrail{.id = body.id});
        auto& points = position->second.points;
        if (inserted)
            points.reserve(maximum_points_per_body_);
        if (!points.empty() && points.back().time_s == simulation_time_s) {
            points.back().position_world_m = body.state.position_m;
        } else {
            if (points.size() == maximum_points_per_body_)
                points.erase(points.begin());
            points.push_back({simulation_time_s, body.state.position_m});
        }
        const double earliest_time = simulation_time_s - duration_s;
        const auto first_retained = std::lower_bound(
            points.begin(), points.end(), earliest_time,
            [](const TrailPoint& point, double time) { return point.time_s < time; });
        points.erase(points.begin(), first_retained);
    }
    std::erase_if(trails_, [&](const auto& entry) { return !live_ids.contains(entry.first); });
}

void TrailHistory::clear() noexcept {
    trails_.clear();
    latest_time_s_ = 0.0;
    has_time_ = false;
}

const BodyTrail* TrailHistory::find(core::EntityId id) const noexcept {
    const auto found = trails_.find(id);
    return found == trails_.end() ? nullptr : &found->second;
}

} // namespace aetherion::renderer
