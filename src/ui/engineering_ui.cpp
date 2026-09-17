#include "aetherion/ui/engineering_ui.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cfloat>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

#include "aetherion/core/body_defaults.hpp"
#include "aetherion/physics/em/electrostatics.hpp"
#include "aetherion/physics/em/lorentz.hpp"
#include "aetherion/physics/stability_analyzer.hpp"
#include "aetherion/presets/em_presets.hpp"
#include "aetherion/presets/mechanics_presets.hpp"
#include "aetherion/serialization/scene_serialization.hpp"

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
        ImGui::DockBuilderDockWindow("Save History", bottom);
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
    const bool creates_charge = controller.settings().electromagnetism.electrostatics_enabled ||
                                controller.settings().electromagnetism.magnetic_enabled;
    if (ImGui::Button(creates_charge ? "Add Charged Body" : "Add Body")) {
        controller.commands().enqueue(core::CreateBodyCommand{
            .body = core::makeInteractiveBody(controller.scene(), controller.settings(),
                                              "Body " + std::to_string(new_body_counter_++))});
    }
    if (creates_charge && ImGui::IsItemHovered())
        ImGui::SetTooltip("Creates a charged, non-overlapping test body for the active EM scene.");
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

void EngineeringUi::drawInspector(core::SimulationController& controller, renderer::Camera& camera,
                                  const renderer::RenderSettings& render_settings) {
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
    double charge = body->charge_C;
    if (ImGui::InputDouble("Charge (C)", &charge, 0.0, 0.0, "%.9g")) {
        controller.commands().enqueue(
            core::UpdateBodyCommand{body->id, core::BodyPatch{.charge_C = charge}});
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
    bool electrostatic = interactions.contains(core::Interaction::electrostatic);
    if (ImGui::Checkbox("Electrostatic interaction", &electrostatic)) {
        interactions.set(core::Interaction::electrostatic, electrostatic);
        controller.commands().enqueue(
            core::UpdateBodyCommand{body->id, core::BodyPatch{.interactions = interactions}});
    }
    bool magnetic = interactions.contains(core::Interaction::magnetic);
    if (ImGui::Checkbox("Magnetic interaction", &magnetic)) {
        interactions.set(core::Interaction::magnetic, magnetic);
        controller.commands().enqueue(
            core::UpdateBodyCommand{body->id, core::BodyPatch{.interactions = interactions}});
    }
    if (ImGui::Button("Focus Camera"))
        camera.focus(body->state.position_m,
                     renderer::visualBodyRadiusMeters(*body, render_settings));
    ImGui::End();
}

void EngineeringUi::drawSimulationControls(core::SimulationController& controller,
                                           renderer::Camera& camera,
                                           renderer::RenderSettings& render_settings) {
    ImGui::Begin("Simulation Controls");
    if (ImGui::Button(controller.isPlaying() ? "Pause" : "Play")) {
        controller.setPlaying(!controller.isPlaying());
    }
    ImGui::SameLine();
    if (ImGui::Button("Single Step"))
        static_cast<void>(controller.singleStep());
    ImGui::SameLine();
    if (ImGui::Button("Save"))
        static_cast<void>(controller.saveCheckpoint());
    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        controller.reset();
        if (selected_ && controller.scene().find(*selected_) == nullptr)
            selected_.reset();
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Restore the most recent save, or the startup preset if none exists.");

    if (ImGui::Button("Save Scene JSON")) {
        const serialization::SceneDocument document{controller.scene(), controller.settings(),
                                                    render_settings};
        const auto status = serialization::saveSceneFile("aetherion_scene.json", document);
        scene_file_status_ = status ? "Saved aetherion_scene.json" : status.error().message;
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Scene JSON")) {
        const auto document = serialization::loadSceneFile("aetherion_scene.json");
        if (!document) {
            scene_file_status_ = document.error().message;
        } else {
            auto loaded = document.value();
            const auto status =
                controller.loadState(std::move(loaded.scene), std::move(loaded.runtime));
            if (status) {
                render_settings = std::move(loaded.visualization);
                const auto bounds =
                    renderer::calculateSceneFocusBounds(controller.scene(), render_settings);
                const auto& field = render_settings.field_visualization;
                const double field_radius =
                    field.mode == renderer::FieldDisplayMode::none
                        ? 0.0
                        : (field.region.center_m - bounds.center_world_m).norm() +
                              field.region.half_extent_m.norm();
                camera.focus(bounds.center_world_m, std::max(bounds.radius_m, field_radius));
                selected_.reset();
                scene_file_status_ = "Loaded aetherion_scene.json";
            } else {
                scene_file_status_ = status.error().message;
            }
        }
    }
    if (!scene_file_status_.empty())
        ImGui::TextWrapped("%s", scene_file_status_.c_str());

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
    auto electromagnetic = controller.settings().electromagnetism;
    bool electrostatics = electromagnetic.electrostatics_enabled;
    if (ImGui::Checkbox("Electrostatics", &electrostatics)) {
        electromagnetic.electrostatics_enabled = electrostatics;
        controller.commands().enqueue(core::SetElectromagneticSettingsCommand{electromagnetic});
    }
    bool magnetic = electromagnetic.magnetic_enabled;
    if (ImGui::Checkbox("Magnetic dynamics", &magnetic)) {
        electromagnetic.magnetic_enabled = magnetic;
        controller.commands().enqueue(core::SetElectromagneticSettingsCommand{electromagnetic});
    }
    auto uniform_source =
        std::find_if(electromagnetic.analytic_sources.begin(),
                     electromagnetic.analytic_sources.end(), [](const auto& source) {
                         return source.kind == physics::em::AnalyticFieldSourceKind::uniform;
                     });
    physics::em::AnalyticFieldSource uniform;
    if (uniform_source != electromagnetic.analytic_sources.end())
        uniform = *uniform_source;
    std::array<double, 3> uniform_e = {uniform.electric_Vpm.x, uniform.electric_Vpm.y,
                                       uniform.electric_Vpm.z};
    std::array<double, 3> uniform_b = {uniform.magnetic_T.x, uniform.magnetic_T.y,
                                       uniform.magnetic_T.z};
    const bool electric_changed =
        ImGui::InputScalarN("Uniform E (V/m)", ImGuiDataType_Double, uniform_e.data(), 3);
    const bool magnetic_changed =
        ImGui::InputScalarN("Uniform B (T)", ImGuiDataType_Double, uniform_b.data(), 3);
    if (electric_changed || magnetic_changed) {
        uniform.electric_Vpm = {uniform_e[0], uniform_e[1], uniform_e[2]};
        uniform.magnetic_T = {uniform_b[0], uniform_b[1], uniform_b[2]};
        if (uniform_source == electromagnetic.analytic_sources.end())
            electromagnetic.analytic_sources.push_back(uniform);
        else
            *uniform_source = uniform;
        controller.commands().enqueue(core::SetElectromagneticSettingsCommand{electromagnetic});
    }
    ImGui::SeparatorText("Analytic Field Sources");
    if (ImGui::SmallButton("Add uniform source")) {
        electromagnetic.analytic_sources.push_back({});
        controller.commands().enqueue(core::SetElectromagneticSettingsCommand{electromagnetic});
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Add magnetic dipole")) {
        electromagnetic.analytic_sources.push_back(
            {.kind = physics::em::AnalyticFieldSourceKind::magnetic_dipole,
             .magnetic_dipole_moment_Am2 = {0.0, 0.0, 1.0}});
        controller.commands().enqueue(core::SetElectromagneticSettingsCommand{electromagnetic});
    }
    bool sources_changed = false;
    int source_to_remove = -1;
    for (std::size_t index = 0; index < electromagnetic.analytic_sources.size(); ++index) {
        auto& source = electromagnetic.analytic_sources[index];
        ImGui::PushID(static_cast<int>(index));
        if (ImGui::TreeNode("Source", "Source %zu", index + 1U)) {
            constexpr const char* source_kinds[] = {"Uniform", "Magnetic dipole"};
            int kind = static_cast<int>(source.kind);
            if (ImGui::Combo("Kind", &kind, source_kinds, 2)) {
                source.kind = static_cast<physics::em::AnalyticFieldSourceKind>(kind);
                sources_changed = true;
            }
            std::array<double, 3> source_position = {source.position_m.x, source.position_m.y,
                                                     source.position_m.z};
            if (ImGui::InputScalarN("Position (m)", ImGuiDataType_Double, source_position.data(),
                                    3)) {
                source.position_m = {source_position[0], source_position[1], source_position[2]};
                sources_changed = true;
            }
            if (source.kind == physics::em::AnalyticFieldSourceKind::uniform) {
                std::array<double, 3> electric = {source.electric_Vpm.x, source.electric_Vpm.y,
                                                  source.electric_Vpm.z};
                std::array<double, 3> magnetic_field = {source.magnetic_T.x, source.magnetic_T.y,
                                                        source.magnetic_T.z};
                if (ImGui::InputScalarN("E (V/m)", ImGuiDataType_Double, electric.data(), 3)) {
                    source.electric_Vpm = {electric[0], electric[1], electric[2]};
                    sources_changed = true;
                }
                if (ImGui::InputScalarN("B (T)", ImGuiDataType_Double, magnetic_field.data(), 3)) {
                    source.magnetic_T = {magnetic_field[0], magnetic_field[1], magnetic_field[2]};
                    sources_changed = true;
                }
            } else {
                std::array<double, 3> moment = {source.magnetic_dipole_moment_Am2.x,
                                                source.magnetic_dipole_moment_Am2.y,
                                                source.magnetic_dipole_moment_Am2.z};
                if (ImGui::InputScalarN("Dipole moment (A m^2)", ImGuiDataType_Double,
                                        moment.data(), 3)) {
                    source.magnetic_dipole_moment_Am2 = {moment[0], moment[1], moment[2]};
                    sources_changed = true;
                }
                if (ImGui::InputDouble("Singularity radius (m)", &source.singularity_radius_m, 0.0,
                                       0.0, "%.6g"))
                    sources_changed = true;
            }
            if (ImGui::SmallButton("Remove source"))
                source_to_remove = static_cast<int>(index);
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    if (source_to_remove >= 0) {
        electromagnetic.analytic_sources.erase(electromagnetic.analytic_sources.begin() +
                                               source_to_remove);
        sources_changed = true;
    }
    if (sources_changed)
        controller.commands().enqueue(core::SetElectromagneticSettingsCommand{electromagnetic});
    double electric_guard_m = electromagnetic.minimum_separation_m;
    if (ImGui::InputDouble("EM guard radius (m)", &electric_guard_m, 0.0, 0.0, "%.6g")) {
        electromagnetic.minimum_separation_m = electric_guard_m;
        controller.commands().enqueue(core::SetElectromagneticSettingsCommand{electromagnetic});
    }
    constexpr const char* integrator_names[] = {"Semi-Implicit Euler", "Velocity Verlet", "RK4",
                                                "Boris"};
    int integrator = static_cast<int>(controller.settings().integrator);
    if (ImGui::Combo("Integrator", &integrator, integrator_names, 4)) {
        controller.commands().enqueue(
            core::SetIntegratorCommand{static_cast<physics::IntegratorKind>(integrator)});
    }
    ImGui::SeparatorText("Visualization (does not affect physics)");
    ImGui::Checkbox("Show engineering grid", &render_settings.show_grid);
    ImGui::InputDouble("Render units / meter", &render_settings.meters_to_render_units, 0.0, 0.0,
                       "%.9g");
    ImGui::InputFloat("Minimum apparent radius", &render_settings.minimum_apparent_radius, 0.0F,
                      0.0F, "%.4g");
    constexpr float minimum_radius_scale = 0.001F;
    constexpr float maximum_radius_scale = 1.0e6F;
    ImGui::SliderFloat("Body radius multiplier", &render_settings.body_radius_scale,
                       minimum_radius_scale, maximum_radius_scale, "%.4gx",
                       ImGuiSliderFlags_Logarithmic);
    ImGui::TextDisabled("1x preserves true relative radii; display only.");
    ImGui::Checkbox("Object trails", &render_settings.trails_enabled);
    if (render_settings.trails_enabled) {
        constexpr double minimum_trail_s = 1.0;
        constexpr double maximum_trail_s = presets::earth_like_orbit_period_s;
        ImGui::SliderScalar("Trail duration (s)", ImGuiDataType_Double,
                            &render_settings.trail_duration_s, &minimum_trail_s, &maximum_trail_s,
                            "%.4g s", ImGuiSliderFlags_Logarithmic);
    }
    ImGui::SeparatorText("Electric / Magnetic / Gravity Field Display");
    auto& field = render_settings.field_visualization;
    if (ImGui::Button(field.planar_2d ? "Return to 3D field view" : "Show 2D field view")) {
        field.planar_2d = !field.planar_2d;
        if (field.planar_2d) {
            field.vectors.geometry = renderer::SamplingGeometry::plane_xz;
            if (field.mode == renderer::FieldDisplayMode::none)
                field.mode = renderer::FieldDisplayMode::observed_vectors;
        }
    }
    if (field.planar_2d)
        ImGui::TextDisabled("Orthographic XZ view; out-of-plane field components are hidden.");
    constexpr const char* field_modes[] = {"None", "Observed Vector Field", "Field Lines"};
    int field_mode = static_cast<int>(field.mode);
    if (ImGui::Combo("Display mode", &field_mode, field_modes, 3))
        field.mode = static_cast<renderer::FieldDisplayMode>(field_mode);
    constexpr const char* field_types[] = {"Electric (V/m)", "Magnetic (T)", "Gravity (m/s^2)"};
    int field_type = static_cast<int>(field.field);
    if (ImGui::Combo("Observed field", &field_type, field_types, 3))
        field.field = static_cast<renderer::ObservedField>(field_type);
    std::array<double, 3> region_center = {field.region.center_m.x, field.region.center_m.y,
                                           field.region.center_m.z};
    if (ImGui::InputScalarN("Field center (m)", ImGuiDataType_Double, region_center.data(), 3))
        field.region.center_m = {region_center[0], region_center[1], region_center[2]};
    std::array<double, 3> region_extent = {
        field.region.half_extent_m.x, field.region.half_extent_m.y, field.region.half_extent_m.z};
    if (ImGui::InputScalarN("Field half extent (m)", ImGuiDataType_Double, region_extent.data(), 3))
        field.region.half_extent_m = {region_extent[0], region_extent[1], region_extent[2]};
    if (field.mode == renderer::FieldDisplayMode::observed_vectors) {
        constexpr const char* geometries[] = {"3D volume", "XY plane", "XZ plane", "YZ plane"};
        int geometry = static_cast<int>(field.vectors.geometry);
        if (ImGui::Combo("Sampling region", &geometry, geometries, 4))
            field.vectors.geometry = static_cast<renderer::SamplingGeometry>(geometry);
        int resolution = static_cast<int>(field.vectors.resolution);
        if (ImGui::SliderInt("Field resolution", &resolution, 2, 24))
            field.vectors.resolution = static_cast<std::size_t>(resolution);
        ImGui::InputDouble("Vector length (m)", &field.vectors.visual_length_m, 0.0, 0.0, "%.6g");
        constexpr const char* scales[] = {"Normalized", "Logarithmic", "Linear"};
        int scaling = static_cast<int>(field.vectors.scaling);
        if (ImGui::Combo("Vector scaling", &scaling, scales, 3))
            field.vectors.scaling = static_cast<renderer::VectorScaling>(scaling);
        ImGui::InputDouble("Reference magnitude", &field.vectors.reference_magnitude, 0.0, 0.0,
                           "%.6g");
        ImGui::InputDouble("Minimum magnitude", &field.vectors.minimum_magnitude, 0.0, 0.0, "%.6g");
        ImGui::InputDouble("Maximum magnitude", &field.vectors.maximum_magnitude, 0.0, 0.0, "%.6g");
    }
    if (field.mode == renderer::FieldDisplayMode::field_lines) {
        int seeds = static_cast<int>(field.lines.automatic_seed_count);
        if (ImGui::SliderInt("Automatic seeds", &seeds, 0, 128))
            field.lines.automatic_seed_count = static_cast<std::size_t>(seeds);
        ImGui::InputDouble("Trace step (m)", &field.lines.step_size_m, 0.0, 0.0, "%.6g");
        int maximum_steps = static_cast<int>(field.lines.maximum_steps);
        if (ImGui::SliderInt("Maximum trace steps", &maximum_steps, 1, 4000))
            field.lines.maximum_steps = static_cast<std::size_t>(maximum_steps);
        ImGui::InputDouble("Maximum line length (m)", &field.lines.maximum_length_m, 0.0, 0.0,
                           "%.6g");
        ImGui::InputDouble("Trace termination field", &field.lines.minimum_field_magnitude, 0.0,
                           0.0, "%.6g");
        ImGui::Checkbox("Trace forward", &field.lines.trace_forward);
        ImGui::SameLine();
        ImGui::Checkbox("Trace backward", &field.lines.trace_backward);
        if (ImGui::SmallButton("Add seed at region center"))
            field.lines.custom_seeds_m.push_back(field.region.center_m);
        int seed_to_remove = -1;
        for (std::size_t index = 0; index < field.lines.custom_seeds_m.size(); ++index) {
            auto& seed = field.lines.custom_seeds_m[index];
            std::array<double, 3> value = {seed.x, seed.y, seed.z};
            ImGui::PushID(static_cast<int>(index));
            if (ImGui::InputScalarN("Custom seed (m)", ImGuiDataType_Double, value.data(), 3))
                seed = {value[0], value[1], value[2]};
            ImGui::SameLine();
            if (ImGui::SmallButton("Remove"))
                seed_to_remove = static_cast<int>(index);
            ImGui::PopID();
        }
        if (seed_to_remove >= 0)
            field.lines.custom_seeds_m.erase(field.lines.custom_seeds_m.begin() + seed_to_remove);
    }
    if (field.mode != renderer::FieldDisplayMode::none) {
        ImGui::SeparatorText("Active Field Color");
        const bool vectors = field.mode == renderer::FieldDisplayMode::observed_vectors;
        if (field.field == renderer::ObservedField::electric) {
            ImGui::ColorEdit3(vectors ? "Electric vectors" : "Electric field lines",
                              vectors ? field.colors.electric_vectors.data()
                                      : field.colors.electric_lines.data());
        } else if (field.field == renderer::ObservedField::magnetic) {
            ImGui::ColorEdit3(vectors ? "Magnetic vectors" : "Magnetic field lines",
                              vectors ? field.colors.magnetic_vectors.data()
                                      : field.colors.magnetic_lines.data());
        } else {
            ImGui::ColorEdit3(vectors ? "Gravity vectors" : "Gravity field lines",
                              vectors ? field.colors.gravity_vectors.data()
                                      : field.colors.gravity_lines.data());
        }
    }
    ImGui::SeparatorText("Gravity Presets");
    const auto load_gravity_preset = [&](presets::GravityPreset preset) {
        if (controller.loadState(std::move(preset.scene), std::move(preset.runtime))) {
            render_settings.meters_to_render_units = preset.meters_to_render_units;
            render_settings.minimum_apparent_radius = preset.minimum_apparent_radius;
            render_settings.body_radius_scale = preset.body_radius_scale;
            const auto bounds =
                renderer::calculateSceneFocusBounds(controller.scene(), render_settings);
            auto& gravity_field = render_settings.field_visualization;
            gravity_field.mode = renderer::FieldDisplayMode::field_lines;
            gravity_field.field = renderer::ObservedField::gravity;
            gravity_field.planar_2d = false;
            gravity_field.region.center_m = bounds.center_world_m;
            const double field_radius_m = std::max(bounds.radius_m * 1.25, 1.0);
            gravity_field.region.half_extent_m = {field_radius_m, field_radius_m, field_radius_m};
            gravity_field.lines.automatic_seed_count = 32;
            gravity_field.lines.step_size_m = field_radius_m / 200.0;
            gravity_field.lines.maximum_length_m = field_radius_m * 3.0;
            gravity_field.lines.minimum_field_magnitude = 0.0;
            camera.focus(bounds.center_world_m, bounds.radius_m);
            selected_.reset();
        }
    };
    if (ImGui::Button("Earth-Sun"))
        load_gravity_preset(presets::makeEarthSunGravityPreset());
    ImGui::SameLine();
    if (ImGui::Button("Earth-Moon"))
        load_gravity_preset(presets::makeEarthMoonGravityPreset());
    ImGui::SameLine();
    if (ImGui::Button("Sun-Earth-Moon"))
        load_gravity_preset(presets::makeSunEarthMoonGravityPreset());
    ImGui::SeparatorText("Electrostatic Presets");
    const auto load_preset = [&](presets::ElectromagneticPreset preset) {
        if (controller.loadState(std::move(preset.scene), std::move(preset.runtime))) {
            render_settings.field_visualization = std::move(preset.visualization);
            render_settings.meters_to_render_units = preset.meters_to_render_units;
            render_settings.minimum_apparent_radius = preset.minimum_apparent_radius;
            render_settings.body_radius_scale = preset.body_radius_scale;
            const auto bounds =
                renderer::calculateSceneFocusBounds(controller.scene(), render_settings);
            const auto& preset_field = render_settings.field_visualization;
            const double field_radius =
                preset_field.mode == renderer::FieldDisplayMode::none
                    ? 0.0
                    : (preset_field.region.center_m - bounds.center_world_m).norm() +
                          preset_field.region.half_extent_m.norm();
            camera.focus(bounds.center_world_m, std::max(bounds.radius_m, field_radius));
            selected_.reset();
        }
    };
    if (ImGui::Button("Like charges"))
        load_preset(presets::makeLikeChargesPreset());
    ImGui::SameLine();
    if (ImGui::Button("Opposite charges"))
        load_preset(presets::makeOppositeChargesPreset());
    ImGui::SameLine();
    if (ImGui::Button("Electric dipole"))
        load_preset(presets::makeElectricDipolePreset());
    ImGui::SeparatorText("Magnetic Presets");
    if (ImGui::Button("Uniform-B gyro"))
        load_preset(presets::makeUniformMagneticGyroPreset());
    ImGui::SameLine();
    if (ImGui::Button("Helical motion"))
        load_preset(presets::makeHelicalMagneticPreset());
    ImGui::SameLine();
    if (ImGui::Button("Crossed E/B"))
        load_preset(presets::makeCrossedFieldsPreset());
    if (ImGui::Button("Magnetic vectors"))
        load_preset(presets::makeMagneticVectorPreset());
    ImGui::SameLine();
    if (ImGui::Button("Magnetic field lines"))
        load_preset(presets::makeMagneticFieldLinesPreset());
    camera.setTwoDimensional(render_settings.field_visualization.planar_2d);
    ImGui::End();
}

void EngineeringUi::drawSaveHistory(core::SimulationController& controller) {
    ImGui::Begin("Save History");
    const auto& checkpoints = controller.checkpoints();
    if (checkpoints.empty()) {
        ImGui::TextDisabled("No saves in this session. Use Save in Simulation Controls.");
    } else {
        for (auto checkpoint = checkpoints.rbegin(); checkpoint != checkpoints.rend();
             ++checkpoint) {
            ImGui::PushID(static_cast<int>(checkpoint->id));
            if (ImGui::SmallButton("Restore")) {
                static_cast<void>(controller.restoreCheckpoint(checkpoint->id));
                if (selected_ && controller.scene().find(*selected_) == nullptr)
                    selected_.reset();
            }
            ImGui::SameLine();
            ImGui::Text("Save %llu | t=%.9g s | %zu bodies",
                        static_cast<unsigned long long>(checkpoint->id),
                        checkpoint->simulation_time_s, checkpoint->scene.size());
            ImGui::PopID();
        }
    }
    ImGui::End();
}

void EngineeringUi::drawDiagnostics(const core::SimulationController& controller) {
    ImGui::Begin("Diagnostics");
    ImGui::Text("Simulation time: %.9g s", controller.simulationTimeSeconds());
    ImGui::Text("Bodies: %zu", controller.scene().size());
    ImGui::Text("Pending commands: %zu", controller.commands().pendingCount());
    if (selected_) {
        if (const auto* body = controller.scene().find(*selected_)) {
            const auto field = physics::em::sampleAnalyticField(
                controller.settings().electromagnetism, body->state.position_m,
                controller.simulationTimeSeconds());
            ImGui::Text("Selected speed: %.9g m/s", body->state.velocity_mps.norm());
            ImGui::Text("Selected kinetic energy: %.9g J",
                        0.5 * body->mass_kg * body->state.velocity_mps.squaredNorm());
            if (field.valid) {
                const auto force =
                    physics::em::lorentzForce(body->charge_C, body->state.velocity_mps, field);
                ImGui::Text("|E|: %.9g V/m, |B|: %.9g T", field.electric_Vpm.norm(),
                            field.magnetic_T.norm());
                if (force) {
                    ImGui::Text("Electric force: [%.5g, %.5g, %.5g] N", force.value().electric_N.x,
                                force.value().electric_N.y, force.value().electric_N.z);
                    ImGui::Text("Magnetic force: [%.5g, %.5g, %.5g] N", force.value().magnetic_N.x,
                                force.value().magnetic_N.y, force.value().magnetic_N.z);
                }
                const auto gyro = physics::em::gyroDiagnostics(
                    body->mass_kg, body->charge_C, body->state.velocity_mps, field.magnetic_T);
                if (gyro) {
                    ImGui::Text("Gyro radius: %.9g m", gyro.value().radius_m);
                    ImGui::Text("Gyro omega/period: %.9g rad/s / %.9g s",
                                gyro.value().angular_frequency_rad_ps, gyro.value().period_s);
                    const double resolution =
                        controller.settings().physics_dt_s / gyro.value().period_s;
                    if (resolution > 0.02) {
                        const ImVec4 color = resolution > 0.1 ? ImVec4{1.0F, 0.25F, 0.2F, 1.0F}
                                                              : ImVec4{1.0F, 0.72F, 0.2F, 1.0F};
                        ImGui::TextColored(color, "Gyro timestep warning: dt/T = %.3g", resolution);
                    }
                }
            }
        }
    }
    if (!controller.telemetry().samples().empty()) {
        const auto& sample = controller.telemetry().samples().back();
        ImGui::Text("Total energy: %.9g J", sample.total_energy_J);
        ImGui::Text("Momentum: [%.6g, %.6g, %.6g] kg m/s", sample.linear_momentum_kg_mps.x,
                    sample.linear_momentum_kg_mps.y, sample.linear_momentum_kg_mps.z);
        ImGui::Text("Relative energy error: %.6g", sample.relative_energy_error);
        ImGui::Text("Momentum error: %.6g kg m/s", sample.momentum_error_kg_mps);
        ImGui::Text("Maximum speed drift: %.6g m/s", sample.maximum_speed_drift_mps);
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
                                       "Total energy (J)", "Maximum speed drift (m/s)"};
    ImGui::Combo("Metric", &plot_metric_, metrics, 4);
    const auto& samples = controller.telemetry().samples();
    std::vector<float> values;
    values.reserve(samples.size());
    for (const auto& sample : samples) {
        double value = sample.relative_energy_error;
        if (plot_metric_ == 1)
            value = sample.momentum_error_kg_mps;
        if (plot_metric_ == 2)
            value = sample.total_energy_J;
        if (plot_metric_ == 3)
            value = sample.maximum_speed_drift_mps;
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
    drawInspector(controller, camera, render_settings);
    drawSimulationControls(controller, camera, render_settings);
    drawSaveHistory(controller);
    drawDiagnostics(controller);
    drawPlots(controller);
    render_settings.selected_entity = selected_;
    const auto& io = ImGui::GetIO();
    input_capture_ = {.mouse = io.WantCaptureMouse, .keyboard = io.WantCaptureKeyboard};
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return core::success();
}

} // namespace aetherion::ui
