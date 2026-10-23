#pragma once
#include "engine/math.hpp"
#include <array>
namespace racing::desert {
struct Model {const char* name;const char* hash;unsigned triangles,materials;engine::Vec3 dimensions;};
inline constexpr std::array<Model,13> models{{
    {"solar-mirror-intact","c22f1703d6c4a1bb08f25231a829844abc74f00c22b228662c6d94e911a9acd7",3419,2,{6.000000f,4.783790f,3.380512f}},
    {"solar-mirror-broken","66b0f40202178283fb1195a65eff3744d4a33f7948775af97ed1d3a72b0d7cad",3391,1,{6.000000f,5.704457f,5.964912f}},
    {"solar-receiver-tower","314d6b15e7d51749855e8e5780c836343f1ed5429359c5a76698bf4a57784e5e",14884,1,{23.595778f,55.000000f,21.651567f}},
    {"sandstone-cliff-wall","a932b1a35b754f36903486b2d03b8a37685df8c0e37cbacd2484c781cb18b1cf",9568,1,{20.000000f,9.494021f,9.058528f}},
    {"sandstone-cliff-corner","60f14e247a8663e9fd3c9db591bbc3feec1d13afdb15c3aa12455fdb8c92b92f",9864,1,{11.999999f,6.564801f,9.707385f}},
    {"sandstone-cliff-buttress","f4c594b3e4ec550c90f25f9634b8802bfeba35313d94b4d072c2282889fef7b0",9658,1,{11.280159f,14.000000f,7.913760f}},
    {"sandstone-cliff-low-shelf","0e015bbeb00a406fa8910990dacf67ad2f9f83a43b382aa7e2e95e410edb94fb",7848,1,{15.999999f,4.544536f,5.908259f}},
    {"sandstone-rock-arch","d9d18b5cb867ca4cedf7455da97bf588200614cc1720d1796d2bdaaeaee4d111",15619,1,{24.000000f,13.312682f,9.791627f}},
    {"energy-conduit-straight","66e85e50eb513fb713ec310d43d313cf52262cd653f8670dabb76422c048e669",2526,1,{4.000000f,0.798003f,0.791954f}},
    {"energy-conduit-elbow","9b29a31c61cec26c6e7d86facbdce67fad1342498a7a8a3fef84aa56a99b2113",2781,1,{3.364620f,0.900000f,3.705886f}},
    {"energy-conduit-junction","b10d09ab02bbd05ba03b6252dd2f976f73ca6f1f295d15579c89e967f2654e8b",3149,1,{3.992435f,0.900000f,3.967632f}},
    {"energy-conduit-support","b0ce908460a139f16c4ac76a49f49e6fa5bb3817ab316bee2540280b6c831683",2147,1,{0.870096f,1.200000f,1.243484f}},
    {"solar-operations","bd7676a78d5cf66fee974c4f39f2b40883babbeb89353c9261e57a5e497e96c2",16571,1,{11.000000f,5.304031f,7.478122f}},
}};
}
