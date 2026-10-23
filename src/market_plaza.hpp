#pragma once
#include "race_rules.hpp"
#include "plaza_art_contract.hpp"
#include "engine/scene.hpp"
#include <span>

namespace racing::plaza {
// The existing sidewalk ends at 13.95 m and rises to 0.185 m.
inline constexpr float surfaceHeight=.215f, inner=14.5f, outer=38.5f;
inline constexpr std::array<std::pair<int,int>,2> sectors{{{0,26},{92,118}}};
inline bool contains(const Circuit& track,Vec3 point) {
    auto nearest=track.nearest(point);
    bool sector=false;for(auto [begin,end]:sectors)sector|=nearest.segment>=begin&&nearest.segment<end;
    return sector&&engine::dot(point-nearest.position,track.right(nearest.segment))>=inner&&nearest.distance<=outer;
}
engine::Vec3 surfacePoint(const Circuit&,float sample,float radial,float height=surfaceHeight);
class Kit {
public:
    void install(engine::Scene&,const Circuit&,unsigned,std::span<const std::byte>);
    bool ready() const {return loaded[0]&&loaded[1];}
    static constexpr unsigned pavingModules=156,drainModules=26,dampPatches=6;
private:
    std::array<bool,2> loaded{};
};
}
