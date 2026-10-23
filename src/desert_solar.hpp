#pragma once
#include "race_rules.hpp"
#include "desert_art_contract.hpp"
#include "engine/scene.hpp"
#include <span>
#include <vector>

namespace racing::desert {
struct Placement { unsigned model; Vec3 position; float yaw=0,scale=1; };
class Kit {
public:
    void plan(const Circuit&);
    void install(engine::Scene&,unsigned,std::span<const std::byte>);
    bool ready() const;
    float height(const Circuit&,float x,float z) const;
    void dress(engine::Scene&,const Circuit&) const;
    const auto& placements() const {return instances;}
private:
    std::array<bool,13> loaded{};
    std::vector<Placement> instances;
};
}
