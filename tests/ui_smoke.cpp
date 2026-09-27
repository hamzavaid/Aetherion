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
    ui.selectEntity(controller.scene().bodies().front().id);
    controller.setPlaying(true);
    ui.handleSceneAction(controller, settings, camera,
                         aetherion::renderer::SceneActionKind::move_right);
    ui.handleSceneAction(controller, settings, camera,
                         aetherion::renderer::SceneActionKind::move_right);
    if (controller.isPlaying())
        return 10;
    if (!controller.update(0.0) || controller.scene().bodies().front().state.position_m.x <= 0.0)
        return 6;
    ui.handleSceneAction(controller, settings, camera, aetherion::renderer::SceneActionKind::undo);
    if (!controller.update(0.0) || controller.scene().bodies().front().state.position_m.x <= 0.0)
        return 11;
    ui.handleSceneAction(controller, settings, camera, aetherion::renderer::SceneActionKind::undo);
    if (!controller.update(0.0) || controller.scene().bodies().front().state.position_m.x != 0.0)
        return 7;
    settings.probes.push_back({"moveable probe", {0.0, 0.0, 0.0}});
    ui.selectProbe(0U);
    ui.handleSceneAction(controller, settings, camera,
                         aetherion::renderer::SceneActionKind::move_up);
    if (settings.probes.front().position_m.y <= 0.0)
        return 8;
    ui.handleSceneAction(controller, settings, camera,
                         aetherion::renderer::SceneActionKind::reset_position);
    if (settings.probes.front().position_m.y != 0.0)
        return 9;
    if (!renderer.render(controller.scene(), camera, settings))
        return 4;
    if (!ui.draw(controller, camera, settings))
        return 5;
    renderer.setInputCapture(ui.inputCapture());
    renderer.present();
    return 0;
}
