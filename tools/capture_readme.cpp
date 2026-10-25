/** @file
 * Reproducible preset captures from the real OpenGL/UI framebuffer, never the user's desktop.
 * Fixed SI steps are independent of capture speed. This optional utility does not modify presets
 * or add production controls; display radii, framing, density and playback speed are illustrative.
 */
#include "aetherion/core/simulation_controller.hpp"
#include "aetherion/presets/em_presets.hpp"
#include "aetherion/presets/mechanics_presets.hpp"
#include "aetherion/renderer/opengl_backend.hpp"
#include "aetherion/ui/engineering_ui.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
using namespace aetherion;

/// Turns recoverable application failures into a failing capture command, not a misleading image.
void require(const core::Status& status) {
    if (!status)
        throw std::runtime_error(status.error().message);
}

/// Writes lossless top-down RGB PPM from this tool's current back buffer, before buffer swap.
/// OpenGL rows start at the lower left; reversing rows is file-format conversion, not a scene edit.
void writeFramebuffer(const std::filesystem::path& path, GLFWwindow* window) {
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    if (width <= 0 || height <= 0 || width > 4096 || height > 4096)
        throw std::runtime_error("capture framebuffer must be between 1 and 4096 pixels per axis");
    const auto row_bytes = static_cast<std::size_t>(width) * 3U;
    std::vector<unsigned char> pixels(row_bytes * static_cast<std::size_t>(height));
    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    if (glGetError() != GL_NO_ERROR)
        throw std::runtime_error("OpenGL framebuffer read failed");
    if (std::filesystem::exists(path))
        throw std::runtime_error("capture refuses to overwrite an existing frame: " +
                                 path.string());
    std::ofstream output(path, std::ios::binary);
    output << "P6\n" << width << ' ' << height << "\n255\n";
    for (int row = height - 1; row >= 0; --row)
        output.write(reinterpret_cast<const char*>(pixels.data() +
                                                   static_cast<std::size_t>(row) * row_bytes),
                     static_cast<std::streamsize>(row_bytes));
    output.close();
    if (!output)
        throw std::runtime_error("could not write capture frame");
}

/// Capture-only styling/framing. These are the same display-only controls available in the UI.
void configureElectricView(renderer::RenderSettings& settings, renderer::Camera& camera) {
    settings.field_visualization = presets::makeElectricDipolePreset().visualization;
    settings.field_visualization.lines.automatic_seed_count = 48;
    settings.field_visualization.lines.maximum_steps = 900;
    settings.field_visualization.lines.maximum_total_steps = 60'000;
    settings.body_radius_scale = 2.0F;
    settings.show_grid = false;
    camera.focus({}, 3.0);
}

/// Generates a named demonstration using existing scene/controller/renderer extension points.
/// Animations use 100 frames at 20 fps when packaged; simulation seconds per frame are explicit.
void capture(const std::string& scenario, const std::filesystem::path& output_directory) {
    const bool gravity = scenario == "gravity";
    const bool gyro = scenario == "gyro";
    const bool workspace = scenario == "workspace";
    const bool magnitude = scenario == "magnitude";
    if (!gravity && !gyro && !workspace && !magnitude && scenario != "dipole")
        throw std::invalid_argument("scene must be workspace, dipole, magnitude, gravity or gyro");
    const bool with_ui = workspace || magnitude;
    renderer::RenderSettings settings;
    renderer::Camera camera;
    core::Scene scene;
    core::RuntimeSettings runtime;
    if (gravity) {
        auto preset = presets::makeEarthMoonGravityPreset();
        scene = std::move(preset.scene);
        runtime = preset.runtime;
        settings.meters_to_render_units = preset.meters_to_render_units;
        settings.body_radius_scale = 6.0F;
        settings.trails_enabled = true;
        settings.trail_duration_s = 2.5e6;
        settings.selected_entity = scene.bodies().back().id;
        camera.focus({}, 4.2e8);
        camera.setTwoDimensional(true);
    } else if (gyro) {
        auto preset = presets::makeUniformMagneticGyroPreset();
        scene = std::move(preset.scene);
        runtime = preset.runtime;
        settings.field_visualization = preset.visualization;
        auto& field = settings.field_visualization;
        field.vectors.geometry = renderer::SamplingGeometry::volume;
        field.vectors.resolution = 4;
        field.vectors.visual_length_m = 0.35;
        field.region.center_m = {0.0, -1.0, 0.0};
        field.region.half_extent_m = {1.6, 1.6, 0.8};
        settings.trails_enabled = true;
        settings.trail_duration_s = 7.0;
        settings.show_grid = false;
        settings.motion_glyphs.velocity = true;
        settings.motion_glyphs.length_fraction = 0.12;
        settings.selected_entity = scene.bodies().front().id;
        camera.focus({0.0, -1.0, 0.0}, 1.5);
        camera.orbit(-0.3, -0.3);
    } else {
        auto preset = presets::makeElectricDipolePreset();
        scene = std::move(preset.scene);
        runtime = preset.runtime;
        configureElectricView(settings, camera);
        settings.selected_entity = scene.bodies().front().id;
        if (magnitude) {
            // A 1 nC dipole makes the slice's log1p intensity range easy to see in one image.
            for (auto& body : scene.bodies())
                body.charge_C *= 1.0e-3;
            auto& field = settings.field_visualization;
            field.mode = renderer::FieldDisplayMode::magnitude_plane;
            field.planar_2d = true;
            field.vectors.geometry = renderer::SamplingGeometry::plane_xz;
            field.vectors.resolution = 32;
            field.plane_follows_camera = false;
            field.region.half_extent_m = {2.5, 1.0, 2.5};
            camera.focus({}, 2.0);
            camera.setTwoDimensional(true);
            settings.probes = {{"Dipole probe", {0.0, 0.0, 0.7}}};
            settings.probes.front().show_electric_vector = true;
            settings.probes.front().vector_length_fraction = 0.12;
            settings.selected_probe = 0U;
            settings.selected_entity.reset();
        }
    }
    if (with_ui)
        camera.pan(0.0, -1.1);
    core::SimulationController controller(std::move(scene), runtime);
    renderer::OpenGlRenderer renderer;
    require(renderer.initialize(with_ui ? 1440 : 960, with_ui ? 900 : 600,
                                "Aetherion README capture", false));
    auto* window = static_cast<GLFWwindow*>(renderer.nativeWindowHandle());
    glfwSwapInterval(0);
    // UI is destroyed before the renderer so its backend still has a live OpenGL context.
    std::unique_ptr<ui::EngineeringUi> interface;
    if (with_ui) {
        interface = std::make_unique<ui::EngineeringUi>();
        require(interface->initialize(window));
        ImGui::GetIO().IniFilename = nullptr;
        if (magnitude)
            interface->selectProbe(0U);
        else
            interface->selectEntity(controller.scene().bodies().front().id);
    }
    std::filesystem::create_directories(output_directory);
    const auto render_frame = [&] {
        settings.simulation_time_s = controller.simulationTimeSeconds();
        require(renderer.render(controller.scene(), camera, settings, &controller.fieldProvider()));
        if (interface)
            require(interface->draw(controller, camera, settings));
    };
    const auto advance = [&] {
        const int steps_per_frame = gravity ? 79 : (gyro ? 2 : 0);
        for (int step = 0; step < steps_per_frame; ++step)
            require(controller.singleStep());
    };
    // Warm up docking/font resources and populate a full orbital trail before recording a loop.
    const int warmup_frames = gravity || gyro ? 100 : 8;
    for (int frame = 0; frame < warmup_frames; ++frame) {
        advance();
        render_frame();
        if (interface && frame == 5) {
            if (!magnitude)
                ImGui::SetWindowFocus("Simulation Controls");
            ImGui::SetWindowFocus(magnitude ? "Field Probes" : "Inspector");
        }
        renderer.present();
        renderer.pollEvents();
    }
    const int frame_count = gravity || gyro ? 100 : 1;
    for (int frame = 0; frame < frame_count; ++frame) {
        advance();
        render_frame();
        std::ostringstream filename;
        filename << "frame-" << std::setfill('0') << std::setw(3) << frame << ".ppm";
        writeFramebuffer(output_directory / filename.str(), window);
        renderer.present();
        renderer.pollEvents();
    }
    std::cout << scenario << ": " << frame_count << " real framebuffer frame(s) saved\n";
}
} // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: aetherion_capture <workspace|dipole|magnitude|gravity|gyro> "
                     "<empty-output-directory>\n";
        return 2;
    }
    try {
        capture(argv[1], argv[2]);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "capture failed: " << error.what() << '\n';
        return 1;
    }
}
