#pragma once
#include "race_rules.hpp"
#include "building_art_contract.hpp"
#include "track_art_contract.hpp"
#include "engine/scene.hpp"
#include "engine/light_component.hpp"
#include <span>
#include <vector>

namespace racing::trackkit {
struct Placement {
    unsigned model;
    Vec3 position;
    float yaw=0;
    Vec3 scale{1,1,1};
    int marking=0; // Turn direction, or braking distance in metres.
};
class Kit {
public:
    void plan(const Circuit&,const std::vector<buildingart::Placement>&);
    void install(engine::Scene&,unsigned,std::span<const std::byte>);
    void illuminate(engine::Scene&,Vec3 viewer);
    bool ready() const;
    float markerMountHeight(int gate) const {return beamTops[gate==0?0:1];}
    const auto& placements() const {return instances;}
private:
    std::array<bool,13> loaded{};
    std::array<float,2> beamTops{};
    std::vector<Placement> instances;
    std::vector<Vec3> emitters;
    std::array<engine::LightComponent*,6> lights{};
};
}
