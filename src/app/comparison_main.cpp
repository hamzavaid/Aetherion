#include "aetherion/physics/integrator_comparison.hpp"
#include "aetherion/presets/mechanics_presets.hpp"

#include <cstddef>
#include <iomanip>
#include <iostream>

int main() {
    constexpr std::size_t steps = 365;
    const double dt_s = aetherion::presets::earth_like_orbit_period_s / static_cast<double>(steps);
    const auto report = aetherion::physics::IntegratorComparison::run(
        aetherion::presets::makeEarthLikeOrbit(), dt_s, steps);
    std::cout << std::setprecision(17)
              << "integrator,maximum_relative_energy_error,relative_orbit_reference_error,"
                 "maximum_momentum_error_kg_mps\n";
    for (const auto& run : report.runs) {
        std::cout << aetherion::physics::integratorName(run.integrator) << ','
                  << run.maximum_relative_energy_error << ',' << run.relative_orbit_reference_error
                  << ',' << run.maximum_momentum_error_kg_mps << '\n';
    }
    const auto& euler =
        report.forIntegrator(aetherion::physics::IntegratorKind::semi_implicit_euler);
    const auto& verlet = report.forIntegrator(aetherion::physics::IntegratorKind::velocity_verlet);
    const auto& rk4 = report.forIntegrator(aetherion::physics::IntegratorKind::rk4);
    const bool expected =
        verlet.maximum_relative_energy_error < euler.maximum_relative_energy_error * 0.1 &&
        rk4.maximum_relative_energy_error < verlet.maximum_relative_energy_error * 0.1 &&
        rk4.relative_orbit_reference_error < verlet.relative_orbit_reference_error;
    return expected ? 0 : 1;
}
