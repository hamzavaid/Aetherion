// Cross-module regressions for failures found by the repository-wide numerical and API audit.
#include <gtest/gtest.h>

#include <cmath>
#include <limits>

#include "aetherion/core/simulation.hpp"
#include "aetherion/core/simulation_clock.hpp"
#include "aetherion/core/simulation_controller.hpp"
#include "aetherion/physics/em/electrostatics.hpp"
#include "aetherion/physics/em/lorentz.hpp"
#include "aetherion/physics/gravity/gravity_solver.hpp"
#include "aetherion/physics/integrators/boris.hpp"
#include "aetherion/physics/integrators/rk4.hpp"
#include "aetherion/physics/integrators/semi_implicit_euler.hpp"
#include "aetherion/physics/integrators/velocity_verlet.hpp"

using namespace aetherion;

TEST(QualityRegression, VectorNormAndDivisionHandleRepresentableExtremeValues) {
    EXPECT_NEAR((math::Vec3d{3.0e200, 4.0e200, 0.0}.norm()) / 5.0e200, 1.0, 1.0e-15);
    EXPECT_NEAR((math::Vec3d{3.0e-200, 4.0e-200, 0.0}.norm()) / 5.0e-200, 1.0, 1.0e-15);
    EXPECT_EQ((math::Vec3d{1.0e-310, 0.0, 0.0} / 1.0e-310), (math::Vec3d{1.0, 0.0, 0.0}));
    EXPECT_THROW(static_cast<void>(
                     math::Vec3d{std::numeric_limits<double>::infinity(), 0.0, 0.0}.normalized()),
                 std::domain_error);
}

TEST(QualityRegression, EntityIdsCannotWrapOrAllocateTheReservedZeroId) {
    core::Scene scene;
    const auto maximum = std::numeric_limits<core::EntityId>::max();
    EXPECT_FALSE(
        scene.importBody({.id = maximum, .name = "reserved", .mass_kg = 1.0, .radius_m = 1.0}));
    ASSERT_TRUE(
        scene.importBody({.id = maximum - 1U, .name = "last", .mass_kg = 1.0, .radius_m = 1.0}));
    EXPECT_FALSE(scene.createBody({.name = "overflow", .mass_kg = 1.0, .radius_m = 1.0}));
    EXPECT_EQ(scene.size(), 1U);
}

TEST(QualityRegression, FailedClockStepsDoNotConsumeOrAdvanceSimulationTime) {
    core::SimulationClock clock({.physics_dt_s = 0.1});
    EXPECT_EQ(clock.advance(0.2, [](double) { return false; }), 0U);
    EXPECT_DOUBLE_EQ(clock.simulationTimeSeconds(), 0.0);
    EXPECT_EQ(clock.advance(0.0, [](double) { return true; }), 2U);
    core::SimulationClock huge({.physics_dt_s = 1.0, .time_scale = 1.0e300});
    EXPECT_THROW(static_cast<void>(huge.advance(1.0e300, [](double) {})), std::overflow_error);
}

TEST(QualityRegression, FailedEulerAndVerletStepsRestoreAllBodyState) {
    core::Scene scene;
    ASSERT_TRUE(scene.createBody({.name = "first",
                                  .mass_kg = 1.0,
                                  .radius_m = 1.0,
                                  .state = {.velocity_mps = {1.0, 0.0, 0.0}}}));
    ASSERT_TRUE(scene.createBody({.name = "overflow",
                                  .mass_kg = 1.0,
                                  .radius_m = 1.0,
                                  .state = {.velocity_mps = {1.0e308, 0.0, 0.0},
                                            .acceleration_mps2 = {1.0e308, 0.0, 0.0}}}));
    const auto first = scene.bodies()[0].state;
    const auto second = scene.bodies()[1].state;
    EXPECT_FALSE(physics::SemiImplicitEuler::integrate(scene, 2.0));
    EXPECT_EQ(scene.bodies()[0].state, first);
    EXPECT_EQ(scene.bodies()[1].state, second);
    physics::VelocityVerletIntegrator verlet;
    int calls = 0;
    EXPECT_FALSE(verlet.step(scene, 0.1, [&](core::Scene& evaluated) -> core::Status {
        evaluated.bodies()[0].state.acceleration_mps2 = {2.0, 0.0, 0.0};
        if (++calls == 2)
            return core::Error{core::ErrorCode::numerical_failure, "injected evaluation failure"};
        return core::success();
    }));
    EXPECT_EQ(scene.bodies()[0].state, first);
}

TEST(QualityRegression, BorisRestoresTheFirstFailedAccelerationEvaluation) {
    core::Scene scene;
    ASSERT_TRUE(scene.createBody({.name = "body", .mass_kg = 1.0, .radius_m = 1.0}));
    const auto initial = scene.bodies()[0].state;
    physics::BorisIntegrator boris;
    EXPECT_FALSE(boris.step(
        scene, 0.1,
        [](core::Scene& evaluated) -> core::Status {
            evaluated.bodies()[0].state.acceleration_mps2 = {3.0, 0.0, 0.0};
            return core::Error{core::ErrorCode::numerical_failure, "injected failure"};
        },
        [](const math::Vec3d&) -> core::Result<math::Vec3d> { return math::Vec3d{}; }));
    EXPECT_EQ(scene.bodies()[0].state, initial);
}

TEST(QualityRegression, InvalidIntegratorAndGyroOverflowAreRejected) {
    core::Scene scene;
    EXPECT_THROW(core::Simulation(scene, {.integrator = static_cast<physics::IntegratorKind>(99)}),
                 std::invalid_argument);
    core::SimulationController controller(scene);
    controller.commands().enqueue(
        core::SetIntegratorCommand{static_cast<physics::IntegratorKind>(99)});
    ASSERT_TRUE(controller.update(0.0));
    EXPECT_FALSE(controller.commands().eventLog().back().accepted);
    EXPECT_FALSE(physics::em::gyroDiagnostics(1.0, 1.0e308, {1.0, 0.0, 0.0}, {0.0, 0.0, 1.0e308}));
    const auto neutral =
        physics::em::lorentzForce(0.0, {1.0e300, 0.0, 0.0}, {.magnetic_T = {0.0, 1.0e300, 0.0}});
    ASSERT_TRUE(neutral);
    EXPECT_EQ(neutral.value().total_N, math::Vec3d{});
}

TEST(QualityRegression, FailingForceSolversRestoreEarlierPairAccumulations) {
    core::Scene scene;
    for (const double x : {0.0, 2.0, -1.0e308, 1.0e308})
        ASSERT_TRUE(scene.createBody(
            {.name = "source",
             .mass_kg = 1.0,
             .charge_C = 1.0e-6,
             .radius_m = 1.0,
             .state = {.position_m = {x, 0.0, 0.0}, .acceleration_mps2 = {1.0, 2.0, 3.0}}}));
    physics::GravitySolver gravity;
    EXPECT_FALSE(gravity.computeAccelerations(scene));
    for (const auto& body : scene.bodies())
        EXPECT_EQ(body.state.acceleration_mps2, (math::Vec3d{1.0, 2.0, 3.0}));
    physics::em::ElectrostaticSolver electric;
    EXPECT_FALSE(electric.accumulateAccelerations(scene));
    for (const auto& body : scene.bodies())
        EXPECT_EQ(body.state.acceleration_mps2, (math::Vec3d{1.0, 2.0, 3.0}));
}

TEST(QualityRegression, Rk4RestoresKinematicsWhenAnEvaluatorThrows) {
    core::Scene scene;
    ASSERT_TRUE(scene.createBody({.name = "body", .mass_kg = 1.0, .radius_m = 1.0}));
    const auto original = scene.bodies().front().state;
    physics::Rk4Integrator rk4;
    EXPECT_THROW(static_cast<void>(
                     rk4.step(scene, 0.1,
                              [](core::Scene& evaluated) -> core::Status {
                                  evaluated.bodies().front().state.velocity_mps = {3.0, 0.0, 0.0};
                                  throw std::runtime_error("injected callback exception");
                              })),
                 std::runtime_error);
    EXPECT_EQ(scene.bodies().front().state, original);
}

TEST(QualityRegression, BorisRejectsAnUnrepresentableRotationInsteadOfSilentlyIgnoringIt) {
    core::Scene scene;
    ASSERT_TRUE(scene.createBody({.name = "particle",
                                  .mass_kg = 1.0,
                                  .charge_C = 1.0,
                                  .radius_m = 1.0,
                                  .state = {.velocity_mps = {1.0, 0.0, 0.0}}}));
    const auto original = scene.bodies().front().state;
    physics::BorisIntegrator boris;
    EXPECT_FALSE(boris.step(
        scene, 1.0, [](core::Scene&) { return core::success(); },
        [](const math::Vec3d&) -> core::Result<math::Vec3d> {
            return math::Vec3d{0.0, 0.0, 1.0e200};
        }));
    EXPECT_EQ(scene.bodies().front().state, original);
}

TEST(QualityRegression, InverseSquareForcesRemainRepresentableWhenDistanceCubedOverflows) {
    core::Scene scene;
    for (const double x : {0.0, 1.0e125})
        ASSERT_TRUE(scene.createBody({.name = "source",
                                      .mass_kg = 1.0e250,
                                      .charge_C = 1.0e100,
                                      .radius_m = 1.0,
                                      .state = {.position_m = {x, 0.0, 0.0}}}));
    physics::GravitySolver gravity;
    ASSERT_TRUE(gravity.computeAccelerations(scene));
    EXPECT_NEAR(scene.bodies()[0].state.acceleration_mps2.x, 6.67430e-11, 1.0e-24);
    for (auto& body : scene.bodies()) {
        body.mass_kg = 1.0e100;
        body.state.acceleration_mps2 = {};
    }
    physics::em::ElectrostaticSolver electric;
    ASSERT_TRUE(electric.accumulateAccelerations(scene));
    EXPECT_NEAR(scene.bodies()[0].state.acceleration_mps2.x / -8.9875517923e-141, 1.0, 1.0e-13);
    const auto field = physics::em::samplePointChargeField(1.0e300, {}, {1.0e150, 0.0, 0.0}, {});
    ASSERT_TRUE(field);
    EXPECT_NEAR(field.value().x / 8.9875517923e9, 1.0, 1.0e-13);
    physics::em::ElectromagneticSettings settings;
    settings.analytic_sources.push_back(
        {.kind = physics::em::AnalyticFieldSourceKind::magnetic_dipole,
         .magnetic_dipole_moment_Am2 = {0.0, 0.0, 1.0e300}});
    const auto dipole = physics::em::sampleAnalyticField(settings, {0.0, 0.0, 1.0e125}, 0.0);
    ASSERT_TRUE(dipole.valid);
    EXPECT_NEAR(dipole.magnetic_T.z / 2.0e-82, 1.0, 1.0e-8);
}
