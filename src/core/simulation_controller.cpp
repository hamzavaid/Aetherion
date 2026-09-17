#include "aetherion/core/simulation_controller.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace aetherion::core {

SimulationController::SimulationController(Scene initial_scene, SimulationControllerConfig config)
    : scene_(std::move(initial_scene)), initial_scene_(scene_), initial_settings_(config),
      settings_(config), simulation_(scene_, {.physics_dt_s = config.physics_dt_s}),
      clock_({config.physics_dt_s, config.time_scale, config.max_substeps}) {
    synchronizePhysicsSettings();
}

void SimulationController::synchronizePhysicsSettings() {
    simulation_.setPhysicsDt(settings_.physics_dt_s);
    simulation_.setGravityEnabled(settings_.gravity_enabled);
    simulation_.setElectromagneticSettings(settings_.electromagnetism);
    simulation_.setIntegrator(settings_.integrator);
}

void SimulationController::synchronizeClockSettings() {
    clock_.configure({settings_.physics_dt_s, settings_.time_scale, settings_.max_substeps});
}

void SimulationController::applyCommandsAtBoundary() {
    if (commands_.pendingCount() == 0U)
        return;
    static_cast<void>(commands_.apply(scene_, settings_, simulation_.timeSeconds()));
    synchronizePhysicsSettings();
}

Status SimulationController::update(double real_delta_s) {
    Status step_status = success();
    if (!playing_) {
        applyCommandsAtBoundary();
        synchronizeClockSettings();
        return step_status;
    }
    static_cast<void>(clock_.advance(real_delta_s, [&](double dt_s) {
        if (!step_status)
            return;
        applyCommandsAtBoundary();
        simulation_.setPhysicsDt(dt_s);
        step_status = simulation_.step();
    }));
    synchronizePhysicsSettings();
    synchronizeClockSettings();
    return step_status;
}

Status SimulationController::singleStep() {
    applyCommandsAtBoundary();
    synchronizeClockSettings();
    simulation_.setPhysicsDt(settings_.physics_dt_s);
    return simulation_.step();
}

CheckpointId SimulationController::saveCheckpoint() {
    applyCommandsAtBoundary();
    synchronizePhysicsSettings();
    synchronizeClockSettings();
    constexpr std::size_t maximum_checkpoints = 64U;
    if (checkpoints_.size() == maximum_checkpoints)
        checkpoints_.erase(checkpoints_.begin());
    const CheckpointId id = next_checkpoint_id_++;
    checkpoints_.push_back({id, scene_, settings_, simulation_.timeSeconds()});
    return id;
}

Status SimulationController::restoreCheckpoint(CheckpointId id) {
    const auto checkpoint = std::find_if(checkpoints_.begin(), checkpoints_.end(),
                                         [id](const auto& entry) { return entry.id == id; });
    if (checkpoint == checkpoints_.end()) {
        return Error{ErrorCode::not_found, "simulation checkpoint does not exist"};
    }
    restoreState(checkpoint->scene, checkpoint->settings, checkpoint->simulation_time_s);
    return success();
}

Status SimulationController::loadState(Scene scene, RuntimeSettings settings) {
    if (!std::isfinite(settings.physics_dt_s) || settings.physics_dt_s <= 0.0 ||
        !std::isfinite(settings.time_scale) || settings.time_scale < 0.0 ||
        settings.max_substeps == 0U) {
        return Error{ErrorCode::invalid_argument, "loaded runtime settings are invalid"};
    }
    const auto em_status = physics::em::validateElectromagneticSettings(settings.electromagnetism);
    if (!em_status)
        return em_status;
    initial_scene_ = scene;
    initial_settings_ = settings;
    checkpoints_.clear();
    restoreState(scene, settings, 0.0);
    return success();
}

void SimulationController::restoreState(const Scene& scene, const RuntimeSettings& settings,
                                        double time_s) {
    static_cast<void>(commands_.discardPending());
    scene_ = scene;
    settings_ = settings;
    simulation_.reset(time_s);
    clock_.reset();
    playing_ = false;
    synchronizePhysicsSettings();
    synchronizeClockSettings();
}

void SimulationController::reset() {
    if (!checkpoints_.empty()) {
        const auto& latest = checkpoints_.back();
        restoreState(latest.scene, latest.settings, latest.simulation_time_s);
        return;
    }
    restoreState(initial_scene_, initial_settings_, 0.0);
}

} // namespace aetherion::core
