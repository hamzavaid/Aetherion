#include "aetherion/renderer/mesh.hpp"

#include <cmath>
#include <numbers>
#include <stdexcept>

namespace aetherion::renderer {

SphereMesh makeUvSphere(std::uint32_t slices, std::uint32_t stacks) {
    if (slices < 3U || stacks < 2U) {
        throw std::invalid_argument("UV sphere requires at least 3 slices and 2 stacks");
    }
    SphereMesh mesh;
    mesh.vertices.reserve(static_cast<std::size_t>(slices + 1U) * (stacks + 1U));
    mesh.indices.reserve(static_cast<std::size_t>(slices) * stacks * 6U);
    for (std::uint32_t stack = 0; stack <= stacks; ++stack) {
        const double latitude =
            std::numbers::pi * static_cast<double>(stack) / static_cast<double>(stacks);
        const float y = static_cast<float>(std::cos(latitude));
        const float ring = static_cast<float>(std::sin(latitude));
        for (std::uint32_t slice = 0; slice <= slices; ++slice) {
            const double longitude =
                2.0 * std::numbers::pi * static_cast<double>(slice) / static_cast<double>(slices);
            const float x = ring * static_cast<float>(std::cos(longitude));
            const float z = ring * static_cast<float>(std::sin(longitude));
            mesh.vertices.push_back({x, y, z, x, y, z});
        }
    }
    for (std::uint32_t stack = 0; stack < stacks; ++stack) {
        for (std::uint32_t slice = 0; slice < slices; ++slice) {
            const std::uint32_t first = stack * (slices + 1U) + slice;
            const std::uint32_t second = first + slices + 1U;
            mesh.indices.insert(mesh.indices.end(),
                                {first, second, first + 1U, second, second + 1U, first + 1U});
        }
    }
    return mesh;
}

std::vector<GridLine> makeEngineeringGrid(float half_extent, float spacing) {
    if (!std::isfinite(half_extent) || !std::isfinite(spacing) || half_extent <= 0.0F ||
        spacing <= 0.0F) {
        throw std::invalid_argument("grid extent and spacing must be positive finite render units");
    }
    const int subdivisions = static_cast<int>(std::floor(half_extent / spacing));
    std::vector<GridLine> lines;
    lines.reserve(static_cast<std::size_t>(2 * (2 * subdivisions + 1)));
    for (int index = -subdivisions; index <= subdivisions; ++index) {
        const float coordinate = static_cast<float>(index) * spacing;
        lines.push_back(
            {{coordinate, 0.0F, -half_extent}, {coordinate, 0.0F, half_extent}, index == 0});
    }
    for (int index = -subdivisions; index <= subdivisions; ++index) {
        const float coordinate = static_cast<float>(index) * spacing;
        lines.push_back(
            {{-half_extent, 0.0F, coordinate}, {half_extent, 0.0F, coordinate}, index == 0});
    }
    return lines;
}

} // namespace aetherion::renderer
