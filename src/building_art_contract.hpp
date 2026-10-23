#pragma once
#include "engine/math.hpp"
#include <array>
namespace racing::buildingart {
struct Model { const char* name; const char* hash; unsigned triangles; engine::Vec3 dimensions; };
inline const std::array<Model,7> models{{
{"kairox-market-block","b88571456c62e5d6c1a2903c43f2a3cf15d7ddaabaf2aa8c1401b13b3df99a3a",38157,{16.316992f,24.000000f,19.906235f}},
{"kairox-service-depot","b86cb9332956ed03aa57099625ef4dc65fd2b8dbe90f8b189c873a33691dfcab",41632,{18.814377f,10.000000f,22.993412f}},
{"kairox-transit-tower","78f18b3f1e8b63b7292edd430c86232a425717b4315e7895ab2daa3ce792c948",40455,{16.277721f,32.000000f,15.884677f}},
{"kairox-shop-arcade","a1fdcd534b663c2d08ba249b620d317984e49067e771d91588171a910e137e12",41822,{19.302113f,12.000000f,30.752077f}},
{"kairox-player-garage","ba45adad7cf28210170db47757bea72cf61dbf2b8cd3e0b83d8da4f08feddd8d",42187,{13.274937f,14.000000f,18.264563f}},
{"kairox-corner-apartments","35cc87205138196abd1ac738d65e7728836a3a03091649857921160206b78c75",41601,{15.182295f,30.000000f,15.958517f}},
{"kairox-loading-warehouse","b0123f689bd0ebf5fd49b6a1ab6907df4016135d05f63ef29fb9c1c34529b20d",41314,{26.607344f,11.000000f,30.541584f}},
}};
struct Placement { unsigned model; engine::Vec3 position; float yaw; float radius; };
}
