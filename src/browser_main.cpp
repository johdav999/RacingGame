#include "starter_scene.hpp"
#include "engine/application.hpp"
#include "engine/renderer.hpp"
#include <emscripten.h>
#include <memory>

namespace {
engine::Application app;
std::unique_ptr<engine::Renderer> renderer;
EM_JS(int, selected_backend, (), {
    const backend = new URLSearchParams(location.search).get('renderer');
    return backend === 'webgpu' ? 1 : backend === 'webgl2' ? 2 : 0;
});
EM_JS(int, hidden, (), { return document.hidden ? 1 : 0; });
EM_JS(void, ready_status, (int gl), {
    document.getElementById('status').textContent = 'Engine ready · ' + (gl ? 'WebGL 2' : 'WebGPU');
    window.racingReady = true;
    window.racingBackend = gl ? 'webgl2' : 'webgpu';
});
void frame() {
    renderer->serviceRecovery();
    app.setSuspended(hidden() != 0);
    app.advance(emscripten_get_now() / 1000.0);
    if (renderer->ready()) {
        renderer->resizeCanvas();
        renderer->render();
        ready_status(renderer->capabilities().backend == engine::RenderBackend::WebGl2);
    }
}
}
int main() {
    auto scene = std::make_unique<racing::StarterScene>();
    auto* selectedScene = scene.get();
    auto id = app.loadScene(std::move(scene));
    app.start(id);
    renderer = std::make_unique<engine::Renderer>(*selectedScene, *selectedScene->camera);
    renderer->initialize(static_cast<engine::RenderBackendPreference>(selected_backend()));
    emscripten_set_main_loop(frame, 0, true);
}
