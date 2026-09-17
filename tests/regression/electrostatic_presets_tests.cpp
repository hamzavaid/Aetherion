#include "aetherion/presets/em_presets.hpp"

#include <gtest/gtest.h>

#include "aetherion/physics/em/electrostatics.hpp"

TEST(ElectrostaticPresets, LikeOppositeAndDipoleHaveExpectedFieldsAndDefaults) {
    const auto like = aetherion::presets::makeLikeChargesPreset();
    const auto opposite = aetherion::presets::makeOppositeChargesPreset();
    const auto dipole = aetherion::presets::makeElectricDipolePreset();
    ASSERT_EQ(like.scene.size(), 2U);
    ASSERT_EQ(opposite.scene.size(), 2U);
    EXPECT_GT(like.scene.bodies()[0].charge_C * like.scene.bodies()[1].charge_C, 0.0);
    EXPECT_LT(opposite.scene.bodies()[0].charge_C * opposite.scene.bodies()[1].charge_C, 0.0);
    EXPECT_TRUE(dipole.scene.bodies()[0].fixed);
    EXPECT_TRUE(dipole.scene.bodies()[1].fixed);
    EXPECT_TRUE(like.runtime.electromagnetism.electrostatics_enabled);
    EXPECT_EQ(dipole.visualization.mode, aetherion::renderer::FieldDisplayMode::field_lines);
    aetherion::physics::em::ElectromagneticFieldProvider provider(dipole.scene,
                                                                  dipole.runtime.electromagnetism);
    const auto sample = provider.sample({0.0, 1.0, 0.0}, 0.0);
    EXPECT_TRUE(sample.valid);
    EXPECT_GT(sample.electric_Vpm.x, 0.0);
    EXPECT_DOUBLE_EQ(dipole.meters_to_render_units, 1.0);
    EXPECT_LT(dipole.minimum_apparent_radius, 0.1F);

    auto vector_settings = dipole.visualization;
    vector_settings.mode = aetherion::renderer::FieldDisplayMode::observed_vectors;
    const auto glyphs = aetherion::renderer::sampleObservedField(provider, vector_settings, 0.0);
    EXPECT_FALSE(glyphs.empty());
    auto seeds =
        aetherion::renderer::generateAutomaticFieldSeeds(dipole.scene, dipole.visualization);
    const auto lines =
        aetherion::renderer::traceFieldLines(provider, dipole.visualization, seeds, 0.0);
    EXPECT_FALSE(lines.empty());
}
