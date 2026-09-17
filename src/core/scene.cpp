#include "aetherion/core/scene.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace aetherion::core {

Status Scene::validate(const Body& body) {
    if (body.name.empty()) {
        return Error{ErrorCode::invalid_argument, "body name must not be empty"};
    }
    if (!std::isfinite(body.mass_kg) || body.mass_kg < 0.0) {
        return Error{ErrorCode::invalid_argument, "mass must be finite and non-negative in kg"};
    }
    if (!std::isfinite(body.charge_C)) {
        return Error{ErrorCode::invalid_argument, "charge must be finite in C"};
    }
    if (!std::isfinite(body.radius_m) || body.radius_m <= 0.0) {
        return Error{ErrorCode::invalid_argument, "radius must be finite and positive in m"};
    }
    if (!body.state.position_m.isFinite() || !body.state.velocity_mps.isFinite() ||
        !body.state.acceleration_mps2.isFinite()) {
        return Error{ErrorCode::invalid_argument, "body state vectors must be finite SI values"};
    }
    return success();
}

Result<EntityId> Scene::createBody(Body body) {
    const auto validation = validate(body);
    if (!validation) {
        return validation.error();
    }
    body.id = next_id_++;
    const EntityId id = body.id;
    bodies_.push_back(std::move(body));
    return id;
}

Result<EntityId> Scene::importBody(Body body) {
    const auto validation = validate(body);
    if (!validation)
        return validation.error();
    if (body.id == 0U || find(body.id) != nullptr) {
        return Error{ErrorCode::invalid_argument, "imported body ID must be non-zero and unique"};
    }
    const EntityId id = body.id;
    next_id_ = std::max(next_id_, id + 1U);
    bodies_.push_back(std::move(body));
    return id;
}

bool Scene::remove(EntityId id) noexcept {
    const auto position = std::find_if(bodies_.begin(), bodies_.end(),
                                       [id](const Body& body) { return body.id == id; });
    if (position == bodies_.end()) {
        return false;
    }
    bodies_.erase(position);
    return true;
}

Status Scene::replace(EntityId id, Body body) {
    auto* existing = find(id);
    if (existing == nullptr) {
        return Error{ErrorCode::not_found, "body update target does not exist"};
    }
    body.id = id;
    const auto validation = validate(body);
    if (!validation) {
        return validation;
    }
    *existing = std::move(body);
    return success();
}

Body* Scene::find(EntityId id) noexcept {
    const auto position = std::find_if(bodies_.begin(), bodies_.end(),
                                       [id](const Body& body) { return body.id == id; });
    return position == bodies_.end() ? nullptr : &*position;
}

const Body* Scene::find(EntityId id) const noexcept {
    const auto position = std::find_if(bodies_.begin(), bodies_.end(),
                                       [id](const Body& body) { return body.id == id; });
    return position == bodies_.end() ? nullptr : &*position;
}

} // namespace aetherion::core
