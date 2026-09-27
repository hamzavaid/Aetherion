#include "aetherion/renderer/diagnostic_visualization.hpp"

#include <gtest/gtest.h>

#include "aetherion/physics/em/electrostatics.hpp"

using namespace aetherion;

TEST(DiagnosticVisualization, MotionGlyphsUseLatestSiBodyStateAndBoundedVisualScale) {
    core::Scene scene;
    const auto id = scene.createBody(
        {.name = "moving",
         .mass_kg = 2.0,
         .radius_m = 0.1,
         .state = {.velocity_mps = {3.0, 0.0, 0.0}, .acceleration_mps2 = {0.0, 4.0, 0.0}}});
    ASSERT_TRUE(id);
    renderer::MotionGlyphSettings settings;
    settings.velocity = true;
    settings.force = true;
    settings.acceleration = true;
    settings.length_fraction = 0.1;
    const auto glyphs = renderer::generateMotionGlyphs(scene, settings, id.value(), 20.0);
    ASSERT_EQ(glyphs.size(), 3U);
    EXPECT_EQ(glyphs[0].kind, renderer::MotionGlyphKind::velocity);
    EXPECT_DOUBLE_EQ(glyphs[0].magnitude_SI, 3.0);
    EXPECT_DOUBLE_EQ(glyphs[0].visual_length_m, 2.0);
    EXPECT_EQ(glyphs[1].kind, renderer::MotionGlyphKind::force);
    EXPECT_DOUBLE_EQ(glyphs[1].magnitude_SI, 8.0);
    EXPECT_EQ(glyphs[1].direction, (math::Vec3d{0.0, 1.0, 0.0}));
    EXPECT_EQ(glyphs[2].kind, renderer::MotionGlyphKind::acceleration);
}

TEST(DiagnosticVisualization, MagnitudePlaneSamplesFiniteFieldAndSkipsSourceSingularity) {
    core::Scene scene;
    ASSERT_TRUE(
        scene.createBody({.name = "charge", .mass_kg = 1.0, .charge_C = 1.0e-6, .radius_m = 0.05}));
    physics::em::ElectromagneticSettings em;
    em.minimum_separation_m = 0.1;
    physics::em::ElectromagneticFieldProvider provider(scene, em);
    renderer::FieldVisualizationSettings settings;
    settings.mode = renderer::FieldDisplayMode::magnitude_plane;
    settings.field = renderer::ObservedField::electric;
    settings.vectors.geometry = renderer::SamplingGeometry::plane_xz;
    settings.vectors.resolution = 3;
    settings.region.half_extent_m = {1.5, 1.0, 1.5};
    const auto cells = renderer::sampleMagnitudePlane(provider, settings, 0.0);
    ASSERT_EQ(cells.size(), 8U);
    for (const auto& cell : cells) {
        EXPECT_GE(cell.intensity, 0.0);
        EXPECT_LE(cell.intensity, 1.0);
        EXPECT_TRUE(cell.center_m.isFinite());
    }
}
