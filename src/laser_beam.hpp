#pragma once
#include "combat_rules.hpp"
#include "engine/mesh_component.hpp"
#include <array>

namespace racing {
enum class LaserImpact { None, Surface, Vehicle, Shield };
// One bounded, car-relative shot per racer. The combat clock owns its lifetime.
class LaserBeam final : public engine::Component {
public:
    LaserBeam();
    void fire(engine::Vec3 start,engine::Vec3 end,engine::Vec3 normal,LaserImpact impact,
              const engine::Mat4& carPose=engine::Mat4::identity());
    void present(float remaining,engine::Vec3 eye,const engine::Mat4& carPose=engine::Mat4::identity());
    void reset();
    engine::Vec3 origin() const {return start;}
    engine::Vec3 endpoint() const {return end;}
    float length() const {return distance;}
    LaserImpact impact() const {return impactKind;}
private:
    engine::Vec3 start{},end{},hitNormal{0,1,0};
    engine::Vec3 localStart{},localEnd{},localNormal{0,1,0};
    float distance=0;
    LaserImpact impactKind=LaserImpact::None;
    engine::Component* shaft=nullptr;
    engine::MeshComponent *core=nullptr,*halo=nullptr,*contact=nullptr;
    std::array<engine::MeshComponent*,6> sparks{};
};
}
