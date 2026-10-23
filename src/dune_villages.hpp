#pragma once
#include "race_rules.hpp"
#include "dune_village_contract.hpp"
#include "engine/scene.hpp"
#include <span>
#include <vector>

namespace racing::dune {
// Shared by terrain grading, placements and scenic review cameras.
inline constexpr std::array<int,2> villageSamples{10,145};
inline Vec3 center(const Circuit& track,unsigned village) {
    int sample=villageSamples.at(village);
    return track.point(sample)+track.right(sample)*43.f;
}
inline float groundBlend(const Circuit& track,float x,float z) {
    float blend=0;
    for(unsigned village=0;village<villageSamples.size();++village) {
        int sample=villageSamples[village];auto delta=Vec3{x,0,z}-center(track,village);
        float edge=std::max(std::abs(engine::dot(delta,track.tangent(sample)))-37.f,
                            std::abs(engine::dot(delta,track.right(sample)))-25.f);
        float t=std::clamp(1-edge/12.f,0.f,1.f);
        blend=std::max(blend,t*t*(3-2*t));
    }
    return blend;
}
struct Placement {unsigned model,village;Vec3 position;float yaw;};
class Kit {
public:
    void plan(const Circuit&);
    void install(engine::Scene&,unsigned,std::span<const std::byte>);
    bool ready() const {return std::all_of(loaded.begin(),loaded.end(),[](bool v){return v;});}
    const auto& placements() const {return instances;}
private:
    std::array<bool,5> loaded{};
    std::vector<Placement> instances;
};
}
