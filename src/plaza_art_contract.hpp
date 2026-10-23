#pragma once
#include "engine/math.hpp"
#include <array>
namespace racing::plaza {
struct Model {const char* name;const char* hash;unsigned triangles;engine::Vec3 dimensions;};
inline const std::array<Model,2> models{{
{"kairox-plaza-paving","34c8b983e44f34c0e30032ddc7f55551644dc6b2f75938b5931e3db052553688",3500,{4.000000f,0.120000f,4.000000f}},
{"kairox-plaza-drain","ce50457f88e7a0b992dc75b6b2c529b083d40df388614f2808d5b6b8421a307d",5999,{0.450000f,0.150000f,4.000000f}},
}};
}
