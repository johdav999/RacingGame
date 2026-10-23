#pragma once
#include "combat_rules.hpp"
#include "engine/mesh_component.hpp"
#include <array>

namespace racing {
// Presentation only: one immutable resource shared by both mounts and all racers.
class PulseWeapon final : public engine::Component {
public:
    PulseWeapon();
    void install(std::shared_ptr<const engine::Mesh> mesh);
    void fit(int chassis);
    void present(const CombatState& combat,int rank);
    engine::Vec3 muzzleLocal(int side) const;
    std::array<engine::Vec3,16> boundsPoints() const;
    bool loaded() const { return modules[0]->mesh!=nullptr; }
    int rank() const {return installedRank;}
    float recoil() const {return kick;}
private:
    std::array<engine::Component*,2> mounts{};
    std::array<engine::MeshComponent*,2> modules{},flashes{};
    std::array<std::array<engine::MeshComponent*,3>,2> indicators{};
    float mountScale=1,kick=0;
    int installedRank=0;
};
}
