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
