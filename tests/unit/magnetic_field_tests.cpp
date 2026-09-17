#include "aetherion/physics/em/electrostatics.hpp"

#include <gtest/gtest.h>

#include "aetherion/physics/constants.hpp"

TEST(MagneticField, UniformSourcesSuperpose) {
    aetherion::core::Scene scene;
    aetherion::physics::em::ElectromagneticSettings settings;
    settings.analytic_sources.push_back({.magnetic_T = {0.0, 0.0, 0.4}});
    settings.analytic_sources.push_back({.magnetic_T = {0.1, -0.2, 0.3}});
    aetherion::physics::em::ElectromagneticFieldProvider provider(scene, settings);
    const auto sample = provider.sample({9.0, -2.0, 4.0}, 1.0);
    ASSERT_TRUE(sample.valid);
    EXPECT_EQ(sample.magnetic_T, (aetherion::math::Vec3d{0.1, -0.2, 0.7}));
}

TEST(MagneticField, DipoleAxisMatchesAnalyticalValueAndGuardsOrigin) {
    aetherion::core::Scene scene;
    aetherion::physics::em::ElectromagneticSettings settings;
    settings.analytic_sources.push_back(
        {.kind = aetherion::physics::em::AnalyticFieldSourceKind::magnetic_dipole,
         .magnetic_dipole_moment_Am2 = {0.0, 0.0, 2.0},
         .singularity_radius_m = 0.01});
    aetherion::physics::em::ElectromagneticFieldProvider provider(scene, settings);
    const auto sample = provider.sample({0.0, 0.0, 1.0}, 0.0);
    ASSERT_TRUE(sample.valid);
    const double expected =
        aetherion::physics::constants::vacuum_permeability / 3.14159265358979323846;
    EXPECT_NEAR(sample.magnetic_T.z, expected, expected * 1.0e-12);
    EXPECT_FALSE(provider.sample({0.0, 0.0, 0.001}, 0.0).valid);
}
