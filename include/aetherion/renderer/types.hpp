#pragma once

#include <array>

#include "aetherion/math/vec3d.hpp"

namespace aetherion::renderer {

struct Vec3f {
    float x{};
    float y{};
    float z{};
};

struct Ray {
    math::Vec3d origin;
    math::Vec3d direction;
};

using Mat4f = std::array<float, 16>;

} // namespace aetherion::renderer
