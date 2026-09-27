#include "aetherion/renderer/diagnostic_visualization.hpp"

#include <algorithm>
#include <cmath>

namespace aetherion::renderer {
namespace {

void addGlyph(std::vector<MotionGlyph>& output, MotionGlyphKind kind, const math::Vec3d& position_m,
              const math::Vec3d& vector, double reference_magnitude, double base_length_m,
              VectorScaling scaling) {
    const double magnitude = vector.norm();
    if (!vector.isFinite() || !std::isfinite(magnitude) || magnitude <= 0.0 ||
        !std::isfinite(reference_magnitude) || reference_magnitude <= 0.0)
        return;
    double factor = 1.0;
    if (scaling == VectorScaling::linear)
        factor = magnitude / reference_magnitude;
    else if (scaling == VectorScaling::logarithmic)
        factor = std::log10(1.0 + magnitude / reference_magnitude);
    if (!std::isfinite(factor))
        return;
    const double length = base_length_m * std::clamp(factor, 0.02, 10.0);
    if (std::isfinite(length) && length > 0.0)
        output.push_back({kind, position_m, vector / magnitude, magnitude, length});
}

} // namespace

std::vector<MotionGlyph> generateMotionGlyphs(const core::Scene& scene,
                                              const MotionGlyphSettings& settings,
                                              std::optional<core::EntityId> selected_body,
                                              double camera_distance_m) {
    std::vector<MotionGlyph> output;
    if (!std::isfinite(camera_distance_m) || camera_distance_m <= 0.0 ||
        !std::isfinite(settings.length_fraction) || settings.length_fraction <= 0.0 ||
        settings.length_fraction > 1.0 || (settings.selected_only && !selected_body))
        return output;
    const double base_length_m = camera_distance_m * settings.length_fraction;
    output.reserve(scene.size() * 3U);
    for (const auto& body : scene.bodies()) {
        if (settings.selected_only && body.id != selected_body)
            continue;
        if (settings.velocity)
            addGlyph(output, MotionGlyphKind::velocity, body.state.position_m,
                     body.state.velocity_mps, settings.velocity_reference_mps, base_length_m,
                     settings.scaling);
        if (settings.force)
            addGlyph(output, MotionGlyphKind::force, body.state.position_m,
                     body.mass_kg * body.state.acceleration_mps2, settings.force_reference_N,
                     base_length_m, settings.scaling);
        if (settings.acceleration)
            addGlyph(output, MotionGlyphKind::acceleration, body.state.position_m,
                     body.state.acceleration_mps2, settings.acceleration_reference_mps2,
                     base_length_m, settings.scaling);
    }
    return output;
}

} // namespace aetherion::renderer
