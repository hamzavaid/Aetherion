#include "aetherion/presets/mechanics_presets.hpp"

#include <gtest/gtest.h>

#include <algorithm>

namespace {
bool hasBody(const aetherion::core::Scene& scene, const char* name) {
    return std::any_of(scene.bodies().begin(), scene.bodies().end(),
                       [name](const auto& body) { return body.name == name; });
}
} // namespace

TEST(GravityPresets, EarthSunEarthMoonAndThreeBodyScenesHaveExpectedMembers) {
    const auto earth_sun = aetherion::presets::makeEarthSunGravityPreset();
    const auto earth_moon = aetherion::presets::makeEarthMoonGravityPreset();
    const auto three_body = aetherion::presets::makeSunEarthMoonGravityPreset();
    EXPECT_EQ(earth_sun.scene.size(), 2U);
    EXPECT_TRUE(hasBody(earth_sun.scene, "Sun"));
    EXPECT_TRUE(hasBody(earth_sun.scene, "Earth"));
    EXPECT_EQ(earth_moon.scene.size(), 2U);
    EXPECT_TRUE(hasBody(earth_moon.scene, "Earth"));
    EXPECT_TRUE(hasBody(earth_moon.scene, "Moon"));
    EXPECT_EQ(three_body.scene.size(), 3U);
    EXPECT_TRUE(hasBody(three_body.scene, "Sun"));
    EXPECT_TRUE(hasBody(three_body.scene, "Earth"));
    EXPECT_TRUE(hasBody(three_body.scene, "Moon"));
    EXPECT_TRUE(earth_sun.runtime.gravity_enabled);
    EXPECT_TRUE(earth_moon.runtime.gravity_enabled);
    EXPECT_TRUE(three_body.runtime.gravity_enabled);
    EXPECT_GT(earth_sun.meters_to_render_units, 0.0);
    EXPECT_GT(earth_moon.meters_to_render_units, earth_sun.meters_to_render_units);
}
