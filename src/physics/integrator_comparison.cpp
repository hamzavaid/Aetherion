#include "aetherion/physics/integrator_comparison.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "aetherion/core/simulation.hpp"
#include "aetherion/physics/constants.hpp"

namespace aetherion::physics {
namespace {

math::Vec3d rotateAroundAxis(const math::Vec3d& vector, const math::Vec3d& axis, double angle_rad) {
    return vector * std::cos(angle_rad) + math::cross(axis, vector) * std::sin(angle_rad) +
           axis * (math::dot(axis, vector) * (1.0 - std::cos(angle_rad)));
}

IntegratorRun runOne(const core::Scene& initial, IntegratorKind kind, double dt_s,
                     std::size_t steps) {
    core::Scene scene = initial;
    core::Simulation simulation(scene, {.physics_dt_s = dt_s, .integrator = kind});
    IntegratorRun run{.integrator = kind};
    run.error_series.reserve(steps);
    for (std::size_t step = 0; step < steps; ++step) {
        const auto status = simulation.step();
        if (!status)
            throw std::runtime_error(status.error().message);
        const auto& sample = simulation.telemetry().samples().back();
        run.maximum_relative_energy_error =
            std::max(run.maximum_relative_energy_error, sample.relative_energy_error);
        run.maximum_momentum_error_kg_mps =
            std::max(run.maximum_momentum_error_kg_mps, sample.momentum_error_kg_mps);
        run.error_series.push_back(
            {sample.time_s, sample.relative_energy_error, sample.momentum_error_kg_mps});
    }
    run.final_scene = scene;
    if (initial.size() >= 2U) {
        const auto initial_relative =
            initial.bodies()[1].state.position_m - initial.bodies()[0].state.position_m;
        const auto final_relative =
            scene.bodies()[1].state.position_m - scene.bodies()[0].state.position_m;
        const double radius = initial_relative.norm();
        run.relative_orbit_closure_error = (final_relative - initial_relative).norm() / radius;
        const auto relative_velocity =
            initial.bodies()[1].state.velocity_mps - initial.bodies()[0].state.velocity_mps;
        const auto angular_axis = math::cross(initial_relative, relative_velocity).normalized();
        const double total_mass = initial.bodies()[0].mass_kg + initial.bodies()[1].mass_kg;
        const double angular_rate =
            std::sqrt(constants::gravitational_constant * total_mass / (radius * radius * radius));
        const auto expected = rotateAroundAxis(initial_relative, angular_axis,
                                               angular_rate * dt_s * static_cast<double>(steps));
        run.relative_orbit_reference_error = (final_relative - expected).norm() / radius;
    }
    return run;
}

} // namespace

const IntegratorRun& IntegratorComparisonReport::forIntegrator(IntegratorKind integrator) const {
    const auto found = std::find_if(runs.begin(), runs.end(), [integrator](const auto& run) {
        return run.integrator == integrator;
    });
    if (found == runs.end())
        throw std::out_of_range("integrator is absent from comparison report");
    return *found;
}

IntegratorComparisonReport IntegratorComparison::run(const core::Scene& initial_scene, double dt_s,
                                                     std::size_t steps) {
    if (!std::isfinite(dt_s) || dt_s <= 0.0 || steps == 0U) {
        throw std::invalid_argument("integrator comparison requires positive dt and step count");
    }
    IntegratorComparisonReport report{.dt_s = dt_s, .steps = steps};
    report.runs.reserve(3);
    report.runs.push_back(runOne(initial_scene, IntegratorKind::semi_implicit_euler, dt_s, steps));
    report.runs.push_back(runOne(initial_scene, IntegratorKind::velocity_verlet, dt_s, steps));
    report.runs.push_back(runOne(initial_scene, IntegratorKind::rk4, dt_s, steps));
    return report;
}

} // namespace aetherion::physics
