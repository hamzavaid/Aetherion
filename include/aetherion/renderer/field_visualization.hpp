#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "aetherion/core/scene.hpp"
#include "aetherion/physics/fields/field_provider.hpp"

namespace aetherion::renderer {

enum class FieldDisplayMode { none, observed_vectors, field_lines };
enum class ObservedField { electric, magnetic };
enum class SamplingGeometry { volume, plane_xy, plane_xz, plane_yz };
enum class VectorScaling { normalized, logarithmic, linear };

struct FieldRegion {
    math::Vec3d center_m;
    math::Vec3d half_extent_m{10.0, 10.0, 10.0};
};

struct VectorFieldSettings {
    SamplingGeometry geometry{SamplingGeometry::plane_xz};
    std::size_t resolution{9};
    double visual_length_m{1.0};
    VectorScaling scaling{VectorScaling::normalized};
    double reference_magnitude{1.0};
    double minimum_magnitude{};
    double maximum_magnitude{1.0e300};
};

struct FieldLineSettings {
    std::size_t automatic_seed_count{16};
    std::vector<math::Vec3d> custom_seeds_m;
    double step_size_m{0.1};
    std::size_t maximum_steps{500};
    std::size_t maximum_total_steps{50'000};
    double maximum_length_m{50.0};
    double minimum_field_magnitude{1.0e-18};
    bool trace_forward{true};
    bool trace_backward{true};
};

struct FieldVisualizationSettings {
    FieldDisplayMode mode{FieldDisplayMode::none};
    ObservedField field{ObservedField::electric};
    FieldRegion region;
    VectorFieldSettings vectors;
    FieldLineSettings lines;
};

struct FieldVectorGlyph {
    math::Vec3d position_m;
    math::Vec3d direction;
    double magnitude{};
    double visual_length_m{};
};

struct TracedFieldLine {
    std::vector<math::Vec3d> points_m;
};

/// Samples a bounded plane or volume and creates finite SI-space vector glyph descriptions.
[[nodiscard]] std::vector<FieldVectorGlyph>
sampleObservedField(const physics::fields::IFieldProvider& provider,
                    const FieldVisualizationSettings& settings, double time_s);

/// Traces normalized field tangents with visualization-only RK4 and a global work budget.
[[nodiscard]] std::vector<TracedFieldLine>
traceFieldLines(const physics::fields::IFieldProvider& provider,
                const FieldVisualizationSettings& settings,
                const std::vector<math::Vec3d>& seed_points_m, double time_s);

/// Deterministically creates useful seeds around electric sources or across a magnetic slice.
[[nodiscard]] std::vector<math::Vec3d>
generateAutomaticFieldSeeds(const core::Scene& scene, const FieldVisualizationSettings& settings);

/// Stable hash of visualization controls for renderer cache invalidation.
[[nodiscard]] std::uint64_t
fieldVisualizationRevision(const FieldVisualizationSettings& settings) noexcept;

} // namespace aetherion::renderer
