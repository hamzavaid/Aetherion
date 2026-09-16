#include "aetherion/renderer/adaptive_grid.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace aetherion::renderer {
namespace {

double niceCeiling(double value) {
    const double magnitude = std::pow(10.0, std::floor(std::log10(value)));
    const double normalized = value / magnitude;
    if (normalized <= 1.0)
        return magnitude;
    if (normalized <= 2.0)
        return 2.0 * magnitude;
    if (normalized <= 5.0)
        return 5.0 * magnitude;
    return 10.0 * magnitude;
}

} // namespace

AdaptiveGridParameters calculateAdaptiveGrid(double camera_distance_m,
                                             double meters_to_render_units) {
    if (!std::isfinite(camera_distance_m) || camera_distance_m <= 0.0 ||
        !std::isfinite(meters_to_render_units) || meters_to_render_units <= 0.0) {
        throw std::invalid_argument(
            "adaptive grid requires positive finite camera distance and scale");
    }
    const double rendered_distance = camera_distance_m * meters_to_render_units;
    const double spacing = niceCeiling(rendered_distance / 12.0);
    const double minimum_extent = rendered_distance * 3.0;
    const std::size_t subdivisions =
        std::clamp(static_cast<std::size_t>(std::ceil(minimum_extent / spacing)), std::size_t{16},
                   std::size_t{64});
    return {.spacing_render_units = spacing,
            .half_extent_render_units = spacing * static_cast<double>(subdivisions),
            .subdivisions_each_direction = subdivisions};
}

} // namespace aetherion::renderer
