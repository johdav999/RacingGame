#pragma once
#include "engine/math.hpp"
#include <array>
namespace racing::trackkit {
struct Model { const char* name; const char* hash; unsigned triangles; engine::Vec3 dimensions; };
inline const std::array<Model,13> models{{
{"kairox-barrier-straight","20d9cb95766eeeb59e6089a3d4044313c08ab6b91a76998f50840613aef6555e",1599,{4.000000f,1.200000f,0.700000f}},
{"kairox-barrier-corner","b5a08e4c357fa7662de66a49505a67a4311e8648ffd7981ae4c518e3daa67863",2000,{2.000000f,1.200000f,2.000000f}},
{"kairox-barrier-end","ec8c66a0eeb72b10b799b1695c3a4c9d47c3b1926a256bf236b949764eeed46f",1200,{0.800000f,1.200000f,0.700000f}},
{"kairox-finish-gantry","117e323f1a7a69b41701e328e745ab69fd6689515f1777c2971405e38856f5ad",5999,{35.000000f,8.000000f,1.400000f}},
{"kairox-checkpoint-gantry","494b0efeaceabab465907f00c39a8e3c64c04da20127eebc509a5e4d01d38023",4999,{35.000000f,8.000000f,1.000000f}},
{"kairox-streetlight","3cc477bb83cfe8f2e02e05d4fe0479c036a2599052a97622868d0d6d59ec1c6a",2200,{2.400000f,7.500000f,0.700000f}},
{"kairox-wall-lamp","fe574707e8e1ae5e89780b7f8eb04002faed78866eb2786d7dec126c850cd468",1200,{0.550000f,0.700000f,0.350000f}},
{"kairox-chevron-board","bd2938e9ff74f0d212b5d4146fbd5cff07bc3475672b1c7510272c520c2cbc18",1800,{2.400000f,2.400000f,0.600000f}},
{"kairox-braking-board","0be6646b8dbf876a42e4735a3057b547baee674d7c15d79e255060e2e7bf2252",1598,{0.850000f,2.300000f,0.550000f}},
{"kairox-sidewalk-pavement","d54adbf1cc92099abba26e43ceb55464ce5b7e05e003bf94a837c7eedbe9b092",1599,{4.000000f,0.160000f,2.000000f}},
{"kairox-sidewalk-curb","59500b0db0cc7b4bcc653c4db6ccd6fac0208b3eaeca310b30ce5f445ca1045f",1200,{4.000000f,0.280000f,0.300000f}},
{"kairox-sidewalk-corner","317d8356162a387f56471e2503a142cbbb3799b3998b3e1542e5d7f4917d29ea",1799,{2.000000f,0.160000f,2.000000f}},
{"kairox-storm-drain","89d357dea406d777de9f35a966e2ac222bbc76a78705155644a97fd9edb7e582",1499,{0.750000f,0.070000f,0.600000f}},
}};
}
