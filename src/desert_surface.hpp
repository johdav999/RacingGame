#pragma once
#include "track_surface.hpp"

namespace racing::desert {
// Small shared, seamless material sources. Albedo relief remains readable on
// the fallback renderer; the PBR renderer also resolves grain normals/roughness.
inline std::shared_ptr<const engine::Material> surfaceMaterial(bool stone) {
    using namespace engine;constexpr int n=256;
    auto color=std::make_shared<Texture>(),normal=std::make_shared<Texture>(),rough=std::make_shared<Texture>();
    for(auto t:{color,normal,rough}){t->width=t->height=n;t->pixels.resize(n*n*4);}
    color->name=stone?"Layered sandstone grains":"Wind-worked sand and gravel";
    normal->name=color->name+" normals";rough->name=color->name+" roughness";
    std::vector<float> h(n*n);
    auto byte=[](float v){return std::uint8_t(std::clamp(v,0.f,255.f));};
    for(int y=0;y<n;++y)for(int x=0;x<n;++x){
        float grain=track_surface::noise(x,y,n,141);
        float cloud=track_surface::smoothNoise(x/32.f,y/32.f,8,41);
        float pore=track_surface::smoothNoise(x/4.f,y/4.f,64,571);
        float wave=std::sin(2*pi*(y*19.f/n+.33f*std::sin(2*pi*x/n)+.16f*std::sin(6*pi*x/n)));
        float vein=std::pow(std::max(0.f,std::sin(2*pi*(y*5.f/n+.2f*std::sin(2*pi*x/n)))),12);
        float shade=stone?.84f+.23f*cloud+.18f*pore-.24f*vein:.87f+.20f*cloud+.14f*grain+.035f*wave;
        if(grain>.96f)shade+=stone?.05f:.16f;
        auto i=(y*n+x)*4;
        color->pixels[i]=byte((stone?178:197)*shade);color->pixels[i+1]=byte((stone?132:126)*shade);
        color->pixels[i+2]=byte((stone?84:65)*shade);color->pixels[i+3]=255;
        h[y*n+x]=stone?.8f*pore-.32f*vein:.10f*wave+.12f*grain;
        rough->pixels[i]=255;rough->pixels[i+1]=byte(225+25*grain);rough->pixels[i+2]=0;rough->pixels[i+3]=255;
    }
    auto sample=[&](int x,int y){return h[((y+n)%n)*n+(x+n)%n];};
    for(int y=0;y<n;++y)for(int x=0;x<n;++x){
        auto v=normalized(Vec3{(sample(x-1,y)-sample(x+1,y))*1.3f,(sample(x,y-1)-sample(x,y+1))*1.3f,1});
        auto i=(y*n+x)*4;normal->pixels[i]=byte(127.5f*(v.x+1));normal->pixels[i+1]=byte(127.5f*(v.y+1));normal->pixels[i+2]=byte(127.5f*(v.z+1));normal->pixels[i+3]=255;
    }
    auto material=std::make_shared<Material>();material->name=color->name;
    material->baseColorTexture=color;material->normalTexture=normal;material->metallicRoughnessTexture=rough;
    material->roughness=.97f;material->metallic=0;
    return material;
}
}
