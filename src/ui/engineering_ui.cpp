#include "aetherion/ui/engineering_ui.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>

#include <GLFW/glfw3.h>

#include <array>
#include <cfloat>
#include <cstdio>
#include <string>
#include <vector>

#include "aetherion/physics/stability_analyzer.hpp"
#include "aetherion/presets/mechanics_presets.hpp"

namespace aetherion::ui {

EngineeringUi::~EngineeringUi() { shutdown(); }

core::Status EngineeringUi::initialize(void* glfw_window) {
    if (glfw_window == nullptr) {
        return core::Error{core::ErrorCode::invalid_argument,
                           "engineering UI requires a valid GLFW window"};
    }
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();
    if (!ImGui_ImplGlfw_InitForOpenGL(static_cast<GLFWwindow*>(glfw_window), true)) {
        ImGui::DestroyContext();
        return core::Error{core::ErrorCode::platform_failure,
                           "Dear ImGui GLFW backend initialization failed"};
    }
    if (!ImGui_ImplOpenGL3_Init("#version 410 core")) {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        return core::Error{core::ErrorCode::platform_failure,
                           "Dear ImGui OpenGL backend initialization failed"};
    }
    initialized_ = true;
    return core::success();
}

void EngineeringUi::shutdown() noexcept {
    if (!initialized_)
        return;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    initialized_ = false;
}

void EngineeringUi::drawDockSpace() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    constexpr ImGuiWindowFlags host_flags =
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);
    ImGui::Begin("Aetherion Engineering Workspace", nullptr, host_flags);
    ImGui::PopStyleVar(2);
    const ImGuiID dock_id = ImGui::GetID("AetherionDockSpace");
    ImGui::DockSpace(dock_id, {}, ImGuiDockNodeFlags_PassthruCentralNode);
    if (!dock_layout_initialized_) {
        dock_layout_initialized_ = true;
        ImGui::DockBuilderRemoveNode(dock_id);
        const auto node_flags = static_cast<ImGuiDockNodeFlags>(
            static_cast<int>(ImGuiDockNodeFlags_DockSpace) |
            static_cast<int>(ImGuiDockNodeFlags_PassthruCentralNode));
        ImGui::DockBuilderAddNode(dock_id, node_flags);
        ImGui::DockBuilderSetNodeSize(dock_id, viewport->WorkSize);
        ImGuiID center = dock_id;
        const ImGuiID left =
            ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.20F, nullptr, &center);
        const ImGuiID right =
            ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.26F, nullptr, &center);
        const ImGuiID bottom =
            ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.24F, nullptr, &center);
        ImGui::DockBuilderDockWindow("Scene Hierarchy", left);
        ImGui::DockBuilderDockWindow("Inspector", right);
        ImGui::DockBuilderDockWindow("Simulation Controls", bottom);
        ImGui::DockBuilderDockWindow("Diagnostics", bottom);
        ImGui::DockBuilderDockWindow("Plots", bottom);
        ImGui::DockBuilderFinish(dock_id);
    }
    ImGui::End();
}

void EngineeringUi::drawHierarchy(core::SimulationController& controller) {
    ImGui::Begin("Scene Hierarchy");
    for (const auto& body : controller.scene().bodies()) {
        const bool selected = selected_ && *selected_ == body.id;
        if (ImGui::Selectable(body.name.c_str(), selected))
            selected_ = body.id;
    }
    if (ImGui::Button("Add Body")) {
        controller.commands().enqueue(core::CreateBodyCommand{
            .body = core::Body{.name = "Body " + std::to_string(new_body_counter_++),
                               .mass_kg = 1.0,
                               .radius_m = 1.0}});
    }
    if (selected_) {
        ImGui::SameLine();
        if (ImGui::Button("Duplicate")) {
            if (const auto* body = controller.scene().find(*selected_)) {
                auto duplicate = *body;
                duplicate.id = 0;
                duplicate.name += " Copy";
                controller.commands().enqueue(core::CreateBodyCommand{.body = duplicate});
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete")) {
            controller.commands().enqueue(core::DeleteBodyCommand{*selected_});
            selected_.reset();
        }
    }
    ImGui::End();
}

void EngineeringUi::drawInspector(core::SimulationController& controller,
                                  renderer::Camera& camera) {
    ImGui::Begin("Inspector");
    if (!selected_) {
        ImGui::TextUnformatted("Select a body in the hierarchy.");
        ImGui::End();
        return;
    }
    const auto* body = controller.scene().find(*selected_);
    if (body == nullptr) {
        selected_.reset();
        ImGui::End();
        return;
    }
    std::array<char, 128> name{};
    std::snprintf(name.data(), name.size(), "%s", body->name.c_str());
    if (ImGui::InputText("Name", name.data(), name.size())) {
        controller.commands().enqueue(
            core::UpdateBodyCommand{body->id, core::BodyPatch{.name = std::string{name.data()}}});
    }
    double mass = body->mass_kg;
    if (ImGui::InputDouble("Mass (kg)", &mass, 0.0, 0.0, "%.9g")) {
        controller.commands().enqueue(
            core::UpdateBodyCommand{body->id, core::BodyPatch{.mass_kg = mass}});
    }
    double radius = body->radius_m;
    if (ImGui::InputDouble("Radius (m)", &radius, 0.0, 0.0, "%.9g")) {
        controller.commands().enqueue(
            core::UpdateBodyCommand{body->id, core::BodyPatch{.radius_m = radius}});
    }
    std::array<double, 3> position = {body->state.position_m.x, body->state.position_m.y,
                                      body->state.position_m.z};
    if (ImGui::InputScalarN("Position (m)", ImGuiDataType_Double, position.data(), 3)) {
        controller.commands().enqueue(core::UpdateBodyCommand{
            body->id, core::BodyPatch{.position_m = {{position[0], position[1], position[2]}}}});
    }
    std::array<double, 3> velocity = {body->state.velocity_mps.x, body->state.velocity_mps.y,
                                      body->state.velocity_mps.z};
    if (ImGui::InputScalarN("Velocity (m/s)", ImGuiDataType_Double, velocity.data(), 3)) {
        controller.commands().enqueue(core::UpdateBodyCommand{
            body->id, core::BodyPatch{.velocity_mps = {{velocity[0], velocity[1], velocity[2]}}}});
    }
    bool fixed = body->fixed;
    if (ImGui::Checkbox("Fixed", &fixed)) {
        controller.commands().enqueue(
            core::UpdateBodyCommand{body->id, core::BodyPatch{.fixed = fixed}});
    }
    auto interactions = body->interactions;
    bool gravity = interactions.contains(core::Interaction::gravity);
    if (ImGui::Checkbox("Gravity interaction", &gravity)) {
        interactions.set(core::Interaction::gravity, gravity);
        controller.commands().enqueue(
            core::UpdateBodyCommand{body->id, core::BodyPatch{.interactions = interactions}});
    }
    if (ImGui::Button("Focus Camera"))
        camera.focus(body->state.position_m, body->radius_m);
    ImGui::End();
}

void EngineeringUi::drawSimulationControls(core::SimulationController& controller,
                                           renderer::RenderSettings& render_settings) {
    ImGui::Begin("Simulation Controls");
    if (ImGui::Button(controller.isPlaying() ? "Pause" : "Play")) {
        controller.setPlaying(!controller.isPlaying());
    }
    ImGui::SameLine();
    if (ImGui::Button("Single Step"))
        static_cast<void>(controller.singleStep());
    ImGui::SameLine();
    if (ImGui::Button("Reset"))
        controller.reset();

    double dt = controller.settings().physics_dt_s;
    if (ImGui::InputDouble("Physics dt (s)", &dt, 0.0, 0.0, "%.9g")) {
        controller.commands().enqueue(core::SetPhysicsDtCommand{dt});
    }
    double time_scale = controller.settings().time_scale;
    if (ImGui::InputDouble("Time scale", &time_scale, 0.0, 0.0, "%.6g")) {
        controller.commands().enqueue(core::SetTimeScaleCommand{time_scale});
    }
    bool gravity = controller.settings().gravity_enabled;
    if (ImGui::Checkbox("Global gravity", &gravity)) {
        controller.commands().enqueue(core::SetGravityEnabledCommand{gravity});
    }
    constexpr const char* integrator_names[] = {"Semi-Implicit Euler", "Velocity Verlet", "RK4"};
    int integrator = static_cast<int>(controller.settings().integrator);
    if (ImGui::Combo("Integrator", &integrator, integrator_names, 3)) {
        controller.commands().enqueue(
            core::SetIntegratorCommand{static_cast<physics::IntegratorKind>(integrator)});
    }
    ImGui::SeparatorText("Visualization (does not affect physics)");
    ImGui::InputDouble("Meters / render unit", &render_settings.meters_to_render_units, 0.0, 0.0,
                       "%.9g");
    ImGui::InputFloat("Minimum apparent radius", &render_settings.minimum_apparent_radius, 0.0F,
                      0.0F, "%.4g");
    ImGui::Checkbox("Object trails", &render_settings.trails_enabled);
    if (render_settings.trails_enabled) {
        constexpr double minimum_trail_s = 1.0;
        constexpr double maximum_trail_s = presets::earth_like_orbit_period_s;
        ImGui::SliderScalar("Trail duration (s)", ImGuiDataType_Double,
                            &render_settings.trail_duration_s, &minimum_trail_s, &maximum_trail_s,
                            "%.4g s", ImGuiSliderFlags_Logarithmic);
    }
    ImGui::End();
}

void EngineeringUi::drawDiagnostics(const core::SimulationController& controller) {
    ImGui::Begin("Diagnostics");
    ImGui::Text("Simulation time: %.9g s", controller.simulationTimeSeconds());
    ImGui::Text("Bodies: %zu", controller.scene().size());
    ImGui::Text("Pending commands: %zu", controller.commands().pendingCount());
    if (!controller.telemetry().samples().empty()) {
        const auto& sample = controller.telemetry().samples().back();
        ImGui::Text("Total energy: %.9g J", sample.total_energy_J);
        ImGui::Text("Momentum: [%.6g, %.6g, %.6g] kg m/s", sample.linear_momentum_kg_mps.x,
                    sample.linear_momentum_kg_mps.y, sample.linear_momentum_kg_mps.z);
        ImGui::Text("Relative energy error: %.6g", sample.relative_energy_error);
        ImGui::Text("Momentum error: %.6g kg m/s", sample.momentum_error_kg_mps);
    }
    const auto warnings =
        physics::analyzeTimestep(controller.scene(), controller.settings().physics_dt_s);
    for (const auto& warning : warnings) {
        const ImVec4 color = warning.severity == physics::StabilitySeverity::unstable
                                 ? ImVec4{1.0F, 0.25F, 0.2F, 1.0F}
                                 : ImVec4{1.0F, 0.72F, 0.2F, 1.0F};
        ImGui::TextColored(color, "dt/timescale %.3g: %s", warning.timestep_to_timescale_ratio,
                           warning.message.c_str());
    }
    const auto& events = controller.commands().eventLog();
    if (!events.empty()) {
        const auto& event = events.back();
        if (!event.accepted)
            ImGui::PushStyleColor(ImGuiCol_Text, {1.0F, 0.35F, 0.3F, 1.0F});
        ImGui::TextWrapped("Last command: %s — %s", event.description.c_str(),
                           event.detail.c_str());
        if (!event.accepted)
            ImGui::PopStyleColor();
    }
    if (ImGui::Button("Run Earth-orbit integrator comparison")) {
        constexpr std::size_t steps = 365;
        const double dt_s = presets::earth_like_orbit_period_s / static_cast<double>(steps);
        comparison_ =
            physics::IntegratorComparison::run(presets::makeEarthLikeOrbit(), dt_s, steps);
    }
    if (comparison_) {
        ImGui::SeparatorText("Comparison: one circular period");
        for (const auto& run : comparison_->runs) {
            ImGui::Text("%s: max dE/E %.3g, orbit error %.3g",
                        physics::integratorName(run.integrator).data(),
                        run.maximum_relative_energy_error, run.relative_orbit_reference_error);
        }
    }
    ImGui::End();
}

void EngineeringUi::drawPlots(const core::SimulationController& controller) {
    ImGui::Begin("Plots");
    constexpr const char* metrics[] = {"Relative energy error", "Momentum error (kg m/s)",
                                       "Total energy (J)"};
    ImGui::Combo("Metric", &plot_metric_, metrics, 3);
    const auto& samples = controller.telemetry().samples();
    std::vector<float> values;
    values.reserve(samples.size());
    for (const auto& sample : samples) {
        double value = sample.relative_energy_error;
        if (plot_metric_ == 1)
            value = sample.momentum_error_kg_mps;
        if (plot_metric_ == 2)
            value = sample.total_energy_J;
        values.push_back(static_cast<float>(value));
    }
    if (!values.empty()) {
        ImGui::PlotLines(metrics[plot_metric_], values.data(), static_cast<int>(values.size()), 0,
                         nullptr, FLT_MAX, FLT_MAX, {-1.0F, 120.0F});
    } else {
        ImGui::TextUnformatted("Run or single-step the simulation to collect telemetry.");
    }
    ImGui::End();
}

core::Status EngineeringUi::draw(core::SimulationController& controller, renderer::Camera& camera,
                                 renderer::RenderSettings& render_settings) {
    if (!initialized_) {
        return core::Error{core::ErrorCode::platform_failure, "engineering UI is not initialized"};
    }
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    drawDockSpace();
    drawHierarchy(controller);
    drawInspector(controller, camera);
    drawSimulationControls(controller, render_settings);
    drawDiagnostics(controller);
    drawPlots(controller);
    render_settings.selected_entity = selected_;
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return core::success();
}

} // namespace aetherion::ui
