#pragma once
#include <array>
namespace racing::sound {
struct Asset {const char* name;const char* hash;};
inline constexpr std::array<Asset,13> assets{{
    {"vektor-engine","365cc5dc4f8b8600cf743e2237883ce30f76137d49d103a3a61b1de45f8359a8"},
    {"skimmer-engine","5890f9302bdefc1142b029cbf799c62e3dee69b8dd4efc2465487847df596ae2"},
    {"bastion-engine","e5cbd2f86ed8a69f5e425bc27570fea7c71cb29de0c1da550f40d8c08f1269d9"},
    {"vector-engine","af1d9d74d92b29b88169b5cdf2a21271971dafe012d8822b9fc227e1ac8e1337"},
    {"tire-slide","77d8a6150370c51aeba5160b82fe30a10bb6bb4c8301bc9f0ac6fb946d62fd07"},
    {"reactor-boost","25f42548ad37ce0b6efd632c277112be363c581b7e98aa0e5de1d6af5371dd1b"},
    {"laser-fire","cf220eb7f8c04adbebfbb973049606aa8cde1eb5182c8e43fba35441c15295b3"},
    {"shield-on","59cc86862789438303ccae4af457dec0c992f56e7603e15f178a69edf46fcaeb"},
    {"shield-impact","35d0071bc61b38f2f9ef88f0c8e2530713904674b94cf7bd963fcacf176f8590"},
    {"hull-impact","3392c683d78c53c0e3dfa2a02bde085d25ac1f23dfee52943b889c2d7ce50bf7"},
    {"countdown-tick","ef2aace14d6b93d9ba1d7faa9e865bc2a7419598b71f1c0ea2661a76f744bd87"},
    {"race-go","f526134c1d1bc337192004b4555306041d63312ac549180d1f642fc381f7abd8"},
    {"race-finish","2caa25c26e9c27899c5e4fb4f6807d2b26ac3a91a4aa88ba58c0088ca93748d7"},
}};
}
