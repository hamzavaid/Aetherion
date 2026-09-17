#include "aetherion/presets/em_presets.hpp"

#include <gtest/gtest.h>

namespace {
void expectMagneticPreset(const aetherion::presets::ElectromagneticPreset& preset) {
    ASSERT_EQ(preset.scene.size(), 1U);
    EXPECT_EQ(preset.runtime.integrator, aetherion::physics::IntegratorKind::boris);
    EXPECT_TRUE(preset.runtime.electromagnetism.magnetic_enabled);
    ASSERT_FALSE(preset.runtime.electromagnetism.analytic_sources.empty());
    EXPECT_GT(preset.runtime.electromagnetism.analytic_sources[0].magnetic_T.norm(), 0.0);
    EXPECT_EQ(preset.visualization.field, aetherion::renderer::ObservedField::magnetic);
}
} // namespace

TEST(MagneticPresets, GyroHelicalVectorAndLineScenesHaveUsefulDefaults) {
    const auto gyro = aetherion::presets::makeUniformMagneticGyroPreset();
    const auto helical = aetherion::presets::makeHelicalMagneticPreset();
    const auto vectors = aetherion::presets::makeMagneticVectorPreset();
    const auto lines = aetherion::presets::makeMagneticFieldLinesPreset();
    expectMagneticPreset(gyro);
    expectMagneticPreset(helical);
    expectMagneticPreset(vectors);
    expectMagneticPreset(lines);
    EXPECT_EQ(vectors.visualization.mode, aetherion::renderer::FieldDisplayMode::observed_vectors);
    EXPECT_EQ(lines.visualization.mode, aetherion::renderer::FieldDisplayMode::field_lines);
    EXPECT_GT(helical.scene.bodies()[0].state.velocity_mps.z, 0.0);
}

TEST(MagneticPresets, CrossedFieldsCancelAtConfiguredSelectorVelocity) {
    const auto preset = aetherion::presets::makeCrossedFieldsPreset();
    expectMagneticPreset(preset);
    EXPECT_TRUE(preset.runtime.electromagnetism.electrostatics_enabled);
    const auto& source = preset.runtime.electromagnetism.analytic_sources[0];
    const auto& velocity = preset.scene.bodies()[0].state.velocity_mps;
    EXPECT_NEAR((source.electric_Vpm + aetherion::math::cross(velocity, source.magnetic_T)).norm(),
                0.0, 1.0e-15);
}
