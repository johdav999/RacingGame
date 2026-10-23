#pragma once
#include "engine/math.hpp"
#include <array>
namespace racing::dune {
struct Model {const char* name;const char* hash;unsigned triangles,materials;engine::Vec3 dimensions;};
inline constexpr std::array<Model,5> models{{
    {"dune-building-1","b94fc8a8dba2466cf8243c59a949eab836da6e963d3b06473058037f528693a0",8042,1,{16.000000f,5.676109f,12.352906f}},
    {"dune-building-2","d819525564dd5c71a242eca39eb8a96bb38a74962abff59889eaad1fe95bb8e1",8131,1,{16.000000f,7.839826f,13.317142f}},
    {"dune-building-3","97ca9485ee764050bb463c14d529adf6142907aa2d8e9a2cbc168552cdda8711",8508,1,{15.999999f,7.906364f,13.406160f}},
    {"dune-building-4","1782758b5f22ad782ba23e61325a0328ecff715f60fc41e26bf686c6cf2d62ff",8722,1,{16.000000f,6.955403f,12.406246f}},
    {"dune-building-5","3de1c33942d12ac6cee7a7a78f7a9ef791dd63c7bf0f1b2fdad251d632a2d601",8624,1,{16.000000f,10.941991f,12.289864f}},
}};
}
