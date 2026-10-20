#include "starter_scene.hpp"
#include "engine/mesh_component.hpp"

namespace racing {
StarterScene::StarterScene() {
    name = "RacingGame integration starter";
    ambient = {0.65f, 0.68f, 0.75f};
    auto& ground = spawn<engine::Actor>("Garage floor");
    auto& floor = ground.addComponent<engine::MeshComponent>("Floor", engine::Mesh::cube());
    floor.transform.scale = {12, 0.1f, 12};
    floor.transform.position = {0, -0.3f, 0};
    floor.color = {0.1f, 0.13f, 0.18f};
    auto& car = spawn<engine::Actor>("Placeholder car");
    auto& body = car.addComponent<engine::MeshComponent>("Body", engine::Mesh::cube());
    body.transform.scale = {0.9f, 0.35f, 1.8f};
    body.transform.position = {0, 0.25f, 0};
    body.color = {0.9f, 0.14f, 0.06f};
    auto& cameraActor = spawn<engine::Actor>("Preview camera");
    camera = &cameraActor.addComponent<engine::CameraComponent>();
    camera->transform.position = {5, 4, 7};
    camera->lookAt({0, 0, 0});
}
}
