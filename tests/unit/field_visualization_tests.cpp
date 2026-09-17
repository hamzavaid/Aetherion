#include "aetherion/renderer/field_visualization.hpp"

#include <gtest/gtest.h>

#include "aetherion/physics/em/electrostatics.hpp"

using namespace aetherion;

namespace {
class UniformProvider final : public physics::fields::IFieldProvider {
  public:
    physics::fields::FieldSample sample(const math::Vec3d&, double) const override {
        return {.electric_Vpm = {2.0, 0.0, 0.0}, .magnetic_T = {0.0, 0.0, 1.0}};
    }
    std::uint64_t revision() const noexcept override { return 1U; }
};

class OutOfPlaneProvider final : public physics::fields::IFieldProvider {
  public:
    physics::fields::FieldSample sample(const math::Vec3d&, double) const override {
        return {.electric_Vpm = {1.0, 5.0, 2.0}};
    }
    std::uint64_t revision() const noexcept override { return 2U; }
};
} // namespace

TEST(FieldVisualization, VectorSamplingSupportsPlanesVolumesAndNormalization) {
    UniformProvider provider;
    renderer::FieldVisualizationSettings settings;
    settings.field = renderer::ObservedField::electric;
    settings.region.half_extent_m = {1.0, 1.0, 1.0};
    settings.vectors.resolution = 3;
    settings.vectors.geometry = renderer::SamplingGeometry::plane_xz;
    settings.vectors.visual_length_m = 0.25;
    auto glyphs = renderer::sampleObservedField(provider, settings, 0.0);
    ASSERT_EQ(glyphs.size(), 9U);
    EXPECT_EQ(glyphs[0].direction, (math::Vec3d{1.0, 0.0, 0.0}));
    EXPECT_DOUBLE_EQ(glyphs[0].visual_length_m, 0.25);
    settings.vectors.geometry = renderer::SamplingGeometry::volume;
    glyphs = renderer::sampleObservedField(provider, settings, 0.0);
    EXPECT_EQ(glyphs.size(), 27U);
}

TEST(FieldLines, UniformFieldHasCorrectDirectionAndRegionTermination) {
    UniformProvider provider;
    renderer::FieldVisualizationSettings settings;
    settings.field = renderer::ObservedField::electric;
    settings.region.half_extent_m = {1.0, 1.0, 1.0};
    settings.lines.step_size_m = 0.1;
    settings.lines.maximum_steps = 100;
    settings.lines.maximum_total_steps = 100;
    settings.lines.maximum_length_m = 10.0;
    settings.lines.trace_backward = false;
    const auto lines = renderer::traceFieldLines(provider, settings, {{{0.0, 0.0, 0.0}}}, 0.0);
    ASSERT_EQ(lines.size(), 1U);
    ASSERT_GT(lines[0].points_m.size(), 2U);
    EXPECT_GT(lines[0].points_m.back().x, 0.8);
    EXPECT_LE(lines[0].points_m.back().x, 1.0);
    EXPECT_NEAR(lines[0].points_m.back().y, 0.0, 1.0e-14);
}

TEST(FieldLines, UniformMagneticFieldTracesBothDirectionsWithoutMonopoleTermination) {
    UniformProvider provider;
    renderer::FieldVisualizationSettings settings;
    settings.field = renderer::ObservedField::magnetic;
    settings.region.half_extent_m = {1.0, 1.0, 1.0};
    settings.lines.step_size_m = 0.1;
    settings.lines.maximum_steps = 100;
    settings.lines.maximum_total_steps = 200;
    settings.lines.maximum_length_m = 10.0;
    const auto lines = renderer::traceFieldLines(provider, settings, {{{0.0, 0.0, 0.0}}}, 0.0);
    ASSERT_EQ(lines.size(), 1U);
    ASSERT_GT(lines[0].points_m.size(), 10U);
    EXPECT_LT(lines[0].points_m.front().z, -0.8);
    EXPECT_GT(lines[0].points_m.back().z, 0.8);
    EXPECT_NEAR(lines[0].points_m.back().x, 0.0, 1.0e-14);
}

TEST(FieldLines, PositivePointChargeLinesDepartAndTerminateBeforeSingularity) {
    core::Scene scene;
    ASSERT_TRUE(
        scene.createBody({.name = "charge", .mass_kg = 1.0, .charge_C = 1.0e-6, .radius_m = 0.01}));
    physics::em::ElectromagneticSettings em;
    em.minimum_separation_m = 0.05;
    physics::em::ElectromagneticFieldProvider provider(scene, em);
    renderer::FieldVisualizationSettings settings;
    settings.field = renderer::ObservedField::electric;
    settings.region.half_extent_m = {2.0, 2.0, 2.0};
    settings.lines.step_size_m = 0.02;
    settings.lines.maximum_length_m = 2.0;
    settings.lines.trace_backward = false;
    const auto lines = renderer::traceFieldLines(provider, settings, {{{0.1, 0.0, 0.0}}}, 0.0);
    ASSERT_EQ(lines.size(), 1U);
    EXPECT_GT(lines[0].points_m.back().x, lines[0].points_m.front().x);

    settings.lines.trace_forward = false;
    settings.lines.trace_backward = true;
    const auto inward = renderer::traceFieldLines(provider, settings, {{{0.1, 0.0, 0.0}}}, 0.0);
    ASSERT_EQ(inward.size(), 1U);
    EXPECT_GE(inward[0].points_m.front().x, 0.05);
}

TEST(FieldVisualization, PlanarModeRemovesOutOfPlaneVectorAndLineComponents) {
    OutOfPlaneProvider provider;
    renderer::FieldVisualizationSettings settings;
    settings.field = renderer::ObservedField::electric;
    settings.planar_2d = true;
    settings.region.half_extent_m = {1.0, 1.0, 1.0};
    settings.vectors.geometry = renderer::SamplingGeometry::plane_xz;
    settings.vectors.resolution = 3;
    const auto glyphs = renderer::sampleObservedField(provider, settings, 0.0);
    ASSERT_EQ(glyphs.size(), 9U);
    EXPECT_NEAR(glyphs.front().direction.y, 0.0, 1.0e-15);
    EXPECT_GT(glyphs.front().direction.z, 0.0);

    settings.lines.step_size_m = 0.05;
    settings.lines.maximum_steps = 20;
    settings.lines.maximum_total_steps = 20;
    settings.lines.maximum_length_m = 1.0;
    settings.lines.trace_backward = false;
    const auto lines = renderer::traceFieldLines(provider, settings, {{{0.0, 0.7, 0.0}}}, 0.0);
    ASSERT_EQ(lines.size(), 1U);
    for (const auto& point : lines[0].points_m)
        EXPECT_NEAR(point.y, settings.region.center_m.y, 1.0e-15);
}

TEST(FieldVisualization, PlanarElectricSeedsUseSelectedSlice) {
    core::Scene scene;
    ASSERT_TRUE(
        scene.createBody({.name = "charge", .mass_kg = 1.0, .charge_C = 1.0, .radius_m = 0.1}));
    renderer::FieldVisualizationSettings settings;
    settings.planar_2d = true;
    settings.vectors.geometry = renderer::SamplingGeometry::plane_xz;
    settings.lines.automatic_seed_count = 8;
    const auto seeds = renderer::generateAutomaticFieldSeeds(scene, settings);
    ASSERT_EQ(seeds.size(), 8U);
    for (const auto& seed : seeds)
        EXPECT_DOUBLE_EQ(seed.y, settings.region.center_m.y);
    EXPECT_NE(seeds[0].z, seeds[1].z);
}
