#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "aetherion/core/error.hpp"
#include "aetherion/core/scene.hpp"
#include "aetherion/physics/fields/field_provider.hpp"

namespace aetherion::core {

enum class ReferenceFrame { world, center_of_mass, selected_body };

struct MassCenter {
    math::Vec3d position_m;
    math::Vec3d velocity_mps;
    double total_mass_kg{};
};

/// Mass-weighted world-frame center (m) and bulk velocity (m/s), using positive body masses.
/// Absent for an empty/zero-mass or nonfinite aggregate; no orbital barycenter approximation.
[[nodiscard]] std::optional<MassCenter> centerOfMass(const Scene& scene);

struct BodyObservation {
    EntityId id{};
    math::Vec3d position_m;
    math::Vec3d velocity_mps;
    math::Vec3d acceleration_mps2;
    math::Vec3d net_force_N;
    double speed_mps{};
    double kinetic_energy_J{};
};

/// Reports body position (m) and velocity (m/s) in a translating, world-aligned reference frame.
/// Subtracts COM or anchor-body position/velocity only; it applies no rotation or fictitious
/// acceleration. Force (N) and acceleration (m/s^2) remain inertial; force is latest `m*a`.
/// Missing bodies/anchors or nonfinite derived quantities return no observation.
[[nodiscard]] std::optional<BodyObservation>
observeBody(const Scene& scene, EntityId id, ReferenceFrame frame,
            std::optional<EntityId> frame_body_id = std::nullopt);

struct FieldProbe {
    std::string name;
    math::Vec3d position_m;
    [[nodiscard]] bool operator==(const FieldProbe&) const = default;
};

struct ProbeObservation {
    double time_s{};
    math::Vec3d electric_Vpm;
    math::Vec3d magnetic_T;
    math::Vec3d gravity_mps2;
    bool electromagnetic_valid{true};
    bool gravity_valid{true};
};

/// Samples a fixed world-frame position (m) through the shared read-only field provider at time
/// (s). Returns E (V/m), B (T), and g (m/s^2) with independent validity flags; rejects invalid
/// probe definitions or nonfinite time without modifying solver state.
[[nodiscard]] std::optional<ProbeObservation>
sampleProbe(const physics::fields::IFieldProvider& provider, const FieldProbe& probe,
            double time_s);

struct TimedBodyObservation {
    double time_s{};
    BodyObservation body;
};

/// Bounded transient plot history sampled at UI frame times, not every physics substep. Equal-time
/// frames replace the latest sample; rewinds and changed definitions clear it. Stores at most 16
/// fixed-world probes and `capacity` samples per series; plot history is not scene serialization.
class ScientificHistory final {
  public:
    explicit ScientificHistory(std::size_t capacity = 2048);
    [[nodiscard]] Status record(const Scene& scene, const physics::fields::IFieldProvider& provider,
                                double time_s, std::optional<EntityId> selected_body,
                                const std::vector<FieldProbe>& probes, ReferenceFrame frame,
                                std::optional<EntityId> frame_body_id = std::nullopt);
    void clear() noexcept;
    [[nodiscard]] const std::vector<TimedBodyObservation>& bodySamples() const noexcept {
        return body_samples_;
    }
    [[nodiscard]] const std::vector<std::vector<ProbeObservation>>& probeSamples() const noexcept {
        return probe_samples_;
    }

  private:
    std::size_t capacity_;
    std::optional<EntityId> selected_body_;
    std::vector<FieldProbe> probes_;
    ReferenceFrame frame_{ReferenceFrame::world};
    std::optional<EntityId> frame_body_id_;
    std::vector<TimedBodyObservation> body_samples_;
    std::vector<std::vector<ProbeObservation>> probe_samples_;
    double last_time_s_{};
    bool has_time_{};
};

} // namespace aetherion::core
