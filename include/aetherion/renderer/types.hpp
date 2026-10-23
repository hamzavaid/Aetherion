/** @file
 * Presentation-only float vectors, column-major matrices, and double-precision world-space rays.
 */
#pragma once

#include <array>

#include "aetherion/math/vec3d.hpp"

namespace aetherion::renderer {

struct Vec3f {
    float x{};
    float y{};
    float z{};
};

/// Inertial-world ray: origin in meters, direction dimensionless (picking normalizes it).
struct Ray {
    math::Vec3d origin;
    math::Vec3d direction;
};

using Mat4f = std::array<float, 16>; // Column-major OpenGL matrix, presentation units only.

} // namespace aetherion::renderer
