#pragma once

#include <cstdint>
#include <vector>

#include "aetherion/renderer/types.hpp"

namespace aetherion::renderer {

struct SphereVertex {
    float x{};
    float y{};
    float z{};
    float nx{};
    float ny{};
    float nz{};
};

struct SphereMesh {
    std::vector<SphereVertex> vertices;
    std::vector<std::uint32_t> indices;
};

struct GridLine {
    Vec3f start;
    Vec3f end;
    bool major_axis{};
};

/// Generates a unit UV sphere with seam duplication for deterministic indexing.
[[nodiscard]] SphereMesh makeUvSphere(std::uint32_t slices, std::uint32_t stacks);
/// Generates lines on the y=0 plane; dimensions are visual render units, not physical feedback.
[[nodiscard]] std::vector<GridLine> makeEngineeringGrid(float half_extent, float spacing);

} // namespace aetherion::renderer
