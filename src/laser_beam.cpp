#include "laser_beam.hpp"
#include <cmath>

namespace racing {
using namespace engine;
namespace {
float magnitude(Vec3 v) {return std::sqrt(dot(v,v));}
Vec3 facing(Vec3 d) {return {-std::asin(std::clamp(d.y,-1.f,1.f)),std::atan2(d.x,d.z),0};}
std::shared_ptr<const Mesh> shaftMesh() {
    static const auto shared=[] {
        auto m=std::make_shared<Mesh>();
        // Unit-length capped cylinder, with a short taper at both apertures.
        for(int ring=0;ring<4;++ring)for(int i=0;i<=12;++i) {
            float a=2*pi*i/12,r=(ring==0||ring==3)?.12f:.5f;
            float z=ring==0?0: ring==1?.006f:ring==2?.994f:1;
            m->vertices.push_back({{r*std::cos(a),r*std::sin(a),z},{std::cos(a),std::sin(a),0},{float(i)/12,z}});
            if(ring<3&&i<12){auto n=static_cast<unsigned>(ring*13+i);m->indices.insert(m->indices.end(),{n,n+1,n+13,n+1,n+14,n+13});}
        }
        for(unsigned i=1;i<11;++i){m->indices.insert(m->indices.end(),{0,i+1,i,39,39+i,40+i});}
        m->generateTangents();return m;
    }();return shared;
}
std::shared_ptr<const Mesh> haloMesh() {
    static const auto shared=[] {
        auto t=std::make_shared<Texture>();t->name="Laser feathered glow";
        t->width=64;t->height=32;t->wrapU=t->wrapV=TextureWrap::Clamp;t->pixels.resize(64*32*4);
        for(unsigned y=0;y<32;++y)for(unsigned x=0;x<64;++x) {
            float across=std::abs((x+.5f)/32-1),along=(y+.5f)/32;
            float alpha=std::pow(1-across,3)*std::min(1.f,std::min(along,1-along)*16);
            auto i=(y*64+x)*4;for(int c=0;c<3;++c)t->pixels[i+c]=255;
            t->pixels[i+3]=static_cast<std::uint8_t>(alpha*255);
        }
        auto m=std::make_shared<Mesh>();
        m->vertices={{{-.5f,0,0},{0,1,0},{0,0}},{{.5f,0,0},{0,1,0},{1,0}},{{-.5f,0,1},{0,1,0},{0,1}},{{.5f,0,1},{0,1,0},{1,1}}};
        m->indices={0,2,1,1,2,3};
        auto material=std::make_shared<Material>();material->shader=MaterialShader::Unlit;
        material->alphaMode=AlphaMode::Additive;material->doubleSided=true;material->baseColorTexture=t;
        m->materials.push_back(material);m->generateTangents();return m;
    }();return shared;
}
void glow(MeshComponent& m,Vec3 color) {
    m.color=color;m.blend=BlendMode::Additive;m.castsShadow=m.receivesShadow=m.receivesDecals=false;
    MaterialInstance material;material.shader=MaterialShader::Unlit;m.setMaterial(0,material);
}
}
LaserBeam::LaserBeam():Component("Laser pulse") {
    shaft=&addChild<Component>("Laser axis");
    core=&shaft->addChild<MeshComponent>("Laser white core",shaftMesh());glow(*core,{.7f,.95f,1});
    halo=&shaft->addChild<MeshComponent>("Laser cyan glow",haloMesh());
    halo->color={.04f,.65f,1};halo->blend=BlendMode::Additive;
    halo->castsShadow=halo->receivesShadow=halo->receivesDecals=false;
    static const auto sphere=Mesh::sphere(12,6);
    contact=&addChild<MeshComponent>("Laser contact flare",sphere);glow(*contact,{.4f,.85f,1});
    for(auto& spark:sparks){spark=&addChild<MeshComponent>("Laser impact spark",shaftMesh());glow(*spark,{1,.45f,.08f});}
    reset();
}
void LaserBeam::reset(){enabled=false;distance=0;impactKind=LaserImpact::None;start=end={};}
void LaserBeam::fire(Vec3 from,Vec3 to,Vec3 normal,LaserImpact impact,const Mat4& carPose) {
    reset();
    if(!std::isfinite(from.x+from.y+from.z+to.x+to.y+to.z+normal.x+normal.y+normal.z))return;
    distance=magnitude(to-from);if(distance<.001f){distance=0;return;}
    start=from;end=to;impactKind=impact;hitNormal=magnitude(normal)>.001f?normalized(normal):normalized(from-to);
    const auto local=inverseAffine(carPose);
    localStart=transformPoint(local,start);localEnd=transformPoint(local,end);
    localNormal=transformDirection(local,hitNormal);
}
void LaserBeam::present(float remaining,Vec3 eye,const Mat4& carPose) {
    enabled=distance>0&&remaining>0;if(!enabled)return;
    // Follow the same interpolated chassis pose as the rendered cannon. Rebuild
    // from the original local segment so repeated frames cannot accumulate drift.
    start=transformPoint(carPose,localStart);end=transformPoint(carPose,localEnd);
    hitNormal=normalized(transformDirection(carPose,localNormal));
    distance=magnitude(end-start);
    shaft->transform.position=start;shaft->transform.rotation=facing(normalized(end-start));
    float strength=std::clamp(remaining/CombatState::beamDuration,0.f,1.f),age=1-strength;
    core->transform.scale={.045f+.025f*strength,.045f+.025f*strength,distance};core->opacity=std::sqrt(strength);
    halo->transform.scale={.5f+.25f*age,1,distance};halo->opacity=.8f*strength;
    // Roll a feathered ribbon about the beam's local Z, facing the current eye.
    auto local=transformPoint(inverseAffine(shaft->worldMatrix()),eye);
    halo->transform.rotation.z=std::atan2(-local.x,local.y);
    contact->enabled=impactKind!=LaserImpact::None;
    contact->transform.position=end+hitNormal*.035f;
    float radius=.18f+age*.45f;contact->transform.scale={radius,radius,radius};contact->opacity=strength;
    contact->color=impactKind==LaserImpact::Shield?Vec3{.1f,.8f,1}:Vec3{1,.65f,.2f};
    auto tangent=normalized(cross(hitNormal,std::abs(hitNormal.y)<.9f?Vec3{0,1,0}:Vec3{1,0,0}));
    auto bitangent=cross(hitNormal,tangent);
    for(unsigned i=0;i<sparks.size();++i) {
        auto& spark=*sparks[i];spark.enabled=impactKind==LaserImpact::Surface||impactKind==LaserImpact::Vehicle;
        float a=2*pi*i/sparks.size();auto direction=normalized(hitNormal*.7f+tangent*std::cos(a)+bitangent*std::sin(a));
        spark.transform.position=end+hitNormal*.04f+direction*(age*.65f);
        spark.transform.rotation=facing(direction);spark.transform.scale={.025f,.025f,.12f+.18f*age};spark.opacity=strength*strength;
    }
}
}
