#include "city_ground.hpp"
#include "market_plaza.hpp"
#include "track_surface.hpp"
#include "static_art.hpp"
#include "engine/mesh_component.hpp"
#include <map>

namespace racing::cityground {
using namespace engine;
namespace {
Vec3 rotate(Vec3 p,float yaw) { float c=std::cos(yaw),s=std::sin(yaw);return {p.x*c+p.z*s,p.y,-p.x*s+p.z*c}; }
std::shared_ptr<const Material> paving(bool apron) {
    constexpr int n=512;
    auto albedo=std::make_shared<Texture>(),normal=std::make_shared<Texture>(),rough=std::make_shared<Texture>();
    const std::string name=apron?"Weathered service concrete / 8 metres":"Promenade slabs / 4 metres";
    for(auto t:{albedo,normal,rough}) {t->width=t->height=n;t->pixels.resize(n*n*4);}
    albedo->name=name+" albedo";normal->name=name+" normal";rough->name=name+" roughness";
    std::vector<float> relief(n*n);
    auto byte=[](float f){return std::uint8_t(std::clamp(f,0.f,255.f));};
    for(int y=0;y<n;++y)for(int x=0;x<n;++x) {
        float grain=track_surface::noise(x,y,n,419),cloud=track_surface::smoothNoise(x/64.f,y/64.f,8,712);
        float grime=track_surface::smoothNoise(x/32.f,y/32.f,16,83);
        int sx=x%256,sy=y%256;
        float seam=1-std::clamp(float(std::min({sx,256-sx,sy,256-sy})-1)/2.f,0.f,1.f);
        float repair=apron&&x>72&&x<219&&y>100&&y<318?1.f:0.f;
        float stain=std::clamp((grime-.52f)*4,0.f,1.f);
        float value=(apron?105.f:152.f)+9*(cloud-.5f)+7*(grain-.5f)-stain*10-seam*30-repair*7;
        unsigned i=(y*n+x)*4;
        albedo->pixels[i]=byte(value*1.025f);albedo->pixels[i+1]=byte(value);albedo->pixels[i+2]=byte(value*.96f);albedo->pixels[i+3]=255;
        relief[y*n+x]=.12f*grain-.6f*seam+.08f*repair;
        rough->pixels[i]=255;rough->pixels[i+1]=byte(244-stain*13);rough->pixels[i+2]=0;rough->pixels[i+3]=255;
    }
    auto h=[&](int x,int y){return relief[((y+n)%n)*n+(x+n)%n];};
    for(int y=0;y<n;++y)for(int x=0;x<n;++x) {
        auto v=normalized(Vec3{(h(x-1,y)-h(x+1,y))*.8f,(h(x,y+1)-h(x,y-1))*.8f,1});unsigned i=(y*n+x)*4;
        normal->pixels[i]=byte((v.x*.5f+.5f)*255);normal->pixels[i+1]=byte((v.y*.5f+.5f)*255);
        normal->pixels[i+2]=byte((v.z*.5f+.5f)*255);normal->pixels[i+3]=255;
    }
    auto m=std::make_shared<Material>();m->name=name;m->baseColorTexture=albedo;m->normalTexture=normal;
    m->metallicRoughnessTexture=rough;m->roughness=1;m->validate();return m;
}
std::shared_ptr<const Texture> fade(bool rectangular) {
    auto t=std::make_shared<Texture>();t->name=rectangular?"Foundation contact falloff":"Soft light and damp falloff";
    t->width=t->height=64;t->pixels.resize(64*64*4);
    for(int y=0;y<64;++y)for(int x=0;x<64;++x) {
        float u=std::abs((x-31.5f)/31.5f),v=std::abs((y-31.5f)/31.5f);
        float radius=rectangular?std::max(u,v):std::hypot(u,v);
        float alpha=rectangular?std::clamp((1-radius)*4,0.f,1.f):std::pow(std::max(0.f,1-radius),1.6f);
        unsigned i=(y*64+x)*4;t->pixels[i]=t->pixels[i+1]=t->pixels[i+2]=255;t->pixels[i+3]=std::uint8_t(alpha*255);
    }return t;
}
struct Resources {
    std::array<std::shared_ptr<const Material>,8> materials;
    std::shared_ptr<const Material> shadow,pool,damp,reflection;
    Resources() {
        materials[0]=paving(false);materials[1]=paving(true);
        constexpr Vec3 colors[]={{.12f,.16f,.19f},{.29f,.34f,.36f},{.40f,.30f,.18f},{.70f,.53f,.18f},{.70f,.77f,.75f},{.035f,.055f,.075f}};
        for(int i=2;i<8;++i) {auto m=std::make_shared<Material>();m->name="Service yard material "+std::to_string(i);m->baseColor=colors[i-2];
            m->roughness=i==7?.3f:.78f;m->metallic=i==3?.65f:.12f;materials[i]=m;}
        auto decal=[&](const char* name,bool rectangle,AlphaMode mode,float opacity) {
            auto m=std::make_shared<Material>();m->name=name;m->shader=MaterialShader::Unlit;
            m->baseColorTexture=fade(rectangle);m->alphaMode=mode;m->opacity=opacity;return m;
        };
        shadow=decal("Static contact shading",true,AlphaMode::Blend,.43f);
        pool=decal("Authored lamp receiver pool",false,AlphaMode::Additive,.18f);
        reflection=decal("Subtle neon damp streak",false,AlphaMode::Additive,.20f);
        auto water=decal("Still residual rain",false,AlphaMode::Blend,.56f);water->shader=MaterialShader::Water;
        water->baseColor={.055f,.07f,.09f};water->roughness=.18f;water->water.amplitude=0;water->water.speed=0;
        water->water.rippleStrength=.015f;water->water.reflectionStrength=.65f;
        water->water.planarReflectionStrength=0;water->water.refractionStrength=0;
        water->water.horizon={.09f,.07f,.15f};water->water.zenith={.02f,.025f,.05f};water->validate();damp=water;
    }
};
const Resources& resources(){static const Resources r;return r;}
struct Assembly {
    Scene& scene;const Circuit& track;Report report;
    std::map<std::pair<int,int>,std::shared_ptr<Mesh>> batches;
    std::map<std::pair<int,int>,std::shared_ptr<Mesh>> accents;
    Mesh& batch(Vec3 p,int material) {auto& mesh=batches[{track.nearest(p).segment/16,material}];if(!mesh)mesh=std::make_shared<Mesh>();return *mesh;}
    void box(Vec3 p,Vec3 size,float yaw,int material=2) {
        static const auto cube=Mesh::cube();auto& m=batch(p,material);unsigned base=m.vertices.size();
        for(auto v:cube->vertices) {
            v.position=p+rotate({v.position.x*size.x,v.position.y*size.y,v.position.z*size.z},yaw);
            v.normal=rotate(v.normal,yaw);v.uv={v.position.x/(material==1?8.f:4.f),v.position.z/(material==1?8.f:4.f)};m.vertices.push_back(v);
        }
        for(auto i:cube->indices)m.indices.push_back(base+i);
    }
    void quad(Vec3 a,Vec3 b,Vec3 c,Vec3 d,int material) {
        auto& m=batch((a+b+c+d)*.25f,material);unsigned n=m.vertices.size();
        for(auto p:{a,b,c,d})m.vertices.push_back({p,{0,1,0},{p.x/4,p.z/4}});
        m.indices.insert(m.indices.end(),{n,n+1,n+2,n,n+2,n+3});
    }
    void decal(const char* name,Vec3 p,Vec3 size,float yaw,std::shared_ptr<const Material> material,Vec3 color) {
        const auto& r=resources();int kind=material==r.shadow?0:material==r.pool?1:material==r.damp?2:3;
        auto& mesh=accents[{track.nearest(p).segment/16,kind}];if(!mesh)mesh=std::make_shared<Mesh>();
        unsigned n=mesh->vertices.size();static const auto plane=Mesh::plane();
        for(auto v:plane->vertices) {v.position=p+rotate({v.position.x*size.x,0,v.position.z*size.z},yaw);mesh->vertices.push_back(v);}
        for(auto i:plane->indices)mesh->indices.push_back(n+i);
    }
    void contact(Vec3 p,Vec3 d,float yaw,float y=.008f) {
        p.y=y;decal("City ground / contact shading",p,{d.x+1.5f,1,d.z+1.5f},yaw,resources().shadow,{.008f,.012f,.02f});++report.contactShadows;
    }
    void wet(Vec3 p,float yaw,float surface=0) {
        p.y=surface+.006f;decal("City ground / damp patch",p,{3.8f,1,2.5f},yaw,resources().damp,{1,1,1});
        p.y+=.004f;decal("City ground / neon damp reflection",p,{.75f,1,2.1f},yaw,resources().reflection,{.035f,.44f,.50f});++report.dampPatches;
    }
    void finish() {
        const auto& r=resources();
        const std::array<std::shared_ptr<const Material>,4> materials{r.shadow,r.pool,r.damp,r.reflection};
        constexpr Vec3 colors[]={{.008f,.012f,.02f},{1,.69f,.33f},{1,1,1},{.035f,.44f,.50f}};
        constexpr const char* names[]={"contact shading","warm lamp pool","damp patch","neon damp reflection"};
        for(auto& [key,mesh]:accents) {
            mesh->generateTangents();auto& actor=scene.spawn("City ground / "+std::string(names[key.second]));
            auto& visual=actor.addComponent<MeshComponent>(actor.name,mesh);visual.color=colors[key.second];
            visual.setMaterial(0,MaterialInstance(materials[key.second]));visual.castsShadow=false;
            ++report.meshes;report.triangles+=mesh->indices.size()/3;
        }
        for(auto& [key,mesh]:batches) {mesh->generateTangents();auto& actor=scene.spawn("City ground / sector "+std::to_string(key.first));
            auto& visual=actor.addComponent<MeshComponent>("City ground / opaque batch",mesh);visual.color={1,1,1};
            visual.setMaterial(0,MaterialInstance(resources().materials[key.second]));visual.allowInstanceReordering=true;
            ++report.meshes;report.triangles+=mesh->indices.size()/3;
        }
    }
};
bool plazaSegment(int sample) {for(auto [a,b]:plaza::sectors)if(sample>=a&&sample<b)return true;return false;}
void promenades(Assembly& a) {
    for(int i=0;i<Circuit::samples;++i)for(int side:{-1,1}) {
        if(side==1&&plazaSegment(i))continue;
        auto point=[&](int s,float r,float y){auto p=a.track.point(s)+a.track.right(s)*(r*side);p.y=y;return p;};
        // Existing imported sidewalks end at 13.95 m / Y=.185. The inset
        // channel has a genuinely lower bed; crossbars leave visible gaps.
        auto strip=[&](float r1,float r2,float y,int material) {
            auto p=point(i,r1,y),q=point(i,r2,y),r=point(i+1,r2,y),s=point(i+1,r1,y);
            if(side>0)a.quad(p,s,r,q,material);else a.quad(p,q,r,s,material);
        };
        strip(14.6f,20.f,.185f,0);strip(14.f,14.6f,.045f,2);
        // The vertical outer edge gives the raised paving a readable silhouette.
        auto p=point(i,20,.0925f),q=point(i+1,20,.0925f),delta=q-p;
        a.box((p+q)*.5f,{.18f,.185f,magnitude(delta)+.03f},std::atan2(delta.x,delta.z),1);
        for(int j=0;j<5;++j) {
            float t=(j+.3f)/5;auto center=point(i,14.3f,.155f)*(1-t)+point(i+1,14.3f,.155f)*t;
            a.box(center,{.53f,.045f,.055f},std::atan2(-a.track.right(i).z,a.track.right(i).x),3);
        }
        ++a.report.promenadeSegments;++a.report.drainSegments;
    }
}
void van(Assembly& a,Vec3 p,float yaw) {
    a.report.parked[a.report.serviceVehicles]={p,yaw};
    a.contact(p,{2.6f,1,5.2f},yaw,.058f);++a.report.serviceVehicles;
}
}
Report build(Scene& scene,const Circuit& track,std::span<const buildingart::Placement> buildings,
    std::span<const citykit::Placement> city,std::span<const streetkit::Placement> street,std::span<const trackkit::Placement> lamps) {
    if(track.kind!=TrackKind::Kairox)return {};
    Assembly a{scene,track};
    // Shared metric UVs replace the former featureless ground, under all roads.
    auto& ground=scene.spawn("City ground / weathered district floor");
    auto mesh=std::make_shared<Mesh>(*Mesh::plane(900));for(auto& v:mesh->vertices)v.uv={v.position.x/8,v.position.z/8};mesh->generateTangents();
    auto& visual=ground.addComponent<MeshComponent>("City ground / service concrete",mesh);visual.transform.position.y=.001f;
    visual.color={.72f,.75f,.80f};visual.setMaterial(0,MaterialInstance(resources().materials[1]));++a.report.meshes;a.report.triangles+=2;
    promenades(a);
    for(const auto& b:buildings) {
        auto d=buildingart::models[b.model].dimensions;
        auto local=[&](Vec3 p){return b.position+rotate(p,b.yaw);};
        a.box(local({0,.035f,0}),{d.x+.8f,.07f,d.z+.8f},b.yaw,1);++a.report.foundations;
        a.contact(b.position,{d.x+1,1,d.z+1},b.yaw);
        // Facade aprons tie doors to the existing sidewalks without lifting art.
        a.box(local({d.x*.5f+1.8f,.04f,0}),{3.6f,.08f,d.z*.72f},b.yaw,1);
        auto p=local({d.x*.5f+2.8f,0,-d.z*.28f});
        // Deliberate clusters: cabinet, vent louvers, pipe risers and framed crates.
        auto part=[&](Vec3 v,Vec3 size,int material){a.box(p+rotate(v,b.yaw),size,b.yaw,material);};
        part({0,.83f,0},{.8f,1.5f,1.45f},3);part({.415f,.85f,0},{.025f,1.25f,1.18f},2);
        for(int i=0;i<7;++i)part({.44f,.52f+i*.1f,0},{.06f,.035f,.94f},3);
        for(float z:{-1.1f,-1.45f}) {part({-.3f,1.6f,z},{.12f,3.1f,.12f},3);part({.2f,3.1f,z},{1.1f,.12f,.12f},3);}
        for(int i=0;i<2;++i) {float z=1.5f+i*.95f;part({.2f,.51f,z},{.85f,.86f,.8f},4);
            for(float x:{-.24f,.64f})part({x,.51f,z},{.075f,.94f,.86f},3);part({.2f,.53f,z},{.91f,.075f,.86f},3);}
        a.contact(p,{2.f,1,5.f},b.yaw,.086f);++a.report.utilityClusters;
        if(b.model==1||b.model==6) {
            auto dock=local({d.x*.5f-.75f,0,d.z*.24f});
            a.box(dock+Vec3{0,.4f,0},{7.5f,.8f,5.6f},b.yaw,1);
            for(int step=0;step<4;++step)a.box(dock+rotate({3.95f+step*.38f,.1f*(4-step),0},b.yaw),{.4f,.2f*(4-step),2.2f},b.yaw,0);
            for(int i=0;i<7;++i)a.box(dock+rotate({3.78f,.71f,-2.4f+i*.75f},b.yaw),{.045f,.15f,.35f},b.yaw,5);
            a.contact(dock,{7.8f,1,6.f},b.yaw);++a.report.loadingDocks;
            if(b.model==6)a.report.views[1]={dock+rotate({15,7,-9},b.yaw),dock+Vec3{0,.6f,0}};
        }
        if(b.model==4)a.report.views[2]={p+rotate({12,3.2f,-5},b.yaw),p+Vec3{0,.65f,0}};
        a.wet(local({d.x*.5f+2.3f,0,3}),b.yaw,.081f);
    }
    // Buildings including the complete skyline, stairs and supports get local
    // contact falloff, rather than a screen-space effect or a new shadow pass.
    for(const auto& p:city) {
        auto d=citykit::models[p.model].dimensions*p.scale;
        if(p.position.y<.3f)a.contact(p.position,{d.x,1,d.z},p.yaw,p.position.y+.008f);
        if(p.model>=6&&(p.model==6||p.model==7||p.model>=11)) {
            a.box(p.position+Vec3{0,.025f,0},{d.x+.6f,.05f,d.z+.6f},p.yaw,1);++a.report.foundations;
        }
    }
    for(const auto& p:street)if(p.model==0||p.model==3) {
        auto d=streetkit::models[p.model].dimensions;
        a.contact(p.position,{d.x*p.scale.x,1,d.z*p.scale.z},p.yaw);
    }
    for(Vec3 p:{Vec3{23.3f,0,25},Vec3{30.3f,0,25}})a.contact(p,{1.2f,1,3.4f},0);
    for(const auto& p:lamps)if(p.model==5||p.model==6) {
        auto emitter=p.position+rotate(p.model==5?Vec3{.85f,6.8f,0}:Vec3{0,.35f,.2f},p.yaw);
        Vec3 receiver=emitter;receiver.y=p.model==5?.195f:.09f;
        a.decal("City ground / warm lamp pool",receiver,p.model==5?Vec3{6,1,7}:Vec3{3,1,4},p.yaw,resources().pool,{1,.69f,.33f});++a.report.lightPools;
        if(p.model==5&&a.report.lightPools%3==0) {receiver=track.point(track.nearest(receiver).segment)+track.right(track.nearest(receiver).segment)*-17.f;
            a.wet(receiver,p.yaw,.185f);}
    }
    for(int sample:{30,84,150}) {
        bool placed=false;
        for(float distance=26;distance<=64&&!placed;distance+=4)for(float along:{0.f,7.f,-7.f}) {
            auto p=track.point(sample)+track.right(sample)*distance+track.tangent(sample)*along;
            if(track.nearest(p).distance<19)continue;
            auto clear=[&](Vec3 other,float radius){other.y=0;return magnitude(p-other)>radius+4.f;};
            if(std::any_of(buildings.begin(),buildings.end(),[&](const auto& b){return !clear(b.position,b.radius);})||
               std::any_of(city.begin(),city.end(),[&](const auto& b){return !clear(b.position,b.radius);})||
               std::any_of(street.begin(),street.end(),[&](const auto& b){auto d=streetkit::models[b.model].dimensions;return !clear(b.position,std::hypot(d.x*b.scale.x,d.z*b.scale.z)*.5f);}))continue;
            const float yaw=std::atan2(track.tangent(sample).x,track.tangent(sample).z);
            a.box(p+Vec3{0,.025f,0},{6,.05f,8},yaw,1);van(a,p,yaw);
            if(sample==30)a.report.views[4]={p-track.right(sample)*9.f+Vec3{0,3.8f,0},p+Vec3{0,1,0}};
            placed=true;break;
        }
    }
    a.report.views[0]={track.point(56)-track.tangent(56)*12.f+Vec3{0,4.f,0},{23,.5f,25}};
    auto p=track.point(38),r=track.right(38),t=track.tangent(38);
    a.report.views[3]={p-r*2-t*12+Vec3{0,2.5f,0},p+r*18+Vec3{0,.3f,0}};
    a.finish();return a.report;
}
void installParked(Scene& scene,const Report& report,const Mesh& source) {
    if(!report.serviceVehicles)return;
    const auto bounds=source.localBounds();
    if(!bounds.valid||report.serviceVehicles>report.parked.size())throw std::invalid_argument("Invalid parked vehicle assembly");
    const auto shared=art::prepareStatic(source);
    static const auto cube=Mesh::cube();
    std::vector<std::unique_ptr<Actor>> staged;
    for(unsigned i=0;i<report.serviceVehicles;++i) {
        const auto& p=report.parked[i];auto actor=std::make_unique<Actor>("City service / parked Bastion");
        actor->transform.position=p.position+Vec3{0,.055f-bounds.minimum.y,0};actor->transform.rotation.y=p.yaw;
        auto& body=actor->addComponent<MeshComponent>("Shared parked service chassis",shared);body.color={1,1,1};body.allowInstanceReordering=true;
        auto fitting=[&](const char* name,Vec3 position,Vec3 size,Vec3 color,bool glow=false) {
            auto& m=actor->addComponent<MeshComponent>(name,cube);m.transform.position=position;m.transform.scale=size;m.color=color;m.allowInstanceReordering=true;
            MaterialInstance material;material.roughness=.73f;material.metallic=.35f;
            if(glow){material.shader=MaterialShader::Unlit;m.castsShadow=false;}m.setMaterial(0,material);
        };
        const float top=bounds.maximum.y;
        for(float side:{-.65f,.65f})fitting("Service roof rack",{side,top+.07f,-.3f},{.06f,.12f,1.8f},{.14f,.18f,.21f});
        for(float z:{-.95f,.35f})fitting("Service rack crossbar",{0,top+.09f,z},{1.4f,.08f,.07f},{.14f,.18f,.21f});
        fitting("Service equipment case",{0,top+.27f,-.6f},{1.12f,.34f,.9f},{.32f,.29f,.21f});
        for(float x:{-.48f,.48f})fitting("Case restraint",{x,top+.28f,-.6f},{.035f,.37f,.92f},{.09f,.11f,.13f});
        fitting("Amber service light",{0,top+.18f,.48f},{.82f,.14f,.16f},{.8f,.36f,.035f},true);
        staged.push_back(std::move(actor));
    }
    for(auto& actor:staged)scene.adoptChild(std::move(actor));
}
}
