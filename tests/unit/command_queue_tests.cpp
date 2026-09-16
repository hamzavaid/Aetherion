#include "aetherion/core/command_queue.hpp"

#include <gtest/gtest.h>

using aetherion::core::Body;
using aetherion::core::BodyPatch;
using aetherion::core::CommandQueue;
using aetherion::core::CreateBodyCommand;
using aetherion::core::RuntimeSettings;
using aetherion::core::Scene;
using aetherion::core::SetIntegratorCommand;
using aetherion::core::SetPhysicsDtCommand;
using aetherion::core::UpdateBodyCommand;

TEST(CommandQueue, AppliesTypedEditsInFifoOrderAndRecordsEvents) {
    Scene scene;
    const auto id = scene.createBody(Body{.name = "body", .mass_kg = 1.0, .radius_m = 1.0});
    ASSERT_TRUE(id);
    RuntimeSettings settings;
    CommandQueue queue;
    queue.enqueue(UpdateBodyCommand{.id = id.value(), .patch = BodyPatch{.mass_kg = 4.0}});
    queue.enqueue(
        UpdateBodyCommand{.id = id.value(), .patch = BodyPatch{.velocity_mps = {{2, 0, 0}}}});
    queue.enqueue(SetPhysicsDtCommand{.physics_dt_s = 0.25});
    queue.enqueue(SetIntegratorCommand{aetherion::physics::IntegratorKind::velocity_verlet});
    const auto report = queue.apply(scene, settings, 1.5);
    EXPECT_EQ(report.accepted, 4U);
    EXPECT_EQ(report.rejected, 0U);
    EXPECT_DOUBLE_EQ(scene.find(id.value())->mass_kg, 4.0);
    EXPECT_EQ(scene.find(id.value())->state.velocity_mps, (aetherion::math::Vec3d{2, 0, 0}));
    EXPECT_DOUBLE_EQ(settings.physics_dt_s, 0.25);
    EXPECT_EQ(settings.integrator, aetherion::physics::IntegratorKind::velocity_verlet);
    ASSERT_EQ(queue.eventLog().size(), 4U);
    EXPECT_LT(queue.eventLog()[0].sequence, queue.eventLog()[1].sequence);
}

TEST(CommandQueue, RejectsInvalidEditAtomicallyAndContinues) {
    Scene scene;
    const auto id = scene.createBody(Body{.name = "body", .mass_kg = 1.0, .radius_m = 1.0});
    ASSERT_TRUE(id);
    RuntimeSettings settings;
    CommandQueue queue;
    queue.enqueue(UpdateBodyCommand{.id = id.value(), .patch = BodyPatch{.mass_kg = -1.0}});
    queue.enqueue(CreateBodyCommand{.body = Body{.name = "new", .mass_kg = 2.0, .radius_m = 1.0}});
    const auto report = queue.apply(scene, settings, 0.0);
    EXPECT_EQ(report.rejected, 1U);
    EXPECT_EQ(report.accepted, 1U);
    EXPECT_DOUBLE_EQ(scene.find(id.value())->mass_kg, 1.0);
    EXPECT_EQ(scene.size(), 2U);
    EXPECT_FALSE(queue.eventLog()[0].accepted);
}
