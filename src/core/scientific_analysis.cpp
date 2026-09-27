#include "aetherion/core/scientific_analysis.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace aetherion::core {

std::optional<MassCenter> centerOfMass(const Scene& scene) {
    MassCenter center;
    for (const auto& body : scene.bodies()) {
        center.total_mass_kg += body.mass_kg;
        center.position_m += body.mass_kg * body.state.position_m;
        center.velocity_mps += body.mass_kg * body.state.velocity_mps;
    }
    if (!(center.total_mass_kg > 0.0) || !std::isfinite(center.total_mass_kg))
        return std::nullopt;
    center.position_m /= center.total_mass_kg;
    center.velocity_mps /= center.total_mass_kg;
    if (!center.position_m.isFinite() || !center.velocity_mps.isFinite())
        return std::nullopt;
    return center;
}

std::optional<BodyObservation> observeBody(const Scene& scene, EntityId id, ReferenceFrame frame,
                                           std::optional<EntityId> frame_body_id) {
    const auto* body = scene.find(id);
    if (body == nullptr)
        return std::nullopt;
    math::Vec3d origin_m;
    math::Vec3d origin_velocity_mps;
    if (frame == ReferenceFrame::center_of_mass) {
        const auto center = centerOfMass(scene);
        if (!center)
            return std::nullopt;
        origin_m = center->position_m;
        origin_velocity_mps = center->velocity_mps;
    } else if (frame == ReferenceFrame::selected_body) {
        const auto* anchor = scene.find(frame_body_id.value_or(id));
        if (anchor == nullptr)
            return std::nullopt;
        origin_m = anchor->state.position_m;
        origin_velocity_mps = anchor->state.velocity_mps;
    }
    const auto velocity = body->state.velocity_mps - origin_velocity_mps;
    BodyObservation result{.id = id,
                           .position_m = body->state.position_m - origin_m,
                           .velocity_mps = velocity,
                           .acceleration_mps2 = body->state.acceleration_mps2,
                           .net_force_N = body->mass_kg * body->state.acceleration_mps2,
                           .speed_mps = velocity.norm(),
                           .kinetic_energy_J = 0.5 * body->mass_kg * velocity.squaredNorm()};
    if (!result.position_m.isFinite() || !result.velocity_mps.isFinite() ||
        !result.acceleration_mps2.isFinite() || !result.net_force_N.isFinite() ||
        !std::isfinite(result.speed_mps) || !std::isfinite(result.kinetic_energy_J))
        return std::nullopt;
    return result;
}

std::optional<ProbeObservation> sampleProbe(const physics::fields::IFieldProvider& provider,
                                            const FieldProbe& probe, double time_s) {
    if (probe.name.empty() || !probe.position_m.isFinite() || !std::isfinite(time_s))
        return std::nullopt;
    const auto field = provider.sample(probe.position_m, time_s);
    return ProbeObservation{.time_s = time_s,
                            .electric_Vpm = field.electric_Vpm,
                            .magnetic_T = field.magnetic_T,
                            .gravity_mps2 = field.gravity_mps2,
                            .electromagnetic_valid = field.valid,
                            .gravity_valid = field.gravity_valid};
}

ScientificHistory::ScientificHistory(std::size_t capacity) : capacity_(capacity) {
    if (capacity == 0U)
        throw std::invalid_argument("scientific history capacity must be positive");
}

void ScientificHistory::clear() noexcept {
    body_samples_.clear();
    probe_samples_.clear();
    has_time_ = false;
}

Status ScientificHistory::record(const Scene& scene,
                                 const physics::fields::IFieldProvider& provider, double time_s,
                                 std::optional<EntityId> selected_body,
                                 const std::vector<FieldProbe>& probes, ReferenceFrame frame,
                                 std::optional<EntityId> frame_body_id) {
    if (!std::isfinite(time_s) || probes.size() > 16U)
        return Error{ErrorCode::invalid_argument,
                     "history requires finite time and at most 16 probes"};
    for (const auto& probe : probes) {
        if (probe.name.empty() || !probe.position_m.isFinite())
            return Error{ErrorCode::invalid_argument,
                         "probe needs a name and finite world position"};
    }
    const bool probe_definitions_changed =
        probes_.size() != probes.size() ||
        !std::equal(probes_.begin(), probes_.end(), probes.begin(),
                    [](const FieldProbe& lhs, const FieldProbe& rhs) {
                        return lhs.name == rhs.name && lhs.position_m == rhs.position_m;
                    });
    if ((has_time_ && time_s < last_time_s_) || selected_body_ != selected_body ||
        probe_definitions_changed || frame_ != frame || frame_body_id_ != frame_body_id) {
        clear();
    }
    selected_body_ = selected_body;
    probes_ = probes;
    frame_ = frame;
    frame_body_id_ = frame_body_id;
    if (probe_samples_.size() != probes.size())
        probe_samples_.resize(probes.size());
    const bool replace = has_time_ && time_s == last_time_s_;
    has_time_ = true;
    last_time_s_ = time_s;
    if (selected_body) {
        const auto body = observeBody(scene, *selected_body, frame, frame_body_id);
        if (body) {
            if (replace && !body_samples_.empty())
                body_samples_.back() = {time_s, *body};
            else {
                if (body_samples_.size() == capacity_)
                    body_samples_.erase(body_samples_.begin());
                body_samples_.push_back({time_s, *body});
            }
        }
    }
    for (std::size_t index = 0; index < probes.size(); ++index) {
        const auto sample = sampleProbe(provider, probes[index], time_s);
        if (!sample)
            continue;
        auto& series = probe_samples_[index];
        if (replace && !series.empty())
            series.back() = *sample;
        else {
            if (series.size() == capacity_)
                series.erase(series.begin());
            series.push_back(*sample);
        }
    }
    return success();
}

} // namespace aetherion::core
