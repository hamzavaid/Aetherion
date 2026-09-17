#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "aetherion/core/scene.hpp"
#include "aetherion/physics/em/electromagnetic_settings.hpp"
#include "aetherion/physics/integrators/integrator.hpp"

namespace aetherion::core {

struct RuntimeSettings {
    double physics_dt_s{1.0 / 120.0};
    double time_scale{1.0};
    std::size_t max_substeps{8};
    bool gravity_enabled{true};
    physics::em::ElectromagneticSettings electromagnetism;
    physics::IntegratorKind integrator{physics::IntegratorKind::semi_implicit_euler};
};

struct BodyPatch {
    std::optional<std::string> name;
    std::optional<double> mass_kg;
    std::optional<double> charge_C;
    std::optional<double> radius_m;
    std::optional<bool> fixed;
    std::optional<InteractionMask> interactions;
    std::optional<math::Vec3d> position_m;
    std::optional<math::Vec3d> velocity_mps;
};

struct CreateBodyCommand {
    Body body;
};
struct DeleteBodyCommand {
    EntityId id{};
};
struct UpdateBodyCommand {
    EntityId id{};
    BodyPatch patch;
};
struct SetPhysicsDtCommand {
    double physics_dt_s{};
};
struct SetTimeScaleCommand {
    double time_scale{};
};
struct SetGravityEnabledCommand {
    bool enabled{};
};
struct SetElectromagneticSettingsCommand {
    physics::em::ElectromagneticSettings settings;
};
struct SetIntegratorCommand {
    physics::IntegratorKind integrator{};
};

using SimulationCommand =
    std::variant<CreateBodyCommand, DeleteBodyCommand, UpdateBodyCommand, SetPhysicsDtCommand,
                 SetTimeScaleCommand, SetGravityEnabledCommand, SetElectromagneticSettingsCommand,
                 SetIntegratorCommand>;

struct CommandEvent {
    std::uint64_t sequence{};
    double simulation_time_s{};
    bool accepted{};
    std::string description;
    std::string detail;
};

struct CommandApplyReport {
    std::size_t accepted{};
    std::size_t rejected{};
    std::vector<EntityId> created_ids;
};

/// Thread-safe FIFO. Draining is restricted to explicit simulation-step boundaries.
class CommandQueue final {
  public:
    void enqueue(SimulationCommand command);
    [[nodiscard]] CommandApplyReport apply(Scene& scene, RuntimeSettings& settings,
                                           double simulation_time_s);
    /// Discards unapplied edits when an explicit checkpoint restore replaces the current state.
    [[nodiscard]] std::size_t discardPending();
    [[nodiscard]] std::size_t pendingCount() const;
    [[nodiscard]] const std::vector<CommandEvent>& eventLog() const noexcept { return event_log_; }

  private:
    mutable std::mutex mutex_;
    std::vector<SimulationCommand> pending_;
    std::vector<CommandEvent> event_log_;
    std::uint64_t next_sequence_{1};
};

} // namespace aetherion::core
