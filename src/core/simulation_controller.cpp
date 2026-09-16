#include "aetherion/core/simulation_controller.hpp"

#include <utility>

namespace aetherion::core {

SimulationController::SimulationController(Scene initial_scene, SimulationControllerConfig config)
    : scene_(std::move(initial_scene)), initial_scene_(scene_), settings_(config),
      simulation_(scene_, {.physics_dt_s = config.physics_dt_s}),
      clock_({config.physics_dt_s, config.time_scale, config.max_substeps}) {
    synchronizePhysicsSettings();
}

void SimulationController::synchronizePhysicsSettings() {
    simulation_.setPhysicsDt(settings_.physics_dt_s);
    simulation_.setGravityEnabled(settings_.gravity_enabled);
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

void SimulationController::reset() {
    scene_ = initial_scene_;
    simulation_.reset();
    clock_.reset();
    playing_ = false;
    applyCommandsAtBoundary();
    synchronizePhysicsSettings();
    synchronizeClockSettings();
}

} // namespace aetherion::core
