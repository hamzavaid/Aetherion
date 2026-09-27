#pragma once

#include <optional>
#include <vector>

#include "aetherion/core/scene.hpp"
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

/// Converts latest accepted inertial SI body vectors to bounded visual arrow descriptions.
/// Net force is m*a; zero and nonfinite vectors are omitted. No physics state is changed.
[[nodiscard]] std::vector<MotionGlyph>
generateMotionGlyphs(const core::Scene& scene, const MotionGlyphSettings& settings,
                     std::optional<core::EntityId> selected_body, double camera_distance_m);

} // namespace aetherion::renderer
