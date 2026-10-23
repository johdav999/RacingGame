#include "pulse_weapon.hpp"
#include <algorithm>

namespace racing {
using namespace engine;
PulseWeapon::PulseWeapon():Component("Pulse weapon assembly") {
    static const auto flashGeometry=Mesh::sphere(12,6);
    for(int side=0;side<2;++side) {
        auto& mount=addChild<Component>(side?"Pulse right hardpoint":"Pulse left hardpoint");mounts[side]=&mount;
        modules[side]=&mount.addChild<MeshComponent>("Pulse cannon module",nullptr);
        modules[side]->color={1,1,1};modules[side]->enabled=false;modules[side]->allowInstanceReordering=true;
        auto& shoe=mount.addChild<MeshComponent>("Pulse hardpoint shoe",Mesh::cube());
        shoe.transform.position={0,-.035f,0};shoe.transform.scale={.20f,.10f,.50f};shoe.color={.075f,.10f,.13f};
        MaterialInstance metal;metal.metallic=.65f;metal.roughness=.48f;shoe.setMaterial(0,metal);
        flashes[side]=&mount.addChild<MeshComponent>("Pulse muzzle flash",flashGeometry);
        auto& flash=*flashes[side];flash.transform.scale={.09f,.09f,.23f};flash.enabled=false;
        flash.blend=BlendMode::Additive;flash.castsShadow=false;flash.receivesDecals=false;
        MaterialInstance glow;glow.shader=MaterialShader::Unlit;flash.setMaterial(0,glow);flash.color={.3f,.9f,1};
        for(int level=0;level<3;++level) {
            auto& light=mount.addChild<MeshComponent>("Pulse upgrade indicator",Mesh::cube());
            indicators[side][level]=&light;light.transform.position={0,.325f,-.26f+level*.09f};
            light.transform.scale={.13f,.012f,.045f};light.color={.1f,.9f,1};
            light.castsShadow=false;light.receivesDecals=false;light.setMaterial(0,glow);light.enabled=false;
        }
    }
    fit(0);
}
void PulseWeapon::install(std::shared_ptr<const Mesh> mesh) {
    for(auto* module:modules){module->mesh=mesh;module->enabled=true;}
}
void PulseWeapon::fit(int chassis) {
    const bool hover=chassis==1||chassis==3;
    mountScale=hover?.78f:1.f;
    for(int side=0;side<2;++side) {
        // Fitted to each imported hood/pod; the shoe bridges the curved surface.
        const float height=chassis==1?-.12f:chassis==3?-.26f:chassis==2?.30f:.26f;
        mounts[side]->transform.position={(side?1.f:-1.f)*(hover?.9f:.72f),height,1.15f};
        mounts[side]->transform.scale={mountScale,mountScale,mountScale};
    }
}
void PulseWeapon::present(const CombatState& combat,int rank) {
    installedRank=std::clamp(rank,0,3);kick=.045f*std::clamp(combat.beamTime/.14f,0.f,1.f);
    for(int side=0;side<2;++side) {
        const bool firing=combat.beamTime>0&&(combat.shots%2)==side;
        modules[side]->transform.position.z=firing?-kick:0;
        auto& flash=*flashes[side];flash.enabled=loaded()&&firing&&combat.beamTime>.07f;
        flash.transform.position={0,.18f,.64f-kick};
        flash.opacity=std::clamp(combat.beamTime/.14f,0.f,1.f);
        for(int level=0;level<3;++level) {
            indicators[side][level]->enabled=loaded()&&level<installedRank;
            indicators[side][level]->transform.position.z=-.26f+level*.09f-(firing?kick:0);
        }
    }
}
Vec3 PulseWeapon::muzzleLocal(int side) const {
    return transformPoint(mounts[side%2]->transform.matrix(),{0,.18f,.60f});
}
std::array<Vec3,16> PulseWeapon::boundsPoints() const {
    std::array<Vec3,16> result{};
    // Includes the top rank indicators; excludes the transient muzzle flash.
    for(int side=0;side<2;++side)for(int corner=0;corner<8;++corner)
        result[side*8+corner]=transformPoint(mounts[side]->transform.matrix(),
            {(corner&1)?.17f:-.17f,(corner&2)?.34f:-.085f,(corner&4)?.60f:-.60f});
    return result;
}
}
