#pragma once
#include "race_rules.hpp"
#include "desert_race_contract.hpp"
#include "engine/scene.hpp"
#include <span>
#include <vector>
namespace racing::desertrace {
struct Placement {unsigned model;Vec3 position;float yaw;Vec3 scale;int gate=-1;};
class Kit {
public:
    void plan(const Circuit&);
    void install(engine::Scene&,unsigned,std::span<const std::byte>,bool emissiveMaps=true);
    bool ready() const {return loaded[0]&&loaded[1];}
    float markerMountHeight() const {return beamTop;}
    const auto& placements() const {return instances;}
private:
    std::array<bool,2> loaded{};
    float beamTop=0;
    std::vector<Placement> instances;
};
}
