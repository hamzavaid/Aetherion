#include "aetherion/core/command_queue.hpp"

#include <cmath>
#include <type_traits>
#include <utility>

namespace aetherion::core {
namespace {

Status applyPatch(Scene& scene, const UpdateBodyCommand& command) {
    const auto* original = scene.find(command.id);
    if (original == nullptr) {
        return Error{ErrorCode::not_found, "body update target does not exist"};
    }
    Body updated = *original;
    const auto& patch = command.patch;
    if (patch.name)
        updated.name = *patch.name;
    if (patch.mass_kg)
        updated.mass_kg = *patch.mass_kg;
    if (patch.charge_C)
        updated.charge_C = *patch.charge_C;
    if (patch.radius_m)
        updated.radius_m = *patch.radius_m;
    if (patch.fixed)
        updated.fixed = *patch.fixed;
    if (patch.interactions)
        updated.interactions = *patch.interactions;
    if (patch.position_m)
        updated.state.position_m = *patch.position_m;
    if (patch.velocity_mps)
        updated.state.velocity_mps = *patch.velocity_mps;
    return scene.replace(command.id, std::move(updated));
}

std::string description(const SimulationCommand& command) {
    return std::visit(
        [](const auto& typed) -> std::string {
            using T = std::decay_t<decltype(typed)>;
            if constexpr (std::is_same_v<T, CreateBodyCommand>)
                return "create body";
            if constexpr (std::is_same_v<T, DeleteBodyCommand>)
                return "delete body";
            if constexpr (std::is_same_v<T, UpdateBodyCommand>)
                return "update body";
            if constexpr (std::is_same_v<T, SetPhysicsDtCommand>)
                return "set physics timestep";
            if constexpr (std::is_same_v<T, SetTimeScaleCommand>)
                return "set time scale";
            return "set gravity enabled";
        },
        command);
}

Status applyOne(const SimulationCommand& command, Scene& scene, RuntimeSettings& settings,
                std::optional<EntityId>& created_id) {
    return std::visit(
        [&](const auto& typed) -> Status {
            using T = std::decay_t<decltype(typed)>;
            if constexpr (std::is_same_v<T, CreateBodyCommand>) {
                const auto result = scene.createBody(typed.body);
                if (!result)
                    return result.error();
                created_id = result.value();
                return success();
            } else if constexpr (std::is_same_v<T, DeleteBodyCommand>) {
                return scene.remove(typed.id) ? success()
                                              : Status{Error{ErrorCode::not_found,
                                                             "body delete target does not exist"}};
            } else if constexpr (std::is_same_v<T, UpdateBodyCommand>) {
                return applyPatch(scene, typed);
            } else if constexpr (std::is_same_v<T, SetPhysicsDtCommand>) {
                if (!std::isfinite(typed.physics_dt_s) || typed.physics_dt_s <= 0.0) {
                    return Error{ErrorCode::invalid_argument,
                                 "physics timestep must be finite and positive in s"};
                }
                settings.physics_dt_s = typed.physics_dt_s;
                return success();
            } else if constexpr (std::is_same_v<T, SetTimeScaleCommand>) {
                if (!std::isfinite(typed.time_scale) || typed.time_scale < 0.0) {
                    return Error{ErrorCode::invalid_argument,
                                 "time scale must be finite and non-negative"};
                }
                settings.time_scale = typed.time_scale;
                return success();
            } else {
                settings.gravity_enabled = typed.enabled;
                return success();
            }
        },
        command);
}

} // namespace

void CommandQueue::enqueue(SimulationCommand command) {
    std::scoped_lock lock(mutex_);
    pending_.push_back(std::move(command));
}

CommandApplyReport CommandQueue::apply(Scene& scene, RuntimeSettings& settings,
                                       double simulation_time_s) {
    std::vector<SimulationCommand> commands;
    {
        std::scoped_lock lock(mutex_);
        commands.swap(pending_);
    }
    CommandApplyReport report;
    for (const auto& command : commands) {
        std::optional<EntityId> created_id;
        const auto status = applyOne(command, scene, settings, created_id);
        if (status) {
            ++report.accepted;
            if (created_id)
                report.created_ids.push_back(*created_id);
        } else {
            ++report.rejected;
        }
        event_log_.push_back({next_sequence_++, simulation_time_s, status.hasValue(),
                              description(command), status ? "accepted" : status.error().message});
        if (event_log_.size() > 4096U) {
            event_log_.erase(event_log_.begin());
        }
    }
    return report;
}

std::size_t CommandQueue::pendingCount() const {
    std::scoped_lock lock(mutex_);
    return pending_.size();
}

} // namespace aetherion::core
