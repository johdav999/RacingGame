#pragma once
#include "combat_rules.hpp"
#include "engine/mesh_component.hpp"
#include <array>

namespace racing {
// Car-owned presentation only. CombatState remains the authority for protection.
// All geometry/textures are immutable and shared; three impact slots bound bursts.
class ShieldEffect final : public engine::Component {
public:
    ShieldEffect();
    void fit(engine::Vec3 center,engine::Vec3 radii);
    void reset();
    void impact(engine::Vec3 localDirection);
    float impactRay(engine::Vec3 localOrigin,engine::Vec3 localDirection);
    void advance(float dt,const CombatState& combat);
    void present(engine::Vec3 worldEye);
    float visibility() const { return envelope; }
    float breakAmount() const { return collapse; }
    unsigned activeImpacts() const;
private:
    struct Ripple { engine::MeshComponent* mesh=nullptr;engine::Vec3 direction{0,0,-1};float remaining=0; };
    engine::MeshComponent *shell=nullptr,*rim=nullptr,*scan=nullptr;
    std::array<Ripple,3> ripples{};
    engine::Vec3 center{},radii{1.7f,1.2f,3.f};
    float envelope=0,clock=0,collapse=0,energy=1,ignition=0;
    bool active=false,hitPending=false;
    unsigned nextRipple=0;
};
}
