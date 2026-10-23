/** @file
 * Direct Coulomb interactions and read-only superposition of gravity, electric, and analytic
 * magnetic fields.
 */
#include "aetherion/physics/em/electrostatics.hpp"

#include <bit>
#include <cmath>
#include <cstdint>
#include <stdexcept>

#include "aetherion/core/state_transaction.hpp"
#include "aetherion/physics/constants.hpp"

namespace aetherion::physics::em {
namespace {

bool finiteSource(const AnalyticFieldSource& source) {
    return source.position_m.isFinite() && source.electric_Vpm.isFinite() &&
           source.magnetic_T.isFinite() && source.magnetic_dipole_moment_Am2.isFinite() &&
           std::isfinite(source.singularity_radius_m) && source.singularity_radius_m >= 0.0;
}

void hashDouble(std::uint64_t& hash, double value) noexcept {
    constexpr std::uint64_t fnv_prime = 1'099'511'628'211ULL;
    hash ^= std::bit_cast<std::uint64_t>(value);
    hash *= fnv_prime;
}

void hashVector(std::uint64_t& hash, const math::Vec3d& value) noexcept {
    hashDouble(hash, value.x);
    hashDouble(hash, value.y);
    hashDouble(hash, value.z);
}

} // namespace

core::Status validateElectromagneticSettings(const ElectromagneticSettings& settings) {
    const auto retarded_status = validateRetardedSettings(settings.retarded);
    if (!retarded_status)
        return retarded_status;
    if (!std::isfinite(settings.minimum_separation_m) || settings.minimum_separation_m < 0.0 ||
        !std::isfinite(settings.softening_m) || settings.softening_m < 0.0) {
        return core::Error{core::ErrorCode::invalid_argument,
                           "electromagnetic distance guards must be finite and non-negative in m"};
    }
    for (const auto& source : settings.analytic_sources) {
        if (!finiteSource(source) || (source.kind != AnalyticFieldSourceKind::uniform &&
                                      source.kind != AnalyticFieldSourceKind::magnetic_dipole)) {
            return core::Error{core::ErrorCode::invalid_argument,
                               "analytic field source values must be finite SI quantities"};
        }
    }
    return core::success();
}

core::Result<math::Vec3d> samplePointChargeField(double charge_C, const math::Vec3d& source_m,
                                                 const math::Vec3d& observer_m,
                                                 const ElectromagneticSettings& settings) {
    if (!std::isfinite(charge_C) || !source_m.isFinite() || !observer_m.isFinite())
        return core::Error{core::ErrorCode::invalid_argument,
                           "point-charge sample requires finite SI inputs"};
    if (!std::isfinite(settings.minimum_separation_m) || settings.minimum_separation_m < 0.0 ||
        !std::isfinite(settings.softening_m) || settings.softening_m < 0.0)
        return core::Error{core::ErrorCode::invalid_argument,
                           "point-charge guards require finite nonnegative meters"};
    if (charge_C == 0.0)
        return math::Vec3d{};
    const auto displacement = observer_m - source_m;
    const double distance_m = displacement.norm();
    if (distance_m <= settings.minimum_separation_m)
        return core::Error{core::ErrorCode::numerical_failure,
                           "point-charge sample inside source guard"};
    const double softened_distance_m = std::hypot(distance_m, settings.softening_m);
    if (!std::isfinite(softened_distance_m) || softened_distance_m <= 0.0)
        return core::Error{core::ErrorCode::numerical_failure,
                           "point-charge distance outside finite range"};
    // E=k(q/r_soft)(1/r_soft)(r/r_soft), avoiding unrepresentable r³ and k*q intermediates.
    const double magnitude_factor =
        (charge_C / softened_distance_m) * (constants::coulomb_constant / softened_distance_m);
    const auto field = (displacement / softened_distance_m) * magnitude_factor;
    if (!field.isFinite())
        return core::Error{core::ErrorCode::numerical_failure, "point-charge sample overflow"};
    return field;
}

ElectrostaticSolver::ElectrostaticSolver(ElectromagneticSettings settings)
    : settings_(std::move(settings)) {
    const auto status = validateElectromagneticSettings(settings_);
    if (!status)
        throw std::invalid_argument(status.error().message);
}

void ElectrostaticSolver::setSettings(const ElectromagneticSettings& settings) {
    const auto status = validateElectromagneticSettings(settings);
    if (!status)
        throw std::invalid_argument(status.error().message);
    settings_ = settings;
}

core::Status ElectrostaticSolver::accumulateAccelerations(core::Scene& scene) {
    core::StateTransaction transaction(scene);
    diagnostics_ = {};
    auto& bodies = scene.bodies();
    for (const auto& body : bodies) {
        if (body.interactions.contains(core::Interaction::electrostatic) && !body.fixed &&
            body.charge_C != 0.0 && body.mass_kg <= 0.0) {
            return core::Error{core::ErrorCode::invalid_argument,
                               "dynamic charged bodies require positive mass in kg"};
        }
    }
    for (std::size_t first = 0; first < bodies.size(); ++first) {
        for (std::size_t second = first + 1U; second < bodies.size(); ++second) {
            auto& lhs = bodies[first];
            auto& rhs = bodies[second];
            if (!lhs.interactions.contains(core::Interaction::electrostatic) ||
                !rhs.interactions.contains(core::Interaction::electrostatic) ||
                lhs.charge_C == 0.0 || rhs.charge_C == 0.0) {
                continue;
            }
            const auto displacement_m = rhs.state.position_m - lhs.state.position_m;
            if (displacement_m.norm() <= settings_.minimum_separation_m) {
                ++diagnostics_.guarded_pairs;
                continue;
            }
            const auto field_on_lhs = samplePointChargeField(rhs.charge_C, rhs.state.position_m,
                                                             lhs.state.position_m, settings_);
            if (!field_on_lhs)
                return field_on_lhs.error();
            const auto force_on_lhs_N = lhs.charge_C * field_on_lhs.value();
            if (!force_on_lhs_N.isFinite())
                return core::Error{core::ErrorCode::numerical_failure, "Coulomb force overflowed"};
            if (!lhs.fixed)
                lhs.state.acceleration_mps2 += force_on_lhs_N / lhs.mass_kg;
            if (!rhs.fixed)
                rhs.state.acceleration_mps2 -= force_on_lhs_N / rhs.mass_kg;
            ++diagnostics_.evaluated_pairs;
        }
    }
    for (const auto& body : bodies) {
        if (!body.state.acceleration_mps2.isFinite()) {
            return core::Error{core::ErrorCode::numerical_failure,
                               "Coulomb interaction generated non-finite acceleration"};
        }
    }
    transaction.commit();
    return core::success();
}

ElectromagneticFieldProvider::ElectromagneticFieldProvider(
    const core::Scene& scene, const ElectromagneticSettings& settings) noexcept
    : scene_(scene), settings_(settings) {}

fields::FieldSample sampleAnalyticField(const ElectromagneticSettings& settings,
                                        const math::Vec3d& position_m, double time_s) {
    if (!position_m.isFinite() || !std::isfinite(time_s))
        return {.valid = false};
    fields::FieldSample result;
    for (const auto& source : settings.analytic_sources) {
        if (source.kind == AnalyticFieldSourceKind::uniform) {
            result.electric_Vpm += source.electric_Vpm;
            result.magnetic_T += source.magnetic_T;
            continue;
        }
        const auto displacement_m = position_m - source.position_m;
        const double distance = displacement_m.norm();
        if (!std::isfinite(distance) || distance <= source.singularity_radius_m)
            return {.valid = false};
        const auto direction = displacement_m / distance;
        // Scale the moment before repeated division so neither r³ nor its raw contraction
        // must fit as an intermediate value.
        const double coefficient = constants::vacuum_permeability / (4.0 * 3.14159265358979323846);
        const auto scaled_moment =
            ((source.magnetic_dipole_moment_Am2 * coefficient / distance) / distance) / distance;
        result.magnetic_T += 3.0 * math::dot(scaled_moment, direction) * direction - scaled_moment;
    }
    if (!result.electric_Vpm.isFinite() || !result.magnetic_T.isFinite())
        return {.valid = false};
    return result;
}

fields::FieldSample ElectromagneticFieldProvider::sample(const math::Vec3d& position_m,
                                                         double time_s) const {
    if (!position_m.isFinite() || !std::isfinite(time_s))
        return {.valid = false, .gravity_valid = false};
    auto result = sampleAnalyticField(settings_, position_m, time_s);
    if (result.valid) {
        for (const auto& source : scene_.bodies()) {
            if (source.charge_C == 0.0 ||
                !source.interactions.contains(core::Interaction::electrostatic)) {
                continue;
            }
            const auto field = samplePointChargeField(source.charge_C, source.state.position_m,
                                                      position_m, settings_);
            if (!field) {
                result.valid = false;
                break;
            }
            result.electric_Vpm += field.value();
        }
    }
    for (const auto& source : scene_.bodies()) {
        if (source.mass_kg == 0.0 || !source.interactions.contains(core::Interaction::gravity))
            continue;
        const auto toward_source_m = source.state.position_m - position_m;
        const double distance_m = toward_source_m.norm();
        if (!std::isfinite(distance_m) || distance_m <= source.radius_m) {
            result.gravity_valid = false;
            break;
        }
        const double magnitude =
            (constants::gravitational_constant * source.mass_kg / distance_m) / distance_m;
        result.gravity_mps2 += (toward_source_m / distance_m) * magnitude;
    }
    if (!result.electric_Vpm.isFinite() || !result.magnetic_T.isFinite())
        result.valid = false;
    if (!result.gravity_mps2.isFinite())
        result.gravity_valid = false;
    return result;
}

std::uint64_t ElectromagneticFieldProvider::revision() const noexcept {
    std::uint64_t hash = 14'695'981'039'346'656'037ULL;
    hashDouble(hash, settings_.minimum_separation_m);
    hashDouble(hash, settings_.softening_m);
    for (const auto& body : scene_.bodies()) {
        hashDouble(hash, static_cast<double>(body.id));
        hashDouble(hash, body.mass_kg);
        hashDouble(hash, body.charge_C);
        hashDouble(hash, body.radius_m);
        hashVector(hash, body.state.position_m);
        hashDouble(hash, static_cast<double>(body.interactions.bits));
    }
    for (const auto& source : settings_.analytic_sources) {
        hashDouble(hash,
                   static_cast<double>(source.kind == AnalyticFieldSourceKind::uniform ? 0 : 1));
        hashVector(hash, source.position_m);
        hashVector(hash, source.electric_Vpm);
        hashVector(hash, source.magnetic_T);
        hashVector(hash, source.magnetic_dipole_moment_Am2);
        hashDouble(hash, source.singularity_radius_m);
    }
    return hash;
}

} // namespace aetherion::physics::em
