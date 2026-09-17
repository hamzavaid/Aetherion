#include "aetherion/serialization/scene_serialization.hpp"

#include <gtest/gtest.h>

TEST(SceneSerialization, ElectromagneticAndVisualizationSettingsRoundTrip) {
    aetherion::serialization::SceneDocument original;
    ASSERT_TRUE(original.scene.importBody(
        {.id = 42,
         .name = "charged body",
         .mass_kg = 2.0,
         .charge_C = -3.0e-6,
         .radius_m = 0.25,
         .state = {.position_m = {1.0, 2.0, 3.0}, .velocity_mps = {4.0, 5.0, 6.0}}}));
    original.runtime.physics_dt_s = 2.0e-6;
    original.runtime.gravity_enabled = false;
    original.runtime.electromagnetism.electrostatics_enabled = true;
    original.runtime.electromagnetism.magnetic_enabled = true;
    original.runtime.integrator = aetherion::physics::IntegratorKind::boris;
    original.runtime.electromagnetism.minimum_separation_m = 0.02;
    original.runtime.electromagnetism.analytic_sources.push_back(
        {.electric_Vpm = {1.0, 2.0, 3.0}, .magnetic_T = {0.0, 0.0, 0.4}});
    original.runtime.electromagnetism.analytic_sources.push_back(
        {.kind = aetherion::physics::em::AnalyticFieldSourceKind::magnetic_dipole,
         .position_m = {5.0, 6.0, 7.0},
         .magnetic_dipole_moment_Am2 = {0.0, 2.0, 0.0},
         .singularity_radius_m = 0.3});
    auto& field = original.visualization.field_visualization;
    field.mode = aetherion::renderer::FieldDisplayMode::field_lines;
    field.field = aetherion::renderer::ObservedField::gravity;
    field.planar_2d = true;
    field.region.center_m = {3.0, 2.0, 1.0};
    field.vectors.resolution = 7;
    field.lines.step_size_m = 0.03;
    field.lines.custom_seeds_m = {{0.1, 0.2, 0.3}, {-0.1, 0.0, 0.2}};
    field.colors.electric_vectors = {0.1F, 0.2F, 0.3F};
    field.colors.electric_lines = {0.4F, 0.5F, 0.6F};
    field.colors.magnetic_vectors = {0.7F, 0.8F, 0.9F};
    field.colors.magnetic_lines = {0.15F, 0.25F, 0.35F};
    field.colors.gravity_vectors = {0.2F, 0.4F, 0.6F};
    field.colors.gravity_lines = {0.3F, 0.5F, 0.7F};

    const auto encoded = aetherion::serialization::serializeScene(original);
    const auto decoded = aetherion::serialization::deserializeScene(encoded);
    ASSERT_TRUE(decoded) << decoded.error().message;
    ASSERT_EQ(decoded.value().scene.size(), 1U);
    const auto& body = decoded.value().scene.bodies()[0];
    EXPECT_EQ(body.id, 42U);
    EXPECT_EQ(body.name, "charged body");
    EXPECT_DOUBLE_EQ(body.charge_C, -3.0e-6);
    EXPECT_TRUE(decoded.value().runtime.electromagnetism.electrostatics_enabled);
    EXPECT_TRUE(decoded.value().runtime.electromagnetism.magnetic_enabled);
    EXPECT_EQ(decoded.value().runtime.integrator, aetherion::physics::IntegratorKind::boris);
    EXPECT_DOUBLE_EQ(decoded.value().runtime.electromagnetism.minimum_separation_m, 0.02);
    ASSERT_EQ(decoded.value().runtime.electromagnetism.analytic_sources.size(), 2U);
    EXPECT_EQ(decoded.value().runtime.electromagnetism.analytic_sources[0].electric_Vpm,
              (aetherion::math::Vec3d{1.0, 2.0, 3.0}));
    EXPECT_EQ(decoded.value().runtime.electromagnetism.analytic_sources[0].magnetic_T,
              (aetherion::math::Vec3d{0.0, 0.0, 0.4}));
    const auto& dipole = decoded.value().runtime.electromagnetism.analytic_sources[1];
    EXPECT_EQ(dipole.kind, aetherion::physics::em::AnalyticFieldSourceKind::magnetic_dipole);
    EXPECT_EQ(dipole.position_m, (aetherion::math::Vec3d{5.0, 6.0, 7.0}));
    EXPECT_EQ(dipole.magnetic_dipole_moment_Am2, (aetherion::math::Vec3d{0.0, 2.0, 0.0}));
    EXPECT_DOUBLE_EQ(dipole.singularity_radius_m, 0.3);
    EXPECT_EQ(decoded.value().visualization.field_visualization.mode,
              aetherion::renderer::FieldDisplayMode::field_lines);
    EXPECT_EQ(decoded.value().visualization.field_visualization.field,
              aetherion::renderer::ObservedField::gravity);
    EXPECT_TRUE(decoded.value().visualization.field_visualization.planar_2d);
    EXPECT_EQ(decoded.value().visualization.field_visualization.lines.custom_seeds_m.size(), 2U);
    EXPECT_EQ(decoded.value().visualization.field_visualization.colors.electric_vectors,
              field.colors.electric_vectors);
    EXPECT_EQ(decoded.value().visualization.field_visualization.colors.electric_lines,
              field.colors.electric_lines);
    EXPECT_EQ(decoded.value().visualization.field_visualization.colors.magnetic_vectors,
              field.colors.magnetic_vectors);
    EXPECT_EQ(decoded.value().visualization.field_visualization.colors.magnetic_lines,
              field.colors.magnetic_lines);
    EXPECT_EQ(decoded.value().visualization.field_visualization.colors.gravity_vectors,
              field.colors.gravity_vectors);
    EXPECT_EQ(decoded.value().visualization.field_visualization.colors.gravity_lines,
              field.colors.gravity_lines);
}

TEST(SceneSerialization, RejectsUnknownSchemaAndNonFiniteJsonNumber) {
    const auto unknown = aetherion::serialization::deserializeScene(R"({"schemaVersion":2})");
    EXPECT_FALSE(unknown);
    const auto invalid = aetherion::serialization::deserializeScene(R"({"schemaVersion":NaN})");
    EXPECT_FALSE(invalid);
}
