/** @file
 * Camera-scale engineering-grid spacing chosen from bounded 1/2/5 increments in render units.
 */
#pragma once

#include <cstddef>

namespace aetherion::renderer {

struct AdaptiveGridParameters {
    double spacing_render_units{};
    double half_extent_render_units{};
    std::size_t subdivisions_each_direction{};
};

/// Chooses 1/2/5 engineering spacing from camera distance so the grid fills the viewport at any
/// zoom.
[[nodiscard]] AdaptiveGridParameters calculateAdaptiveGrid(double camera_distance_m,
                                                           double meters_to_render_units);

} // namespace aetherion::renderer
