#include "aetherion/core/scientific_analysis.hpp"

#include <gtest/gtest.h>

#include "aetherion/physics/constants.hpp"
#include "aetherion/physics/em/electrostatics.hpp"

using namespace aetherion;

TEST(ScientificAnalysis, MassWeightedCenterAndTranslatingReferenceCoordinates) {
    core::Scene scene;
    const auto first = scene.createBody(
        {.name = "a",
         .mass_kg = 2.0,
         .radius_m = 0.1,
         .state = {.position_m = {0.0, 0.0, 0.0}, .velocity_mps = {1.0, 0.0, 0.0}}});
    ASSERT_TRUE(first);
    ASSERT_TRUE(scene.createBody(
        {.name = "b",
         .mass_kg = 1.0,
         .radius_m = 0.1,
         .state = {.position_m = {3.0, 0.0, 0.0}, .velocity_mps = {-2.0, 0.0, 0.0}}}));
    const auto center = core::centerOfMass(scene);
    ASSERT_TRUE(center);
    EXPECT_NEAR(center->position_m.x, 1.0, 1.0e-15);
    EXPECT_NEAR(center->velocity_mps.x, 0.0, 1.0e-15);
    EXPECT_DOUBLE_EQ(center->total_mass_kg, 3.0);
    const auto body = core::observeBody(scene, first.value(), core::ReferenceFrame::center_of_mass);
    ASSERT_TRUE(body);
    EXPECT_NEAR(body->position_m.x, -1.0, 1.0e-15);
    EXPECT_DOUBLE_EQ(body->velocity_mps.x, 1.0);
    EXPECT_FALSE(core::observeBody(scene, 999U, core::ReferenceFrame::world));
}

TEST(ScientificAnalysis, BodyObservationReportsNetForceInNewtons) {
    core::Scene scene;
    const auto id = scene.createBody({.name = "test",
                                      .mass_kg = 3.0,
                                      .radius_m = 0.1,
                                      .state = {.position_m = {2.0, 0.0, 0.0},
                                                .velocity_mps = {0.0, 4.0, 0.0},
                                                .acceleration_mps2 = {0.0, -2.0, 0.0}}});
    ASSERT_TRUE(id);
    const auto sample = core::observeBody(scene, id.value(), core::ReferenceFrame::world);
    ASSERT_TRUE(sample);
    EXPECT_EQ(sample->net_force_N, (math::Vec3d{0.0, -6.0, 0.0}));
    EXPECT_DOUBLE_EQ(sample->kinetic_energy_J, 24.0);
    EXPECT_DOUBLE_EQ(sample->speed_mps, 4.0);
}

TEST(ScientificAnalysis, BodyAnchoredFrameSubtractsAnchorPositionAndBulkVelocity) {
    core::Scene scene;
    const auto anchor = scene.createBody(
        {.name = "anchor",
         .mass_kg = 2.0,
         .radius_m = 0.1,
         .state = {.position_m = {10.0, 0.0, 0.0}, .velocity_mps = {2.0, 0.0, 0.0}}});
    const auto target = scene.createBody(
        {.name = "target",
         .mass_kg = 1.0,
         .radius_m = 0.1,
         .state = {.position_m = {15.0, 0.0, 0.0}, .velocity_mps = {3.0, 0.0, 0.0}}});
    ASSERT_TRUE(anchor);
    ASSERT_TRUE(target);
    const auto observed = core::observeBody(scene, target.value(),
                                            core::ReferenceFrame::selected_body, anchor.value());
    ASSERT_TRUE(observed);
    EXPECT_EQ(observed->position_m, (math::Vec3d{5.0, 0.0, 0.0}));
    EXPECT_EQ(observed->velocity_mps, (math::Vec3d{1.0, 0.0, 0.0}));
    EXPECT_FALSE(
        core::observeBody(scene, target.value(), core::ReferenceFrame::selected_body, 999U));
}

TEST(ScientificAnalysis, FixedProbeSamplesFieldAndHistoryReplacesDuplicateTime) {
    core::Scene scene;
    ASSERT_TRUE(
        scene.createBody({.name = "charge", .mass_kg = 1.0, .charge_C = 1.0e-6, .radius_m = 0.1}));
    physics::em::ElectromagneticSettings em;
    physics::em::ElectromagneticFieldProvider provider(scene, em);
    const core::FieldProbe probe{.name = "P1", .position_m = {1.0, 0.0, 0.0}};
    const auto sample = core::sampleProbe(provider, probe, 2.0);
    ASSERT_TRUE(sample);
    EXPECT_NEAR(sample->electric_Vpm.x, physics::constants::coulomb_constant * 1.0e-6,
                physics::constants::coulomb_constant * 1.0e-18);
    core::ScientificHistory history(3);
    ASSERT_TRUE(
        history.record(scene, provider, 2.0, std::nullopt, {probe}, core::ReferenceFrame::world));
    ASSERT_TRUE(
        history.record(scene, provider, 2.0, std::nullopt, {probe}, core::ReferenceFrame::world));
    ASSERT_EQ(history.probeSamples().size(), 1U);
    ASSERT_EQ(history.probeSamples().front().size(), 1U);
    ASSERT_TRUE(
        history.record(scene, provider, 3.0, std::nullopt, {probe}, core::ReferenceFrame::world));
    ASSERT_TRUE(
        history.record(scene, provider, 4.0, std::nullopt, {probe}, core::ReferenceFrame::world));
    ASSERT_TRUE(
        history.record(scene, provider, 5.0, std::nullopt, {probe}, core::ReferenceFrame::world));
    EXPECT_EQ(history.probeSamples().front().size(), 3U);
    ASSERT_TRUE(
        history.record(scene, provider, 1.0, std::nullopt, {probe}, core::ReferenceFrame::world));
    EXPECT_EQ(history.probeSamples().front().size(), 1U);
}
