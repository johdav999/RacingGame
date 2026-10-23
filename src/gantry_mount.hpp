#pragma once
#include "engine/mesh.hpp"
#include <stdexcept>

namespace racing {
// Measure the beam under the sign, excluding taller outboard column caps.
// Source vertices are used before LOD preparation and include the placed scale.
inline float gantryBeamTop(const engine::Mesh& mesh,float heightScale=1.f) {
    float top=0;
    for(const auto& v:mesh.vertices)
        if(std::abs(v.position.x)<=2.8f)top=std::max(top,v.position.y*heightScale);
    if(!std::isfinite(top)||top<=0)throw std::runtime_error("Gantry has no central mounting beam");
    return top;
}
}
