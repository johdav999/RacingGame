#pragma once
#include "race_rules.hpp"
#include "gate_marker_contract.hpp"
#include "engine/scene.hpp"
#include "engine/mesh_component.hpp"

namespace racing {
// One non-colliding target, driven exclusively by accepted race progress.
class GateMarker {
public:
    void install(engine::Scene&,std::span<const std::byte>);
    void present(const Circuit&,const RaceProgress&,bool racing,float beamTop);
    float mountHeight() const;
    bool ready() const {return root!=nullptr;}
    const engine::Actor* actor() const {return root;}
private:
    engine::Actor* root=nullptr;
    engine::MeshComponent* sign=nullptr;
};
}
