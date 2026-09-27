#pragma once

#include <optional>
#include <vector>

#include "aetherion/core/scene.hpp"
#include "aetherion/core/scientific_analysis.hpp"
#include "aetherion/renderer/field_visualization.hpp"

namespace aetherion::renderer {

enum class MotionGlyphKind { velocity, force, acceleration };

struct MotionGlyphSettings {
    bool velocity{};
    bool force{};
    bool acceleration{};
    bool selected_only{true};
    /// Glyph length is this fraction of camera distance; purely visual and dimensionless.
    double length_fraction{0.1};
    VectorScaling scaling{VectorScaling::normalized};
    double velocity_reference_mps{1.0};
    double force_reference_N{1.0};
    double acceleration_reference_mps2{1.0};
};

struct MotionGlyph {
    MotionGlyphKind kind{};
    math::Vec3d position_m;
    math::Vec3d direction;
    double magnitude_SI{};
    double visual_length_m{};
};

struct ProbeMarker {
    math::Vec3d position_m;
    bool selected{};
};

struct ProbeVectorGlyph {
    ObservedField field{};
    math::Vec3d position_m;
    math::Vec3d direction;
    double magnitude_SI{};
    double visual_length_m{};
};

struct ProbeDisplay {
    std::vector<ProbeMarker> markers;
    std::vector<ProbeVectorGlyph> vectors;
};

/// Samples fixed probes through the read-only provider for finite E/B/g arrows. Probe markers
/// remain visible even when the selected field is singular; lengths are camera-relative display
/// fractions and do not feed back into the solver.
[[nodiscard]] ProbeDisplay generateProbeDisplay(const physics::fields::IFieldProvider& provider,
                                                const std::vector<core::FieldProbe>& probes,
                                                double time_s, double camera_distance_m,
                                                std::optional<std::size_t> selected_probe);

/// Converts latest accepted inertial SI body vectors to bounded visual arrow descriptions.
/// Net force is m*a; zero and nonfinite vectors are omitted. No physics state is changed.
[[nodiscard]] std::vector<MotionGlyph>
generateMotionGlyphs(const core::Scene& scene, const MotionGlyphSettings& settings,
                     std::optional<core::EntityId> selected_body, double camera_distance_m);

} // namespace aetherion::renderer
