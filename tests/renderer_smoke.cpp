#include "aetherion/core/scene.hpp"
#include "aetherion/renderer/camera.hpp"
#include "aetherion/renderer/opengl_backend.hpp"

#include <iostream>

int main() {
    aetherion::renderer::OpenGlRenderer renderer;
    const auto initialized = renderer.initialize(640, 360, "Aetherion renderer smoke", false);
    if (!initialized) {
        std::cerr << initialized.error().message << '\n';
        return 1;
    }
    aetherion::renderer::Camera camera;
    aetherion::core::Scene local_scene;
    const auto local_id = local_scene.createBody({.name = "local",
                                                  .mass_kg = 1.0,
                                                  .radius_m = 1.0,
                                                  .state = {.position_m = {0.0, 0.0, 0.0}}});
    if (!local_id) {
        return 2;
    }
    aetherion::renderer::RenderSettings local_settings{.selected_entity = local_id.value(),
                                                       .trails_enabled = true,
                                                       .trail_duration_s = 10.0,
                                                       .simulation_time_s = 0.0};
    if (!renderer.render(local_scene, camera, local_settings)) {
        return 3;
    }
    renderer.present();
    local_scene.bodies()[0].state.position_m.x = 1.0;
    local_settings.simulation_time_s = 1.0;
    if (!renderer.render(local_scene, camera, local_settings))
        return 6;
    renderer.present();

    aetherion::core::Scene astronomical_scene;
    if (!astronomical_scene.createBody({.name = "astronomical",
                                        .mass_kg = 5.9722e24,
                                        .radius_m = 6.371e6,
                                        .state = {.position_m = {1.5e11, 0.0, 0.0}}})) {
        return 4;
    }
    camera.focus({1.5e11, 0.0, 0.0}, 2.0e7);
    if (!renderer.render(astronomical_scene, camera,
                         {.meters_to_render_units = 1.0e-7, .minimum_apparent_radius = 0.01F})) {
        return 5;
    }
    renderer.present();
    return 0;
}
