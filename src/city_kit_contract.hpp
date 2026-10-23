#pragma once
#include "engine/math.hpp"
#include <array>
namespace racing::citykit {
struct Model { const char* name; const char* hash; unsigned triangles,parts,materials; engine::Vec3 dimensions; };
inline const std::array<Model,14> models{{
{"kairox-market-stall","0b94cef72b6cc72debc1de45a02f2d107f1cf0e0928b6c54794e9eeb46edf68d",41643,1,1,{3.999929f,3.000000f,5.157598f}},
{"kairox-closed-kiosk","f4ee710f41ec89ea2082c23ee4f8033c8b215927cf0984f9e86d4fd76e8514e1",43759,1,1,{3.571875f,3.200000f,4.000268f}},
{"kairox-dumpster","ebfef1d96da79f0fda86830fd8bea3e74ca62912c6cca864ce7f141148807b93",43580,1,1,{1.361415f,1.500000f,2.518663f}},
{"kairox-crate-stack","572389dc160c9a70ff9c172b85861b99d01cc059c167822a2b4060c78be18f18",45573,1,1,{1.376760f,1.500000f,2.558415f}},
{"kairox-bollard","19c137c386ddd572763d45a52e3d0d91454960d337fa2b6fb3a8cbfb9578998d",43553,1,1,{0.431257f,1.100000f,0.432727f}},
{"kairox-ventilation-unit","688d4a7cf6907d1ce1792fd80a7e8eee78ed7d7cf248e533ed9cff96c29d95b4",39491,1,1,{1.959512f,0.850000f,2.416276f}},
{"kairox-tower-stepped","691b2b8489ec9b1bc5fd90ee418dc300ede95cee3067b092137e1220dbf91c83",44126,1,1,{22.403429f,65.000000f,38.531170f}},
{"kairox-tower-crowned","b3c6a067e5f3d6b4364426c86fe4810a79cb2f870b2a3588b3231653d55054ca",42249,1,1,{28.340546f,80.000000f,31.106728f}},
{"kairox-staircase","44e24bd06057149c0fa326abdcfbe0a1755673ab7d162f5a9f983c149a25870b",42012,1,1,{10.057355f,5.000000f,3.999491f}},
{"kairox-catwalk","12683e9b690acec8b85b4339ed1a1e8ef68a345a065e16572e1ce194401aab49",41896,1,1,{2.763105f,1.481317f,8.000000f}},
{"kairox-rooftop-equipment","1dcdb2439da7c38840363364d0d718bcfa41b1ffbd41f0bb0cc1bdc010da3f60",43973,1,1,{2.465731f,2.500000f,4.887935f}},
{"kairox-utility-room","f26f1083bc31db86d27d59447e27478f87a721b1a2387aee6d39da4dc00642ad",16539,1,1,{6.000000f,4.000000f,10.000000f}},
{"kairox-residential-midrise","3d2729e9ed2d4158042b8c00bcab1309177e4c222bcc2bd8ca88cd072b0d089f",17836,1,1,{21.261276f,36.000000f,22.454659f}},
{"kairox-office-midrise","af2e5214b4c015909c6de5071c8ab5581a4e230770aff5fc2d7a63bbd480f42e",16368,1,1,{18.129238f,48.000000f,24.010696f}},
}};
struct Placement { unsigned model; engine::Vec3 position; float yaw; float radius; float scale=1.f; };
}
