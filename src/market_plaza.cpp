#include "market_plaza.hpp"
#include "static_art.hpp"
#include "engine/mesh_component.hpp"
#include "engine/glb_importer.hpp"
#include <algorithm>

namespace racing::plaza {
using namespace engine;
Vec3 surfacePoint(const Circuit& track,float sample,float radial,float height) {
    int i=int(std::floor(sample));float t=sample-i;
    auto p=track.point(i)*(1-t)+track.point(i+1)*t;
    auto right=normalized(track.right(i)*(1-t)+track.right(i+1)*t);
    p=p+right*radial;p.y=height;return p;
}
namespace {
void addFoundation(Actor& actor,const Circuit& track) {
    auto mesh=std::make_shared<Mesh>();
    for(auto [begin,end]:sectors)for(int s=begin;s<end;++s) {
        unsigned n=mesh->vertices.size();
        for(auto [sample,r]:std::array<std::pair<float,float>,4>{{{float(s),inner},{float(s),outer},{float(s+1),inner},{float(s+1),outer}}})
            mesh->vertices.push_back({surfacePoint(track,sample,r,surfaceHeight-.085f),{0,1,0},{r,float(s)}});
        mesh->indices.insert(mesh->indices.end(),{n,n+2,n+1,n+1,n+2,n+3});
    }
    mesh->generateTangents();
    auto& base=actor.addComponent<MeshComponent>("Recessed paving foundation",mesh);
    base.color={.22f,.23f,.24f};MaterialInstance material;material.roughness=1; base.setMaterial(0,material);
}
std::shared_ptr<const Mesh> conform(const Mesh& source,const Circuit& track,int sample,float radial,float width,float height) {
    auto result=std::make_shared<Mesh>();result->materials=source.materials;result->parts=source.parts;
    result->vertices=source.vertices;result->indices=source.indices;
    auto bounds=source.localBounds();auto dimensions=bounds.maximum-bounds.minimum;
    for(auto& v:result->vertices) {
        float along=(v.position.z-bounds.minimum.z)/dimensions.z;
        float across=(v.position.x-bounds.minimum.x)/dimensions.x;
        float s=sample+along*2, r=radial+across*width;
        auto tangent=normalized(surfacePoint(track,s+.01f,r)-surfacePoint(track,s-.01f,r));
        Vec3 right{tangent.z,0,-tangent.x};
        v.position=surfacePoint(track,s,r,v.position.y+height);
        v.normal=normalized(right*v.normal.x+Vec3{0,v.normal.y,0}+tangent*v.normal.z);
    }
    result->generateTangents();return art::prepareStatic(*result);
}
std::shared_ptr<const Texture> dampMask() {
    auto image=std::make_shared<Texture>();image->name="Damp edge fade";image->width=image->height=64;
    image->pixels.resize(64*64*4);
    for(int y=0;y<64;++y)for(int x=0;x<64;++x) {
        float r=std::hypot((x-31.5f)/31.5f,(y-31.5f)/31.5f);
        auto i=(y*64+x)*4;image->pixels[i]=image->pixels[i+1]=image->pixels[i+2]=255;
        image->pixels[i+3]=std::uint8_t(255*std::clamp((1-r)*6.f,0.f,1.f));
    }return image;
}
void addDamp(Actor& actor,const Circuit& track) {
    auto material=std::make_shared<Material>();material->name="Small residual damp patches";
    material->shader=MaterialShader::Water;material->baseColor={.09f,.10f,.12f};
    material->baseColorTexture=dampMask();material->opacity=.62f;material->alphaMode=AlphaMode::Blend;
    material->roughness=.28f;material->water.amplitude=0;material->water.speed=0;
    material->water.rippleStrength=.025f;material->water.reflectionStrength=1;
    // Tiny damp patches use the existing environment reflection. Capturing the
    // whole city for these six accents costs more than the visible race scene.
    material->water.planarReflectionStrength=0;material->water.refractionStrength=0;
    material->water.horizon={.07f,.06f,.1f};material->water.zenith={.015f,.02f,.04f};
    material->validate();
    auto blob=std::make_shared<Mesh>();blob->vertices.push_back({{0,0,0},{0,1,0},{.5f,.5f}});
    constexpr int n=32;
    for(int i=0;i<n;++i) {
        float a=2*pi*i/n,r=1+.09f*std::sin(a*3)+.05f*std::cos(a*7);
        blob->vertices.push_back({{std::cos(a)*r,0,std::sin(a)*r},{0,1,0},{.5f+.5f*std::cos(a),.5f+.5f*std::sin(a)}});
        blob->indices.insert(blob->indices.end(),{0u,unsigned((i+1)%n+1),unsigned(i+1)});
    }
    blob->generateTangents();
    for(auto [begin,end]:sectors)for(int i=0;i<3;++i) {
        auto& patch=actor.addComponent<MeshComponent>("Residual rain / neon reflection",blob);
        patch.transform.position=surfacePoint(track,begin+5.f+i*7.f,16.f+(i==1?2.f:0.f),surfaceHeight+.006f);
        auto t=track.tangent(begin+5+i*7);patch.transform.rotation.y=std::atan2(t.x,t.z);
        patch.transform.scale={i==1?1.5f:.9f,1,2.1f+i*.25f};
        patch.setMaterial(0,MaterialInstance(material));patch.castsShadow=false;
    }
}
}
void Kit::install(Scene& scene,const Circuit& track,unsigned id,std::span<const std::byte> bytes) {
    if(id>=loaded.size()||loaded[id])throw std::logic_error("Invalid or installed plaza module");
    GlbImportOptions options;options.maxTextureBytes=16*1024*1024;
    auto source=GlbImporter::loadMemory(bytes,options);const auto& contract=models[id];
    auto bounds=source->localBounds();
    if(!source->warnings.empty()||source->parts.size()!=1||source->materials.size()!=1||
       source->indices.size()!=contract.triangles*3||!bounds.valid||
       magnitude(bounds.maximum-bounds.minimum-contract.dimensions)>.02f)
        throw std::runtime_error("Plaza model contract mismatch");
    auto actor=std::make_unique<Actor>(id==0?"Paved market plazas":"Market plaza drainage channels");
    // Separate compact sectors preserve culling; all modules share source maps.
    for(auto [begin,end]:sectors)for(int sample=begin;sample<end;sample+=2) {
        int rows=id==0?6:1;
        for(int row=0;row<rows;++row) {
            auto mesh=conform(*source,track,sample,id==0?inner+row*4.f:inner-.45f,id==0?4.f:.45f,surfaceHeight-(id==0?.12f:.15f));
            auto& visual=actor->addComponent<MeshComponent>(id==0?"Dry stone paving":"Continuous grated drain",mesh);
            visual.color={1,1,1};visual.allowInstanceReordering=true;
            MaterialInstance material;material.roughness=id==0?.92f:.65f;material.metallic=id==0?0.f:.3f;
            // Low-contrast module variation remains matte rather than a wet checkerboard.
            if(id==0){float shade=.96f+.025f*((sample+row*3)%4);material.tint={shade,shade,shade};}
            visual.setMaterial(0,material);
        }
    }
    if(id==0){addFoundation(*actor,track);addDamp(*actor,track);}
    scene.adoptChild(std::move(actor));loaded[id]=true;scene.synchronize();
}
}
