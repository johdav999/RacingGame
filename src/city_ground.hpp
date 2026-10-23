#pragma once
#include "race_rules.hpp"
#include "building_art_contract.hpp"
#include "city_kit_contract.hpp"
#include "street_kit.hpp"
#include "track_kit.hpp"
#include "engine/scene.hpp"
#include <span>

namespace racing::cityground {
struct View { Vec3 eye,target; };
struct Parked {Vec3 position;float yaw=0;};
struct Report {
    unsigned promenadeSegments=0,drainSegments=0,foundations=0,loadingDocks=0;
    unsigned utilityClusters=0,serviceVehicles=0,contactShadows=0,lightPools=0,dampPatches=0;
    unsigned meshes=0,triangles=0;
    std::array<View,5> views{};
    std::array<Parked,3> parked{};
};
// One immutable scenic assembly, owned by the scene. No colliders, callbacks,
// file IO or per-frame rebuilds; track/physics and imported assets stay unchanged.
Report build(engine::Scene&,const Circuit&,std::span<const buildingart::Placement>,
    std::span<const citykit::Placement>,std::span<const streetkit::Placement>,
    std::span<const trackkit::Placement>);
// Called once after the existing Bastion import passes its game asset contract.
void installParked(engine::Scene&,const Report&,const engine::Mesh&);
}
