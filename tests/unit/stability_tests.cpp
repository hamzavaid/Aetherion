#include "aetherion/physics/stability_analyzer.hpp"
#include "aetherion/presets/mechanics_presets.hpp"

#include <gtest/gtest.h>

TEST(StabilityAnalyzer, WarnsWhenOrbitIsUndersampledAndAcceptsResolvedStep) {
    const auto scene = aetherion::presets::makeEarthLikeOrbit();
    const auto stable = aetherion::physics::analyzeTimestep(scene, 3600.0);
    EXPECT_TRUE(stable.empty());
    const auto unstable = aetherion::physics::analyzeTimestep(
        scene, aetherion::presets::earth_like_orbit_period_s / 10.0);
    ASSERT_FALSE(unstable.empty());
    EXPECT_GT(unstable.front().timestep_to_timescale_ratio, 0.05);
}
