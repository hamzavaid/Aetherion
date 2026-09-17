#include "aetherion/physics/em/lorentz.hpp"

#include <cmath>
#include <numbers>
#include <stdexcept>
#include <utility>

#include "aetherion/physics/em/electrostatics.hpp"

namespace aetherion::physics::em {

core::Result<LorentzForceComponents> lorentzForce(double charge_C, const math::Vec3d& velocity_mps,
                                                  const fields::FieldSample& field) {
    if (!std::isfinite(charge_C) || !velocity_mps.isFinite() || !field.valid ||
        !field.electric_Vpm.isFinite() || !field.magnetic_T.isFinite()) {
        return core::Error{core::ErrorCode::invalid_argument,
                           "Lorentz force requires a finite valid SI field and particle state"};
    }
    LorentzForceComponents result;
    result.electric_N = charge_C * field.electric_Vpm;
    result.magnetic_N = charge_C * math::cross(velocity_mps, field.magnetic_T);
    result.total_N = result.electric_N + result.magnetic_N;
    if (!result.total_N.isFinite())
        return core::Error{core::ErrorCode::numerical_failure, "Lorentz force overflowed"};
    return result;
}

core::Result<GyroDiagnostics> gyroDiagnostics(double mass_kg, double charge_C,
                                              const math::Vec3d& velocity_mps,
                                              const math::Vec3d& magnetic_T) {
    if (!std::isfinite(mass_kg) || mass_kg <= 0.0 || !std::isfinite(charge_C) || charge_C == 0.0 ||
        !velocity_mps.isFinite() || !magnetic_T.isFinite()) {
        return core::Error{
            core::ErrorCode::invalid_argument,
            "gyro diagnostics require positive mass, nonzero charge, and finite vectors"};
    }
    const double magnetic_magnitude = magnetic_T.norm();
    if (magnetic_magnitude <= 0.0)
        return core::Error{core::ErrorCode::invalid_argument,
                           "gyro diagnostics require nonzero magnetic field in T"};
    const auto direction = magnetic_T / magnetic_magnitude;
    const auto perpendicular = velocity_mps - math::dot(velocity_mps, direction) * direction;
    const double angular_frequency = std::abs(charge_C) * magnetic_magnitude / mass_kg;
    return GyroDiagnostics{.radius_m = perpendicular.norm() / angular_frequency,
                           .angular_frequency_rad_ps = angular_frequency,
                           .period_s = 2.0 * std::numbers::pi / angular_frequency};
}

LorentzSolver::LorentzSolver(ElectromagneticSettings settings) : settings_(std::move(settings)) {
    const auto status = validateElectromagneticSettings(settings_);
    if (!status)
        throw std::invalid_argument(status.error().message);
}

void LorentzSolver::setSettings(const ElectromagneticSettings& settings) {
    const auto status = validateElectromagneticSettings(settings);
    if (!status)
        throw std::invalid_argument(status.error().message);
    settings_ = settings;
}

core::Status LorentzSolver::accumulateAccelerations(core::Scene& scene, double time_s,
                                                    bool include_electric, bool include_magnetic) {
    diagnostics_ = {};
    for (auto& body : scene.bodies()) {
        if (body.fixed || body.charge_C == 0.0)
            continue;
        if (body.mass_kg <= 0.0)
            return core::Error{core::ErrorCode::invalid_argument,
                               "dynamic charged bodies require positive mass in kg"};
        const bool electric =
            include_electric && body.interactions.contains(core::Interaction::electrostatic);
        const bool magnetic =
            include_magnetic && body.interactions.contains(core::Interaction::magnetic);
        if (!electric && !magnetic)
            continue;
        auto sample = sampleAnalyticField(settings_, body.state.position_m, time_s);
        if (!sample.valid)
            return core::Error{core::ErrorCode::numerical_failure,
                               "particle entered an analytic field-source singularity guard"};
        if (!electric)
            sample.electric_Vpm = {};
        if (!magnetic)
            sample.magnetic_T = {};
        const auto force = lorentzForce(body.charge_C, body.state.velocity_mps, sample);
        if (!force)
            return force.error();
        body.state.acceleration_mps2 += force.value().total_N / body.mass_kg;
        diagnostics_.total_electric_force_N += force.value().electric_N;
        diagnostics_.total_magnetic_force_N += force.value().magnetic_N;
        ++diagnostics_.evaluated_bodies;
    }
    return core::success();
}

} // namespace aetherion::physics::em
