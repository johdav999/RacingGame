#pragma once
#include "engine/scene.hpp"
#include "engine/camera_component.hpp"

namespace racing {
class StarterScene final : public engine::Scene {
public:
    StarterScene();
    engine::CameraComponent* camera = nullptr; // Scene-owned, valid for scene lifetime.
};
}
