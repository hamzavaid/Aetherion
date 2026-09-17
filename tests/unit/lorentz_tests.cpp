#include "aetherion/physics/em/lorentz.hpp"

#include <gtest/gtest.h>

using aetherion::math::Vec3d;
using aetherion::physics::em::gyroDiagnostics;
using aetherion::physics::em::lorentzForce;
using aetherion::physics::fields::FieldSample;

TEST(LorentzForce, MagnitudeDirectionAndRightHandRule) {
    const auto force =
        lorentzForce(2.0, {3.0, 0.0, 0.0},
                     FieldSample{.electric_Vpm = {0.0, 4.0, 0.0}, .magnetic_T = {0.0, 0.0, 5.0}});
    ASSERT_TRUE(force);
    EXPECT_EQ(force.value().electric_N, (Vec3d{0.0, 8.0, 0.0}));
    EXPECT_EQ(force.value().magnetic_N, (Vec3d{0.0, -30.0, 0.0}));
    EXPECT_EQ(force.value().total_N, (Vec3d{0.0, -22.0, 0.0}));
}

TEST(LorentzForce, ZeroChargeAndZeroFieldProduceZeroForce) {
    const auto neutral =
        lorentzForce(0.0, {1.0, 2.0, 3.0},
                     FieldSample{.electric_Vpm = {9.0, 8.0, 7.0}, .magnetic_T = {6.0, 5.0, 4.0}});
    ASSERT_TRUE(neutral);
    EXPECT_EQ(neutral.value().total_N, Vec3d{});
    const auto zero = lorentzForce(3.0, {1.0, 2.0, 3.0}, FieldSample{});
    ASSERT_TRUE(zero);
    EXPECT_EQ(zero.value().total_N, Vec3d{});
}

TEST(LorentzForce, PureMagneticForceIsPerpendicularToVelocity) {
    const Vec3d velocity{3.0, -2.0, 5.0};
    const auto force = lorentzForce(-4.0, velocity, FieldSample{.magnetic_T = {0.2, 0.7, -0.3}});
    ASSERT_TRUE(force);
    EXPECT_NEAR(aetherion::math::dot(force.value().magnetic_N, velocity), 0.0, 1.0e-13);
}

TEST(GyroDiagnostics, MatchesAnalyticalRadiusFrequencyAndPeriod) {
    const auto result = gyroDiagnostics(2.0, 4.0, {3.0, 0.0, 4.0}, {0.0, 0.0, 0.5});
    ASSERT_TRUE(result);
    EXPECT_DOUBLE_EQ(result.value().angular_frequency_rad_ps, 1.0);
    EXPECT_NEAR(result.value().period_s, 2.0 * 3.14159265358979323846, 1.0e-14);
    EXPECT_DOUBLE_EQ(result.value().radius_m, 3.0);
}
