#include "aetherion/renderer/mesh.hpp"

#include <gtest/gtest.h>

#include <cmath>

using aetherion::renderer::makeEngineeringGrid;
using aetherion::renderer::makeUvSphere;

TEST(Mesh, UvSphereHasDeterministicTopologyAndUnitNormals) {
    const auto sphere = makeUvSphere(16, 8);
    EXPECT_EQ(sphere.vertices.size(), 153U);
    EXPECT_EQ(sphere.indices.size(), 768U);
    for (const auto& vertex : sphere.vertices) {
        const double norm = std::sqrt(static_cast<double>(
            vertex.nx * vertex.nx + vertex.ny * vertex.ny + vertex.nz * vertex.nz));
        EXPECT_NEAR(norm, 1.0, 1.0e-6);
    }
}

TEST(Mesh, EngineeringGridProducesXZLinesWithMajorAxisFlags) {
    const auto grid = makeEngineeringGrid(10.0F, 1.0F);
    EXPECT_EQ(grid.size(), 42U);
    EXPECT_TRUE(grid[10].major_axis);
    EXPECT_TRUE(grid[31].major_axis);
}
