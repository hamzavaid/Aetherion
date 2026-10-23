/** @file
 * Bounded source trajectories and finite-delay Coulomb sampling without extrapolation or radiation
 * terms.
 */
#pragma once

#include <cstdint>
#include <map>
#include <string_view>
#include <vector>

#include "aetherion/core/scene.hpp"
#include "aetherion/physics/em/electromagnetic_settings.hpp"
#include "aetherion/physics/em/retarded_settings.hpp"
#include "aetherion/physics/fields/field_provider.hpp"

namespace aetherion::physics::em {

[[nodiscard]] core::Status validateRetardedScene(const core::Scene& scene,
                                                 const RetardedSettings& settings, double dt_s);

struct ParticleHistorySample {
    double time_s{};
    math::Vec3d position_m;
    math::Vec3d velocity_mps;
    math::Vec3d acceleration_mps2;
    [[nodiscard]] bool operator==(const ParticleHistorySample&) const = default;
};

struct HistoryBracket {
    ParticleHistorySample before;
    ParticleHistorySample after;
};

/// Fixed-capacity chronological ring in inertial SI coordinates. Negative times are allowed for
/// analytical fixtures. Equal-time writes replace only the newest sample. Lookup never
/// extrapolates.
class HistoryBuffer final {
  public:
    explicit HistoryBuffer(std::size_t capacity);
    [[nodiscard]] core::Status append(const ParticleHistorySample& sample);
    [[nodiscard]] core::Result<HistoryBracket> bracket(double time_s) const;
    /// Piecewise linear interpolation of position (m), velocity (m/s), acceleration (m/s^2).
    [[nodiscard]] core::Result<ParticleHistorySample> interpolate(double time_s) const;
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] std::size_t capacity() const noexcept { return samples_.size(); }
    [[nodiscard]] const ParticleHistorySample& oldest() const;
    [[nodiscard]] const ParticleHistorySample& newest() const;
    [[nodiscard]] std::uint64_t revision() const noexcept { return revision_; }

  private:
    [[nodiscard]] const ParticleHistorySample& at(std::size_t index) const;
    std::vector<ParticleHistorySample> samples_;
    std::size_t begin_{};
    std::size_t size_{};
    std::uint64_t revision_{};
};

enum class RetardedStatus {
    converged,
    history_underrun,
    history_future,
    non_converged,
    singularity,
    invalid_input,
    superluminal_source
};

struct RetardedResult {
    RetardedStatus status{RetardedStatus::invalid_input};
    double retarded_time_s{};
    double residual_s{};
    double distance_m{};
    std::size_t iterations{};
    ParticleHistorySample source;
};

/// Bounded fixed-point solve tr=t-|r-rq(tr)|/c. Uses available endpoint residuals to detect missing
/// history, then iterates within the bracket. Tolerance is absolute seconds plus relative flight
/// time and a double-roundoff margin. Rejects >=c trajectory segments/velocities and source guards.
[[nodiscard]] RetardedResult solveRetardedTime(const HistoryBuffer& history,
                                               const math::Vec3d& observer_m, double time_s,
                                               const RetardedSettings& settings);
[[nodiscard]] std::string_view retardedStatusName(RetardedStatus status) noexcept;

struct SourceTrajectory {
    double charge_C{};
    HistoryBuffer history;
};

/// Owns bounded charged-source histories. Record only accepted states and explicit baseline seeds.
/// Source removal/charge changes discard affected trajectories; edits and restores clear all
/// history.
class SourceHistories final {
  public:
    [[nodiscard]] core::Status configure(const RetardedSettings& settings, double physics_dt_s);
    [[nodiscard]] core::Status record(const core::Scene& scene, double time_s);
    void clear() noexcept;
    [[nodiscard]] const SourceTrajectory* find(core::EntityId id) const noexcept;
    [[nodiscard]] std::size_t capacityPerSource() const noexcept { return capacity_; }
    [[nodiscard]] std::size_t sourceCount() const noexcept { return sources_.size(); }
    [[nodiscard]] std::uint64_t revision() const noexcept { return revision_; }

  private:
    RetardedSettings settings_;
    std::size_t capacity_{4};
    double physics_dt_s_{};
    std::map<core::EntityId, SourceTrajectory> sources_;
    std::uint64_t revision_{};
};

struct RetardedDiagnostics {
    std::uint64_t queries{};
    std::uint64_t converged_sources{};
    std::uint64_t history_failures{};
    std::uint64_t convergence_failures{};
    std::uint64_t invalid_sources{};
    std::uint64_t guarded_sources{};
    std::size_t maximum_iterations_seen{};
    double maximum_residual_s{};
};

/// Phase 8 delayed Coulomb sampling preview, E=k*q*(r-rq(tr))/|r-rq(tr)|^3. Low-speed educational
/// approximation: full LW velocity/radiation E/B belongs to Phase 9. Analytic E/B and gravity are
/// retained; particle force integrators remain instantaneous. Missing source history invalidates
/// EM superposition rather than silently substituting present-time state.
class RetardedFieldProvider final : public fields::IFieldProvider {
  public:
    RetardedFieldProvider(const core::Scene& scene, const ElectromagneticSettings& settings,
                          const SourceHistories& histories,
                          const fields::IFieldProvider& instantaneous) noexcept;
    [[nodiscard]] fields::FieldSample sample(const math::Vec3d& position_m,
                                             double time_s) const override;
    [[nodiscard]] std::uint64_t revision() const noexcept override;
    [[nodiscard]] const RetardedDiagnostics& diagnostics() const noexcept { return diagnostics_; }
    void clearDiagnostics() noexcept { diagnostics_ = {}; }

  private:
    const core::Scene& scene_;
    const ElectromagneticSettings& settings_;
    const SourceHistories& histories_;
    const fields::IFieldProvider& instantaneous_;
    mutable RetardedDiagnostics diagnostics_;
};

} // namespace aetherion::physics::em
