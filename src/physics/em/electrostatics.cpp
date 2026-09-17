#include "aetherion/physics/em/electrostatics.hpp"

#include <bit>
#include <cmath>
#include <cstdint>
#include <stdexcept>

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
    if (!std::isfinite(settings.minimum_separation_m) || settings.minimum_separation_m < 0.0 ||
        !std::isfinite(settings.softening_m) || settings.softening_m < 0.0) {
        return core::Error{core::ErrorCode::invalid_argument,
                           "electromagnetic distance guards must be finite and non-negative in m"};
    }
    for (const auto& source : settings.analytic_sources) {
        if (!finiteSource(source)) {
            return core::Error{core::ErrorCode::invalid_argument,
                               "analytic field source values must be finite SI quantities"};
        }
    }
    return core::success();
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
    diagnostics_ = {};
    auto& bodies = scene.bodies();
    for (const auto& body : bodies) {
        if (!body.fixed && body.charge_C != 0.0 && body.mass_kg <= 0.0) {
            return core::Error{core::ErrorCode::invalid_argument,
                               "dynamic charged bodies require positive mass in kg"};
        }
    }
    const double minimum_squared = settings_.minimum_separation_m * settings_.minimum_separation_m;
    const double softening_squared = settings_.softening_m * settings_.softening_m;
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
            const double physical_distance_squared = displacement_m.squaredNorm();
            if (physical_distance_squared <= minimum_squared) {
                ++diagnostics_.guarded_pairs;
                continue;
            }
            const double distance_squared = physical_distance_squared + softening_squared;
            const double inverse_distance_cubed =
                1.0 / (distance_squared * std::sqrt(distance_squared));
            const auto force_on_lhs_N = -constants::coulomb_constant * lhs.charge_C * rhs.charge_C *
                                        displacement_m * inverse_distance_cubed;
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
    return core::success();
}

ElectromagneticFieldProvider::ElectromagneticFieldProvider(
    const core::Scene& scene, const ElectromagneticSettings& settings) noexcept
    : scene_(scene), settings_(settings) {}

fields::FieldSample ElectromagneticFieldProvider::sample(const math::Vec3d& position_m,
                                                         double time_s) const {
    if (!position_m.isFinite() || !std::isfinite(time_s))
        return {.valid = false};
    fields::FieldSample result;
    const double guard_squared = settings_.minimum_separation_m * settings_.minimum_separation_m;
    const double softening_squared = settings_.softening_m * settings_.softening_m;
    for (const auto& source : scene_.bodies()) {
        if (source.charge_C == 0.0 ||
            !source.interactions.contains(core::Interaction::electrostatic)) {
            continue;
        }
        const auto displacement_m = position_m - source.state.position_m;
        const double physical_distance_squared = displacement_m.squaredNorm();
        if (physical_distance_squared <= guard_squared)
            return {.valid = false};
        const double distance_squared = physical_distance_squared + softening_squared;
        const double inverse_distance_cubed =
            1.0 / (distance_squared * std::sqrt(distance_squared));
        result.electric_Vpm +=
            constants::coulomb_constant * source.charge_C * displacement_m * inverse_distance_cubed;
    }
    for (const auto& source : settings_.analytic_sources) {
        if (source.kind == AnalyticFieldSourceKind::uniform) {
            result.electric_Vpm += source.electric_Vpm;
            result.magnetic_T += source.magnetic_T;
            continue;
        }
        const auto displacement_m = position_m - source.position_m;
        const double distance_squared = displacement_m.squaredNorm();
        if (distance_squared <= source.singularity_radius_m * source.singularity_radius_m)
            return {.valid = false};
        const double distance = std::sqrt(distance_squared);
        const auto direction = displacement_m / distance;
        const double coefficient = constants::vacuum_permeability /
                                   (4.0 * 3.14159265358979323846 * distance_squared * distance);
        result.magnetic_T +=
            coefficient *
            (3.0 * math::dot(source.magnetic_dipole_moment_Am2, direction) * direction -
             source.magnetic_dipole_moment_Am2);
    }
    if (!result.electric_Vpm.isFinite() || !result.magnetic_T.isFinite())
        return {.valid = false};
    return result;
}

std::uint64_t ElectromagneticFieldProvider::revision() const noexcept {
    std::uint64_t hash = 14'695'981'039'346'656'037ULL;
    hashDouble(hash, settings_.minimum_separation_m);
    hashDouble(hash, settings_.softening_m);
    for (const auto& body : scene_.bodies()) {
        hashDouble(hash, static_cast<double>(body.id));
        hashDouble(hash, body.charge_C);
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
