#include "aetherion/core/log.hpp"

#ifdef AETHERION_HAS_UI
#include "aetherion/core/simulation_controller.hpp"
#include "aetherion/presets/mechanics_presets.hpp"
#include "aetherion/renderer/camera.hpp"
#include "aetherion/renderer/opengl_backend.hpp"
#include "aetherion/ui/engineering_ui.hpp"

#include <algorithm>
#include <chrono>
#include <utility>
#endif

int main() {
#ifdef AETHERION_HAS_UI
    auto scene = aetherion::presets::makeEarthLikeOrbit();
    aetherion::core::SimulationController controller(
        std::move(scene), {.physics_dt_s = 3600.0, .time_scale = 86'400.0, .max_substeps = 16});
    aetherion::renderer::OpenGlRenderer renderer;
    const auto renderer_status = renderer.initialize(1440, 900, "Aetherion Engineering Simulator");
    if (!renderer_status) {
        aetherion::core::Logger::write(aetherion::core::LogLevel::error,
                                       renderer_status.error().message);
        return 1;
    }
    aetherion::ui::EngineeringUi ui;
    const auto ui_status = ui.initialize(renderer.nativeWindowHandle());
    if (!ui_status) {
        aetherion::core::Logger::write(aetherion::core::LogLevel::error, ui_status.error().message);
        return 1;
    }
    aetherion::renderer::Camera camera;
    camera.focus({}, aetherion::presets::astronomical_unit_m * 0.55);
    aetherion::renderer::RenderSettings render_settings{.meters_to_render_units = 1.0e-9,
                                                        .minimum_apparent_radius = 0.35F};
    auto previous = std::chrono::steady_clock::now();
    while (!renderer.shouldClose()) {
        const auto now = std::chrono::steady_clock::now();
        const double real_delta_s =
            std::clamp(std::chrono::duration<double>(now - previous).count(), 0.0, 0.25);
        previous = now;
        const auto simulation_status = controller.update(real_delta_s);
        if (!simulation_status) {
            aetherion::core::Logger::write(aetherion::core::LogLevel::error,
                                           simulation_status.error().message);
            controller.setPlaying(false);
        }
        const auto render_status = renderer.render(controller.scene(), camera, render_settings);
        if (!render_status) {
            aetherion::core::Logger::write(aetherion::core::LogLevel::error,
                                           render_status.error().message);
            return 1;
        }
        const auto draw_status = ui.draw(controller, camera, render_settings);
        if (!draw_status)
            return 1;
        renderer.present();
        renderer.pollEvents();
    }
    return 0;
#else
    aetherion::core::Logger::write(aetherion::core::LogLevel::info,
                                   "Aetherion engineering runtime initialized");
    return 0;
#endif
}
