#pragma once
#include "race_rules.hpp"
#include "building_art_contract.hpp"
#include "street_art_contract.hpp"
#include "engine/scene.hpp"
#include <span>
#include <vector>

namespace racing::streetkit {
struct Placement {
    unsigned model;
    Vec3 position;
    float yaw=0;
    Vec3 scale{1,1,1};
    bool collision=true;
};
class Kit {
public:
    void plan(const Circuit&,const std::vector<buildingart::Placement>&);
    void install(engine::Scene&,unsigned,std::span<const std::byte>);
    bool ready() const;
    bool overlapsSkyline(Vec3,float radius) const;
    const std::vector<Placement>& placements() const {return instances;}
private:
    std::array<bool,12> loaded{};
    std::vector<Placement> instances;
};
}
