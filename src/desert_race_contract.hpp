#pragma once
#include "engine/math.hpp"
#include <array>
namespace racing::desertrace {
struct Model {const char* name;const char* hash;unsigned triangles,materials;engine::Vec3 dimensions;};
inline constexpr std::array<Model,2> models{{
    {"desert-race-divider","10a95cefb1bd42b7f5e18f83c659e87b54a249160863b4ee78202134ac15a701",3480,1,{4.015475f,1.201527f,0.656534f}},
    {"desert-race-gate","a3aae3762fd6326b326e1b9d3358b9ebf2c2edb8e46d0f7ca496ed4b25c9dfdb",11980,1,{27.999456f,7.225799f,4.490505f}},
}};
}
