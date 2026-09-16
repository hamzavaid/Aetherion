#include "aetherion/core/simulation_controller.hpp"
#include "aetherion/renderer/opengl_backend.hpp"
#include "aetherion/ui/engineering_ui.hpp"

#include <utility>

int main() {
    aetherion::core::Scene scene;
    if (!scene.createBody({.name = "editable", .mass_kg = 1.0, .radius_m = 1.0}))
        return 1;
    aetherion::core::SimulationController controller(std::move(scene));
    aetherion::renderer::OpenGlRenderer renderer;
    if (!renderer.initialize(800, 600, "Aetherion UI smoke", false))
        return 2;
    aetherion::ui::EngineeringUi ui;
    if (!ui.initialize(renderer.nativeWindowHandle()))
        return 3;
    aetherion::renderer::Camera camera;
    aetherion::renderer::RenderSettings settings;
    if (!renderer.render(controller.scene(), camera, settings))
        return 4;
    if (!ui.draw(controller, camera, settings))
        return 5;
    renderer.present();
    return 0;
}
