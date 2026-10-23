#pragma once
#include "engine/material.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace racing::track_surface {
// Four metres per tile. These small, shared sources are built once, with no
// downloads or per-frame texture updates. Both renderers consume the albedo;
// WebGPU additionally uses the normal and metallic/roughness maps.
inline constexpr int size=512;
inline constexpr float tileMetres=4;

inline float noise(int x,int y,int period,std::uint32_t seed) {
    x=(x%period+period)%period;y=(y%period+period)%period;
    std::uint32_t h=std::uint32_t(x)*374761393u+std::uint32_t(y)*668265263u+seed;
    h=(h^(h>>13))*1274126177u;h^=h>>16;
    return float(h&0xffff)/65535.f;
}
inline float smoothNoise(float x,float y,int period,std::uint32_t seed) {
    int ix=int(std::floor(x)),iy=int(std::floor(y));
    float u=x-ix,v=y-iy;u=u*u*(3-2*u);v=v*v*(3-2*v);
    return std::lerp(std::lerp(noise(ix,iy,period,seed),noise(ix+1,iy,period,seed),u),
                     std::lerp(noise(ix,iy+1,period,seed),noise(ix+1,iy+1,period,seed),u),v);
}
inline std::shared_ptr<const engine::Material> makeMaterial(bool concrete) {
    using namespace engine;
    auto color=std::make_shared<Texture>(),normal=std::make_shared<Texture>(),rough=std::make_shared<Texture>();
    const std::string label=concrete?"Cast concrete shoulder":"Aggregate asphalt";
    for(auto texture:{color,normal,rough}) {
        texture->width=texture->height=size;texture->pixels.resize(size*size*4);
    }
    color->name=label+" / albedo";normal->name=label+" / normal";rough->name=label+" / roughness";
    std::vector<float> height(size*size);
    auto byte=[](float v){return static_cast<std::uint8_t>(std::clamp(std::round(v),0.f,255.f));};
    for(int y=0;y<size;++y)for(int x=0;x<size;++x) {
        const float fine=noise(x,y,size,17);
        const float grain=smoothNoise(x*.5f,y*.5f,size/2,83);
        const float cloud=smoothNoise(x/64.f,y/64.f,size/64,151);
        const float stain=smoothNoise(x/16.f,y/16.f,size/16,397);
        // Small pale aggregate in dark binder; broad, low-contrast discoloration
        // remains visible after semantic mip filtering removes the fine grain.
        float value=concrete?151+13*(cloud-.5f)+9*(grain-.5f)+6*(fine-.5f)
                            :91+23*(cloud-.5f)+12*(stain-.5f)+38*(grain-.5f)+13*(fine-.5f);
        float relief=concrete?.2f*grain+.08f*fine:.65f*grain+.15f*fine;
        float roughness=concrete?.88f:.9f-.09f*grain;
        if(concrete) {
            // A narrow recessed expansion joint every four metres along the
            // shoulder. Wrapped sampling keeps its normal continuous at tiles.
            float joint=std::min(y,size-y);
            float edge=1-std::clamp((joint-1)/2.f,0.f,1.f);
            value-=edge*53;relief-=edge*.5f;
            if(fine<.035f){value-=16;relief-=.1f;}
        } else if(grain>.66f) value+=14*(grain-.66f)/.34f;
        const auto index=(y*size+x)*4;
        color->pixels[index]=byte(value*1.025f);
        color->pixels[index+1]=byte(value);
        color->pixels[index+2]=byte(value*.965f);
        color->pixels[index+3]=255;
        rough->pixels[index]=255;rough->pixels[index+1]=byte(roughness*255);
        rough->pixels[index+2]=0;rough->pixels[index+3]=255;
        height[y*size+x]=relief;
    }
    auto h=[&](int x,int y){return height[((y+size)%size)*size+(x+size)%size];};
    for(int y=0;y<size;++y)for(int x=0;x<size;++x) {
        // OpenGL +Y tangent normal; Texture's rows/mesh UVs run top to bottom.
        auto n=normalized(Vec3{(h(x-1,y)-h(x+1,y))*.65f,(h(x,y+1)-h(x,y-1))*.65f,1});
        const auto index=(y*size+x)*4;
        normal->pixels[index]=byte((n.x*.5f+.5f)*255);
        normal->pixels[index+1]=byte((n.y*.5f+.5f)*255);
        normal->pixels[index+2]=byte((n.z*.5f+.5f)*255);normal->pixels[index+3]=255;
    }
    auto material=std::make_shared<Material>();material->name=label;
    material->baseColorTexture=color;material->normalTexture=normal;
    material->metallicRoughnessTexture=rough;material->roughness=1;material->metallic=0;
    material->validate();return material;
}
inline std::shared_ptr<const engine::Material> asphalt() {
    static const auto material=makeMaterial(false);return material;
}
inline std::shared_ptr<const engine::Material> concrete() {
    static const auto material=makeMaterial(true);return material;
}
} // namespace racing::track_surface
