#include "starter_scene.hpp"
#include "engine/application.hpp"
#include <iostream>
#include <stdexcept>

int main() {
    try {
        engine::Application app;
        auto scene = std::make_unique<racing::StarterScene>();
        auto* camera = scene->camera;
        const auto id = app.loadScene(std::move(scene));
        app.start(id);
        app.advance(0);
        app.advance(1.0 / 60.0);
        if (!app.activeScene() || app.activeScene()->children().size() != 3 || !camera)
            throw std::runtime_error("Starter scene did not load its three actors and camera");
        const auto projected = camera->worldToScreen({0, 0, 0}, {0, 0, 1280, 720});
        if (!projected.visible) throw std::runtime_error("Preview camera cannot see the car");
        app.shutdown();
        if (app.state() != engine::ApplicationState::Stopped)
            throw std::runtime_error("Application failed to shut down");
        std::cout << "RacingGame engine integration passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
