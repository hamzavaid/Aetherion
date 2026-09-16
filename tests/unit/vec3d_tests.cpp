#include "aetherion/math/vec3d.hpp"

#include <gtest/gtest.h>

#include <limits>
#include <stdexcept>

using aetherion::math::cross;
using aetherion::math::dot;
using aetherion::math::Vec3d;

TEST(Vec3d, ArithmeticAndNormUseCartesianDoublePrecision) {
    const Vec3d a{1.0, 2.0, 3.0};
    const Vec3d b{-4.0, 5.0, -6.0};
    EXPECT_EQ(a + b, (Vec3d{-3.0, 7.0, -3.0}));
    EXPECT_EQ(a - b, (Vec3d{5.0, -3.0, 9.0}));
    EXPECT_DOUBLE_EQ(dot(a, b), -12.0);
    EXPECT_DOUBLE_EQ((Vec3d{3.0, 4.0, 0.0}.norm()), 5.0);
}

TEST(Vec3d, CrossProductUsesRightHandedOrientation) {
    EXPECT_EQ(cross({1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}), (Vec3d{0.0, 0.0, 1.0}));
}

TEST(Vec3d, NormalizationRejectsSingularAndNonFiniteInputs) {
    EXPECT_THROW(static_cast<void>(Vec3d{}.normalized()), std::domain_error);
    EXPECT_THROW(static_cast<void>(Vec3d{1.0, 0.0, 0.0} / 0.0), std::domain_error);
    EXPECT_FALSE((Vec3d{std::numeric_limits<double>::infinity(), 0.0, 0.0}.isFinite()));
}
