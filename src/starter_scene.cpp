#include "starter_scene.hpp"
#include "desert_surface.hpp"
#include "static_art.hpp"
#include "engine/collider_component.hpp"
#include "cybercar_contract.hpp"
#include "vehicle_art_contract.hpp"
#include "weapon_art_contract.hpp"
#include "track_surface.hpp"
#include <unordered_map>
#include "engine/physics.hpp"
#include <string>

namespace racing {
using namespace engine;
namespace {
const Vec3 cyan{.04f,.85f,.95f},pink{.95f,.06f,.42f},amber{1,.55f,.08f};
MeshComponent& box(Actor& actor,const std::string& name,Vec3 p,Vec3 size,Vec3 color,bool glow=false) {
    auto& m=actor.addComponent<MeshComponent>(name,Mesh::cube());
    m.transform.position=p;m.transform.scale=size;m.color=color;m.allowInstanceReordering=true;
    MaterialInstance material;material.roughness=.55f;material.metallic=.25f;
    if(glow){material.shader=MaterialShader::Unlit;m.castsShadow=false;}
    m.setMaterial(0,material);return m;
}
Actor& solid(Scene& scene,const std::string& name,Vec3 p,Vec3 size,Vec3 color,float yaw=0) {
    auto& a=scene.spawn(name);a.transform.position=p;a.transform.rotation.y=yaw;
    RigidBodySettings s;s.motion=BodyMotion::Static;
    a.addComponent<RigidBodyComponent>("Static body",s);
    a.addComponent<ColliderComponent>("Solid",CollisionShape::box(size*.5f));
    box(a,name,{},size,color);return a;
}
std::shared_ptr<const Mesh> ribbon(const Circuit& track,float inner,float outer,float y) {
    auto mesh=std::make_shared<Mesh>();
    float length=0,distance=0;
    for(int i=0;i<Circuit::samples;++i)length+=magnitude(track.point(i+1)-track.point(i));
    // Fit an integer number of metric tiles around the loop, so the start/finish
    // seam closes without stretching textures by waypoint spacing.
    const float repeats=std::max(1.f,std::round(length/track_surface::tileMetres));
    for(int i=0;i<=Circuit::samples;++i) {
        if(i)distance+=magnitude(track.point(i)-track.point(i-1));
        auto p=track.point(i),r=track.right(i);
        for(float side:{inner,outer}) {
            auto v=p+r*side;v.y=y;
            mesh->vertices.push_back({v,{0,1,0},{side/track_surface::tileMetres,distance/length*repeats}});
        }
        if(i<Circuit::samples) {
            auto n=static_cast<std::uint32_t>(i*2);
            mesh->indices.insert(mesh->indices.end(),{n,n+2,n+1,n+1,n+2,n+3});
        }
    }
    mesh->generateTangents();
    return mesh;
}
class RaceDriver final:public Actor {
public:
    explicit RaceDriver(StarterScene& scene):Actor("Race simulation"),game(scene){setTickEnabled(true);}
    void fixedTick(double dt) override { game.fixedStep(dt); }
private:StarterScene& game;
};
}
StarterScene::StarterScene(TrackKind selected):track(selected),aiPlanner(track) {
    name="Kairox / Midnight Circuit";ambient={.34f,.39f,.52f};environmentIntensity=.48f;
    SkyBackground sky;sky.horizon={.07f,.035f,.15f};sky.zenith={.006f,.009f,.035f};sky.nadir={.01f,.02f,.04f};setSky(sky);
    FogSettings fog;fog.color={.055f,.035f,.12f};fog.density=.004f;fog.startDistance=70;fog.heightFalloff=.015f;setFog(fog);
    auto& light=spawn("Moonlight").addComponent<LightComponent>("Moon",LightType::Directional);
    light.direction={-.65f,-.85f,.35f};light.color={.72f,.82f,1};light.intensity=1.65f;
    if(isDesert()) {
        name="Desert Solar / Helios Run";ambient={.36f,.39f,.43f};environmentIntensity=.48f;
        sky.horizon={.78f,.72f,.62f};sky.zenith={.42f,.56f,.68f};sky.nadir={.38f,.23f,.11f};setSky(sky);
        fog.color={.64f,.57f,.46f};fog.density=.0015f;fog.startDistance=85;fog.heightFalloff=.012f;setFog(fog);
        light.color={1,.88f,.72f};light.intensity=2.35f;light.direction={-.65f,-.72f,-.32f};
        // The visible sky and indirect light share one palette. The old default
        // studio cube introduced blue fill and a second, unrelated sun highlight.
        // This track has fixed daylight; share the immutable bake across restarts.
        static const auto desertEnvironment=Environment::bake("Helios desert daylight",
            [sky,sunDirection=normalized(light.direction*-1.f),sunRadiance=light.color*(light.intensity*3.f)](Vec3 d) {
                const float hemisphere=std::abs(d.y);
                const Vec3 background=sky.horizon*(1-hemisphere)+(d.y>=0?sky.zenith:sky.nadir)*hemisphere;
                const float sun=std::pow(std::max(dot(d,sunDirection),0.f),256.f);
                return background+sunRadiance*sun;
            });
        environment=desertEnvironment;
        light.shadow.enabled=true;light.shadow.halfExtent=240;light.shadow.halfDepth=310;
        desertKit.plan(track);buildCity();
    } else {
        planBuildings();planCityKit();streetKit.plan(track,buildings);trackKit.plan(track,buildings);buildCity();buildCityKitSupports();
        cityGround_=cityground::build(*this,track,buildings,cityDetails,streetKit.placements(),trackKit.placements());
    }
    buildVehicles();
    camera=&spawn("Chase camera").addComponent<CameraComponent>();
    camera->setPerspective(65*pi/180,.1f,650);
    spawn<RaceDriver>(*this);
    synchronize();
    cameraRig=std::make_unique<CameraFollowRig>(*this,camera->id());
    select(VehicleKind::Car);
    physics().onCollision=[this](const CollisionEvent& event) {
        if(phase!=Phase::Racing||!event.impactValid||event.closingSpeed<2.5f||std::abs(event.normal.y)>.75f)return;
        for(auto& v:vehicles)if(v.actor->enabled&&(event.first==v.actor->id()||event.second==v.actor->id())&&worldTime-v.lastAudioImpact>.3) {
            ++v.audioImpacts;v.lastAudioImpact=worldTime;
        }
    };
}
StarterScene::~StarterScene()=default;
void StarterScene::planBuildings() {
    // Leave the transit deck corridor clear. Front facades face local +X.
    constexpr int samples[]={0,12,24,36,64,80,96,112,128,156,168,180};
    // Unique landmarks replace repeated frontage: garage at the grid, arcade
    // at the market bend, warehouse on the service stretch, corner apartments
    // on the return. Keep the same bounded twelve-building scene budget.
    constexpr unsigned models[]={4,3,1,0,2,6,0,5,1,0,2,1};
    for(unsigned i=0;i<std::size(samples);++i) {
        auto dimensions=buildingart::models[models[i]].dimensions;
        float radius=std::hypot(dimensions.x,dimensions.z)*.5f;
        float side=(i==3||i==9)?-1.f:1.f;
        auto outward=track.right(samples[i])*side;
        auto position=track.point(samples[i])+outward*(radius+17.f);
        // Circle clearance conservatively covers every rotated footprint corner.
        auto clear=[&] {
            if(track.nearest(position).distance<radius+16.f)return false;
            for(const auto& b:buildings)
                if(magnitude(position-b.position)<radius+b.radius+1.f)return false;
            return true;
        };
        for(int step=0;step<100&&!clear();++step)position=position+outward;
        if(!clear())throw std::logic_error("No clear trackside building placement");
        auto front=outward*-1.f;
        buildings.push_back({models[i],position,std::atan2(-front.z,front.x),radius});
    }
}
void StarterScene::installBuildingArt(unsigned model,std::span<const std::byte> bytes) {
    if(model>=buildingsLoaded.size()||buildingsLoaded[model])throw std::logic_error("Invalid or installed building");
    const auto& contract=buildingart::models[model];
    GlbImportOptions options;options.optimizeMeshes=false;options.maxTextureBytes=16*1024*1024;
    auto mesh=GlbImporter::loadMemory(bytes,options);
    const auto& bounds=mesh->localBounds();
    if(!mesh->warnings.empty()||mesh->parts.size()!=1||mesh->materials.size()!=1||
       mesh->indices.size()!=contract.triangles*3||!bounds.valid||
       magnitude((bounds.maximum-bounds.minimum)-contract.dimensions)>.02f||std::abs(bounds.minimum.y)>.01f)
        throw std::runtime_error("Building art contract mismatch");
    mesh=art::prepareStatic(*mesh);
    // Stage complete actors before publishing. Every instance shares mesh/maps.
    std::vector<std::unique_ptr<Actor>> staged;
    for(const auto& placement:buildings)if(placement.model==model) {
        auto actor=std::make_unique<Actor>(contract.name);
        actor->transform.position=placement.position;actor->transform.rotation.y=placement.yaw;
        auto& visual=actor->addComponent<MeshComponent>("Trackside building",mesh);
        visual.allowInstanceReordering=true;
        RigidBodySettings body;body.motion=BodyMotion::Static;
        actor->addComponent<RigidBodyComponent>("Building body",body);
        auto& collision=actor->addComponent<ColliderComponent>("Building footprint",CollisionShape::box(contract.dimensions*.5f));
        collision.transform.position={0,contract.dimensions.y*.5f,0};
        staged.push_back(std::move(actor));
    }
    for(auto& actor:staged)adoptChild(std::move(actor));
    buildingsLoaded[model]=true;synchronize();
}
void StarterScene::buildCity() {
    auto& foundation=solid(*this,"Circuit foundation",{0,-.5f,0},{900,1,900},isDesert()?Vec3{.52f,.245f,.085f}:Vec3{.025f,.03f,.055f});
    // Keep the old foundation visual below both authored ground surfaces. Tiny
    // offsets fight at racing distances; the physical driving collider stays Y=0.
    foundation.getComponent<MeshComponent>()->transform.position.y=-.1f;
    auto& road=spawn("Midnight circuit");
    auto& asphalt=road.addComponent<MeshComponent>("Asphalt",ribbon(track,-10,10,.015f));
    asphalt.color={1,1,1};asphalt.setMaterial(0,MaterialInstance(track_surface::asphalt()));
    for(int side:{-1,1}) {
        auto& shoulder=road.addComponent<MeshComponent>(side<0?"Inner concrete shoulder":"Outer concrete shoulder",
            ribbon(track,side<0?-12.f:10.f,side<0?-10.f:12.f,.02f));
        shoulder.color={1,1,1};shoulder.setMaterial(0,MaterialInstance(track_surface::concrete()));
        auto& line=road.addComponent<MeshComponent>("Neon edge",ribbon(track,side*9.65f-.12f,side*9.65f+.12f,.025f));
        line.color=isDesert()?(side==1?amber:cyan):(side==1?cyan:pink);MaterialInstance m;m.shader=MaterialShader::Unlit;line.setMaterial(0,m);
    }
    for(int i=0;i<Circuit::samples;i+=2) {
        auto p=track.point(i),t=track.tangent(i),r=track.right(i);
        float yaw=std::atan2(t.x,t.z);
        if(i%4==0) {auto& dash=box(road,"Lane dash",p+Vec3{0,.035f,0},{.14f,.025f,2.3f},{.38f,.48f,.65f},true);dash.transform.rotation.y=yaw;}
        for(int side:{-1,1}) {
            Vec3 a=p+r*(side*11.3f),b=track.point(i+2)+track.right(i+2)*(side*11.3f);
            auto center=(a+b)*.5f;center.y=.6f;
            float segmentYaw=std::atan2((b-a).x,(b-a).z);
            auto& barrier=spawn("Crash barrier");barrier.transform.position=center;barrier.transform.rotation.y=segmentYaw;
            RigidBodySettings staticBody;staticBody.motion=BodyMotion::Static;
            barrier.addComponent<RigidBodyComponent>("Static body",staticBody);
            barrier.addComponent<ColliderComponent>("Solid",CollisionShape::box(Vec3{.65f,1.2f,magnitude(b-a)+.2f}*.5f));
            if(!isDesert())box(barrier,"Barrier light",{-side*.22f,.615f,0},{.07f,.035f,magnitude(b-a)},i%8==0?amber:side==1?cyan:pink,true);
        }
    }
    if(isDesert()){buildDesert();return;}
    unsigned random=89;
    auto rnd=[&](){random=random*1664525u+1013904223u;return float((random>>8)%10000)/10000;};
    struct SkylineSite { Vec3 position; };
    std::vector<SkylineSite> skylineCandidates;
    for(int z=-210;z<=210;z+=28)for(int x=-200;x<=200;x+=28) {
        Vec3 p{float(x)+rnd()*6,0,float(z)+rnd()*6};
        if(track.nearest(p).distance<29)continue;
        if(std::any_of(buildings.begin(),buildings.end(),[&](const auto& b){return magnitude(p-b.position)<b.radius+16.f;}))continue;
        if(streetKit.overlapsSkyline(p,16.f))continue;
        if(std::any_of(cityDetails.begin(),cityDetails.end(),[&](const auto& b){auto q=b.position;q.y=0;return magnitude(p-q)<b.radius+((b.model==6||b.model==7)?35.f:16.f);}))continue;
        // Retain the old grid's random sequence so existing scenery stays put.
        for(int draw=0;draw<4;++draw)rnd();
        skylineCandidates.push_back({p});
    }
    std::stable_sort(skylineCandidates.begin(),skylineCandidates.end(),[&](const auto& a,const auto& b){
        return track.nearest(a.position).distance<track.nearest(b.position).distance;
    });
    unsigned placed=0;
    constexpr unsigned variants[]{12,13,12,13,6,7,12,13,12,13,6,7};
    for(const auto& site:skylineCandidates) {
        const auto p=site.position;
        const auto model=variants[placed++%std::size(variants)];
        const auto d=citykit::models[model].dimensions;
        const float radius=std::hypot(d.x,d.z)*.5f;
        float available=15.f;
        // Reserve half the gap for each neighbour, independent of model/order.
        // The circle contains the complete footprint at any facade rotation.
        for(const auto& other:skylineCandidates)if(&site!=&other)
            available=std::min(available,(magnitude(p-other.position)-1.f)*.5f);
        for(const auto& other:cityDetails) {
            auto q=other.position;q.y=0;
            available=std::min(available,magnitude(p-q)-other.radius-1.f);
        }
        if(available<=0.f)throw std::logic_error("No clear skyline footprint");
        const float scale=std::min(1.f,available/radius);
        auto front=track.nearest(p).position-p;
        // These building references have their storefront/front face on +X.
        cityDetails.push_back({model,p,std::atan2(-front.z,front.x),radius*scale,scale});
    }
    for(int g=0;g<Circuit::gates;++g) {
        auto p=track.gate(g),t=track.gateForward(g);
        auto& gate=spawn(g==0?"Finish gantry":"Sector gantry");gate.transform.position=p;gate.transform.rotation.y=std::atan2(t.x,t.z);
        if(g==0)for(int x=0;x<20;++x)for(int z=0;z<2;++z)
            box(gate,"Finish checker",{-9.5f+x,.045f,float(z)-.5f},{1,.04f,1},(x+z)%2?Vec3{.8f,.85f,.95f}:Vec3{.02f,.03f,.05f},true);
    }

}
void StarterScene::buildDesert() {
    desertRaceKit.plan(track);
    duneVillages.plan(track);
    // Only the road and working facility are graded. Rock skirts remain sloped.
    auto height=[&](float x,float z) {
        return desertKit.height(track,x,z);
    };
    auto mesh=std::make_shared<Mesh>();constexpr int cells=240;constexpr float step=3.5f;
    std::vector<float> heights((cells+1)*(cells+1));
    for(int z=0;z<=cells;++z)for(int x=0;x<=cells;++x)
        heights[z*(cells+1)+x]=height((x-cells*.5f)*step,(z-cells*.5f)*step);
    auto sample=[&](int x,int z){return heights[std::clamp(z,0,cells)*(cells+1)+std::clamp(x,0,cells)];};
    for(int z=0;z<=cells;++z)for(int x=0;x<=cells;++x) {
        float px=(x-cells*.5f)*step,pz=(z-cells*.5f)*step;
        auto normal=normalized(Vec3{sample(x-1,z)-sample(x+1,z),2*step,sample(x,z-1)-sample(x,z+1)});
        mesh->vertices.push_back({{px,sample(x,z),pz},normal,{px/3,pz/3}});
    }
    for(int z=0;z<cells;++z)for(int x=0;x<cells;++x) {
        auto n=std::uint32_t(z*(cells+1)+x);
        mesh->indices.insert(mesh->indices.end(),{n,n+cells+1,n+1,n+1,n+cells+1,n+cells+2});
    }
    mesh->generateTangents();
    auto& sand=spawn("Wind-sculpted orange dunes").addComponent<MeshComponent>("Dune terrain",mesh);
    sand.color={1,1,1};sand.setMaterial(0,MaterialInstance(desert::surfaceMaterial(false)));
    desertKit.dress(*this,track);
    for(int g=0;g<Circuit::gates;++g) {
        auto t=track.gateForward(g);auto& gate=spawn(g==0?"Helios finish":"Solar sector gate");
        gate.transform.position=track.gate(g);gate.transform.rotation.y=std::atan2(t.x,t.z);
        if(g==0) {
            for(int x=0;x<20;++x)for(int z=0;z<2;++z)
                box(gate,"Finish checker",{-9.5f+x,.045f,float(z)-.5f},{1,.04f,1},(x+z)%2?Vec3{.9f,.85f,.7f}:Vec3{.025f,.03f,.04f},true);
        }
    }
}
void StarterScene::buildVehicles() {
    const std::array<Vec3,7> colors{{{.8f,.065f,.22f},{.07f,.65f,.72f},
        {.9f,.5f,.07f},{.35f,.2f,.95f},{.15f,.85f,.35f},{.9f,.2f,.6f},{.2f,.5f,.95f}}};
    for(int i=0;i<static_cast<int>(vehicles.size());++i) {
        auto& v=vehicles[i];v.kind=i%2==0?VehicleKind::Car:VehicleKind::Hover;
        constexpr Chassis chassis[]={Chassis::Vektor,Chassis::Skimmer,Chassis::Bastion,Chassis::Vector,Chassis::Vektor,Chassis::Skimmer,Chassis::Bastion};
        v.chassis=chassis[i];
        bool car=v.kind==VehicleKind::Car;
        auto& a=spawn(car?"Vektor / street coupe":"Wisp / hovercraft");v.actor=&a;
        RigidBodySettings settings;settings.mass=car?1150:850;settings.continuousCollision=true;
        settings.centerOfMassOffset={0,-.28f,0};settings.inertiaScale={2,1,2};
        settings.gravityFactor=car?1:0;settings.angularDamping=.5f;settings.friction=.3f;
        v.body=&a.addComponent<RigidBodyComponent>("Vehicle body",settings);
        a.addComponent<ColliderComponent>("Vehicle hull",CollisionShape::box({.92f,.32f,1.95f}));
        auto& shell=box(a,"Armored body",{0,0,0},{1.85f,.52f,3.8f},colors[i]);v.shell=&shell;
        v.weapon=&a.addComponent<PulseWeapon>();v.weapon->fit(static_cast<int>(v.chassis));
        v.shieldEffect=&a.addComponent<ShieldEffect>();
        v.beam=&spawn("Laser shot").addComponent<LaserBeam>();
        box(a,"Canopy",{0,.43f,-.25f},{1.4f,.46f,1.45f},{.025f,.075f,.12f});
        box(a,"Windshield",{0,.52f,.5f},{1.26f,.25f,.04f},cyan*.6f,true);
        box(a,"Headlight strip",{0,.06f,1.93f},{1.6f,.09f,.05f},cyan,true);
        box(a,"Tail light",{0,.05f,-1.94f},{1.65f,.08f,.05f},pink,true);
        for(int side:{-1,1})box(a,"Side trim",{side*.95f,-.14f,0},{.07f,.08f,3.5f},colors[i],true);
        if(car) {
            auto definition=std::make_shared<VehicleDefinition>();
            definition->engineTorque=650;definition->frontTorqueFraction=.4f;definition->antiRollStiffness=1800;
            for(auto& w:definition->wheels){w.friction=2.2f;w.maxSteerAngle=w.maxSteerAngle>0?.42f:0;}
            v.wheels=&a.addComponent<VehicleComponent>("Wheeled drivetrain",definition);
            std::array<std::string,4> ids;
            for(int w=0;w<4;++w) {
                auto& tire=box(a,"Wheel",{}, {.28f,.7f,.7f},{.025f,.03f,.045f});
                ids[w]=tire.savedId();
            }
            v.wheels->setVisuals(ids);
            box(a,"Rear wing",{0,.55f,-1.65f},{2.15f,.1f,.4f},{.055f,.065f,.11f});
        } else {
            for(int side:{-1,1})for(int end:{-1,1}) {
                box(a,"Lift pod",{side*1.02f,-.1f,end*1.15f},{.5f,.45f,1.3f},{.08f,.12f,.2f});
                box(a,"Lift emitter",{side*1.02f,-.34f,end*1.15f},{.4f,.06f,1.1f},cyan,true);
            }
            box(a,"Dorsal fin",{0,.65f,-1.3f},{.12f,.75f,.65f},cyan);
        }
    }
    vehicles[1].actor->enabled=false;
}
void StarterScene::installPlayerCar(std::span<const std::byte> bytes) {
    if(carLoaded)throw std::logic_error("Player car is already installed");
    GlbImportOptions options;
    // Preserve primitive order for this asset's generated part contract.
    options.optimizeMeshes=false;
    options.maxTextureBytes=193*1024*1024; // Original body maps plus the small authored wheel palette.
    auto imported=GlbImporter::loadMemory(bytes,options);
    if(imported->parts.size()!=cybercar::parts.size() || !imported->warnings.empty())
        throw std::runtime_error("Cybercar mesh contract mismatch");
    std::array<std::shared_ptr<Mesh>,cybercar::parts.size()> meshes;
    auto definition=std::make_shared<VehicleDefinition>(*vehicles[0].wheels->definition());
    // Prepare all geometry before publishing any change to the live scene.
    for(std::size_t i=0;i<meshes.size();++i) {
        const auto& expected=cybercar::parts[i];const auto& range=imported->parts[i];
        if(range.indexCount!=expected.indices)throw std::runtime_error("Cybercar part index count mismatch");
        auto mesh=std::make_shared<Mesh>();mesh->materials={imported->materials.at(range.materialIndex)};
        std::unordered_map<std::uint32_t,std::uint32_t> remap;
        for(unsigned n=range.firstIndex;n<range.firstIndex+range.indexCount;++n) {
            auto source=imported->indices.at(n);
            auto [entry,inserted]=remap.emplace(source,static_cast<std::uint32_t>(mesh->vertices.size()));
            if(inserted) {
                auto vertex=imported->vertices.at(source);
                vertex.position=expected.wheel<0?cybercar::point(vertex.position):
                    cybercar::direction(vertex.position-expected.pivot)*cybercar::scale;
                vertex.normal=cybercar::direction(vertex.normal);
                auto tangent=cybercar::direction({vertex.tangent[0],vertex.tangent[1],vertex.tangent[2]});
                vertex.tangent={tangent.x,tangent.y,tangent.z,vertex.tangent[3]};
                mesh->vertices.push_back(vertex);
            }
            mesh->indices.push_back(entry->second);
        }
        mesh->parts.push_back({0,range.indexCount,0});meshes[i]=std::move(mesh);
        if(expected.wheel>=0) {
            auto& wheel=definition->wheels[expected.wheel];
            wheel.position=cybercar::point(expected.pivot)+Vec3{0,cybercar::restSuspension,0};
            wheel.radius=expected.radius*cybercar::scale;
            wheel.suspensionMin=cybercar::suspensionMin;
            wheel.suspensionMax=cybercar::suspensionMax;
            const auto bounds=meshes[i]->localBounds();
            wheel.width=bounds.maximum.x-bounds.minimum.x;
        }
    }
    definition->validate();
    auto& art=arts[0];
    art.meshes.assign(meshes.begin(),meshes.end());art.definition=definition;
    for(const auto& part:cybercar::parts)art.names.emplace_back(part.name);
    art.loaded=true;carLoaded=true;
    for(auto& vehicle:vehicles)if(vehicle.chassis==Chassis::Vektor)applyArt(vehicle);
    synchronize();resetGrid();
}
bool StarterScene::setPaint(int chassis,int rgb) {
    if(phase!=Phase::Ready||chassis<0||chassis>3||chassis!=static_cast<int>(selectedChassis())||rgb<0||rgb>Progression::originalPaint)return false;
    if(career.paint[chassis]==rgb)return true;
    career.paint[chassis]=rgb;paintedTextures[chassis].reset();applyPaint(racer(0));return true;
}
void StarterScene::applyPaint(VehicleRuntime& v) {
    const int c=static_cast<int>(v.chassis);auto& art=arts[c];if(!art.loaded)return;
    bool original=career.paint[c]==Progression::originalPaint;
    if(!original&&!paintedTextures[c]) {
        auto& surface=paintSurfaces[c];
        if(!surface.base)surface.prepare(*art.meshes[0]->materials[0]->baseColorTexture);
        paintedTextures[c]=surface.recolor(career.paint[c]);
    }
    // Only the player body uses this override. Wheels, liners, guns and rivals share authored assets.
    if(v.shell) {auto mat=engine::MaterialInstance{};
        if(!original)mat.baseColorTexture=paintedTextures[c];
        v.shell->setMaterial(0,mat);
    }
}
void StarterScene::applyArt(VehicleRuntime& v) {
    const auto& art=arts[static_cast<int>(v.chassis)];if(!art.loaded)return;
    if(v.wheels)v.wheels->setVisuals({});
    // Keep gameplay components and mounted equipment; replace chassis art only.
    auto oldMeshes=v.actor->getComponents<MeshComponent>();
    for(auto* mesh:oldMeshes) {
        if(mesh->parent()==v.actor && !mesh->name.starts_with("Pulse "))v.actor->removeChild(*mesh);
    }
    v.shell=nullptr;
    std::array<std::string,4> wheelIds;
    for(std::size_t i=0;i<art.meshes.size();++i) {
        auto& component=v.actor->addComponent<MeshComponent>(art.names[i],art.meshes[i]);
        component.color={1,1,1};
        static constexpr const char* wheels[]={"wheel_front_left","wheel_front_right","wheel_rear_left","wheel_rear_right"};
        bool wheel=false;
        for(int w=0;w<4;++w)if(art.names[i]==wheels[w]){wheelIds[w]=component.savedId();wheel=true;}
        if(!wheel&&!v.shell)v.shell=&component;
    }
    if(v.wheels){v.wheels->configure(art.definition);v.wheels->setVisuals(wheelIds);}
    if(&v==&racer(0))applyPaint(v);
    v.weapon->fit(static_cast<int>(v.chassis));
    // Fit once per art swap, including mounted equipment and resting wheels.
    // Geometry is immutable and never rebuilt during presentation.
    std::vector<Vec3> points;
    for(auto* mesh:v.actor->getComponents<MeshComponent>())if(mesh->parent()==v.actor) {
        auto transform=mesh->transform;
        if(v.wheels)for(int w=0;w<4;++w)if(mesh->savedId()==wheelIds[w]) {
            transform.position=art.definition->wheels[w].position;
            transform.position.y-=cybercar::restSuspension;
        }
        const auto matrix=transform.matrix();
        for(const auto& vertex:mesh->mesh->vertices)points.push_back(transformPoint(matrix,vertex.position));
    }
    const auto weaponPoints=v.weapon->boundsPoints();points.insert(points.end(),weaponPoints.begin(),weaponPoints.end());
    Vec3 lo=points.front(),hi=lo;
    for(auto p:points){lo={std::min(lo.x,p.x),std::min(lo.y,p.y),std::min(lo.z,p.z)};hi={std::max(hi.x,p.x),std::max(hi.y,p.y),std::max(hi.z,p.z)};}
    auto center=(lo+hi)*.5f,extent=(hi-lo)*.5f;
    Vec3 radii{extent.x+.25f,extent.y+.3f,extent.z+.3f};
    float growth=1;
    for(auto p:points){auto q=p-center;growth=std::max(growth,std::sqrt(q.x*q.x/(radii.x*radii.x)+q.y*q.y/(radii.y*radii.y)+q.z*q.z/(radii.z*radii.z)));}
    v.shieldEffect->fit(center,radii*(growth*1.08f));
    v.actor->name=chassisName(v.chassis);tune(v);
}
void StarterScene::tune(VehicleRuntime& v) {
    auto body=v.body->settings();
    float mass=v.chassis==Chassis::Bastion?1350.f:v.chassis==Chassis::Vektor?1150.f:v.chassis==Chassis::Vector?800.f:850.f;
    if(body.mass!=mass){body.mass=mass;v.body->configure(body);}
    if(!v.wheels)return;
    auto definition=std::make_shared<VehicleDefinition>(*v.wheels->definition());
    definition->engineTorque=(v.chassis==Chassis::Bastion?720.f:650.f)*v.upgrades.driveMultiplier();
    v.wheels->configure(definition);
}
void StarterScene::installWeaponArt(std::span<const std::byte> bytes) {
    if(weaponLoaded)throw std::logic_error("Weapon is already installed");
    GlbImportOptions options;options.maxTextureBytes=16*1024*1024;options.optimizeMeshes=false;
    auto mesh=GlbImporter::loadMemory(bytes,options);
    const auto& bounds=mesh->localBounds();
    if(!mesh->warnings.empty()||mesh->parts.size()!=1||mesh->materials.size()!=1||
       mesh->indices.size()!=weaponart::indices||!bounds.valid||
       magnitude(bounds.minimum-weaponart::minimum)>.01f||magnitude(bounds.maximum-weaponart::maximum)>.01f)
        throw std::runtime_error("Pulse weapon art contract mismatch");
    for(auto& v:vehicles)v.weapon->install(mesh);
    weaponLoaded=true;synchronize();
}
void StarterScene::installVehicleArt(Chassis chassis,std::span<const std::byte> bytes) {
    int slot=static_cast<int>(chassis);
    if(slot<1||slot>3||arts[slot].loaded)throw std::logic_error("Invalid or installed chassis");
    const auto& contract=vehicleart::models[slot==1?1:slot==2?0:2];
    GlbImportOptions options;options.optimizeMeshes=false;options.maxTextureBytes=16*1024*1024;
    auto imported=GlbImporter::loadMemory(bytes,options);
    if(imported->parts.size()!=contract.count||!imported->warnings.empty())throw std::runtime_error("Vehicle art contract mismatch");
    VehicleArt candidate;
    auto definition=std::make_shared<VehicleDefinition>(*vehicles[0].wheels->definition());
    for(unsigned i=0;i<contract.count;++i) {
        const auto& part=contract.parts[i];const auto& range=imported->parts[i];
        static constexpr const char* wheelNames[]={"wheel_front_left","wheel_front_right","wheel_rear_left","wheel_rear_right"};
        int wheelIndex=-1;
        for(int w=0;w<4;++w)if(std::string_view(part.name)==wheelNames[w])wheelIndex=w;
        if(range.indexCount!=part.indices)throw std::runtime_error("Vehicle part count mismatch");
        auto mesh=std::make_shared<Mesh>();mesh->materials={imported->materials.at(range.materialIndex)};
        std::unordered_map<std::uint32_t,std::uint32_t> remap;
        for(unsigned n=range.firstIndex;n<range.firstIndex+range.indexCount;++n) {
            auto source=imported->indices.at(n);auto [entry,inserted]=remap.emplace(source,static_cast<std::uint32_t>(mesh->vertices.size()));
            if(inserted){auto vertex=imported->vertices.at(source);if(wheelIndex>=0)vertex.position=vertex.position-part.pivot;mesh->vertices.push_back(vertex);}
            mesh->indices.push_back(entry->second);
        }
        mesh->parts.push_back({0,range.indexCount,0});
        if(wheelIndex>=0){auto& wheel=definition->wheels[wheelIndex];wheel.position=part.pivot+Vec3{0,contract.suspensionRest,0};
            wheel.suspensionMin=contract.suspensionMin;wheel.suspensionMax=contract.suspensionMax;
            wheel.radius=part.radius;wheel.width=part.width;}
        candidate.meshes.push_back(mesh);candidate.names.emplace_back(part.name);
    }
    definition->validate();candidate.definition=definition;candidate.loaded=true;arts[slot]=std::move(candidate);
    for(auto& v:vehicles)if(v.chassis==chassis)applyArt(v);
    if(chassis==Chassis::Bastion)cityground::installParked(*this,cityGround_,*imported);
    synchronize();resetGrid();
}
float StarterScene::heading() const {
    auto f=transformDirection(player().worldMatrix(),{0,0,1});
    return std::atan2(f.x,f.z);
}
void StarterScene::cancelInput(){
    input={};boosting=false;
    for(auto& v:vehicles){v.steering=0;v.boosting=false;if(v.wheels)v.wheels->cancel();}
}
void StarterScene::place(VehicleRuntime& v,int gate,float lane,float distance) {
    Transform pose;auto t=track.gateForward(gate);
    pose.position=track.gate(gate)+t*distance+Vec3{t.z,0,-t.x}*lane;
    pose.position.y=v.kind==VehicleKind::Car?.95f:1.15f;
    pose.rotation.y=std::atan2(t.x,t.z);v.body->teleport(pose);
    v.previous=pose.position;v.offTrack=0;v.stalled=0;v.steering=0;
    v.combat.disruption=0;v.combat.immunity=1.65f;v.combat.beamTime=0;v.combat.shield=false;
    v.shieldEffect->reset();
    v.beam->reset();
    if(v.wheels)v.wheels->cancel();
}
void StarterScene::placeAtGate(int gate) {
    cancelInput();
    place(racer(0),gate);
    synchronize();
    if(cameraRig)cameraRig->reset();
}
void StarterScene::resetGrid() {
    aiStates={};
    replay=false;newBest=false;recorder={};reference.reset();
    if(trialMode&&personalBest())reference=*personalBest();
    applyPaint(racer(0));
    vehicles[0].actor->enabled=kind==VehicleKind::Car;vehicles[1].actor->enabled=kind==VehicleKind::Hover;
    for(int id=1;id<racerCount;++id)racer(id).actor->enabled=true;
    synchronize(); // Teleports require registered bodies, including after replay/solo mode.
    ++audioEpoch;
    for(auto& v:vehicles){v.audioThrottle=0;v.audioImpacts=0;v.lastAudioImpact=-1;}
    cancelInput();race.reset();countdown=3;battery=100;reward=0;rewardPaid=false;
    for(int id=0;id<racerCount;++id) {
        auto& v=racer(id);v.progress.reset();v.battery=100;v.combat={};v.beam->reset();
        v.combat.interval=v.upgrades.shotInterval();v.combat.capacity=v.upgrades.shieldCapacity();
        v.combat.recharge=v.upgrades.shieldRecharge();v.combat.charge=v.combat.capacity;
        v.shieldEffect->reset();
        // Staggered two-wide rows; the player starts at the front centre.
        place(v,0,id==0?0:(id%2? -4.f:4.f),id==0?2.f:2.f-7.f*((id+1)/2));
    }
    for(int id=1;id<racerCount;++id)racer(id).actor->enabled=!trialMode;
    buildGhost();
    if(cameraRig) {
        CameraFollowSettings follow;follow.enabled=true;follow.target=player().savedId();
        follow.offset={0,4.8f,-10.5f};follow.focusOffset={0,.6f,2};follow.lookAheadSeconds=.18f;
        follow.positionHalfLife=.09f;follow.rotationHalfLife=.08f;follow.collision=false;camera->setFollow(follow);
    }
    synchronize();if(cameraRig)cameraRig->reset();
}
void StarterScene::select(VehicleKind selected) {
    selectChassis(selected==VehicleKind::Car?Chassis::Vektor:Chassis::Skimmer);
}
void StarterScene::selectChassis(Chassis chassis) {
    if(static_cast<int>(chassis)<0||static_cast<int>(chassis)>3)return;
    if(awaitingResults())return;
    const auto selected=chassisKind(chassis);
    career.selected=static_cast<int>(selected);
    career.chassis=static_cast<int>(chassis);
    restoreLeagueTuning();leagueRace_=false;difficulty_=garageDifficulty_;
    cancelInput();kind=selected;phase=Phase::Ready;
    auto& active=vehicles[static_cast<int>(kind)];
    if(active.chassis!=chassis){active.chassis=chassis;applyArt(active);}
    applyPaint(active);
    vehicles[0].actor->enabled=kind==VehicleKind::Car;vehicles[1].actor->enabled=kind==VehicleKind::Hover;
    for(auto& v:vehicles)v.beam->reset();
    synchronize();applyUpgrades();
    CameraFollowSettings follow;follow.enabled=true;follow.target=player().savedId();
    follow.offset={3.8f,2.8f,-6.8f};follow.focusOffset={0,.25f,0};follow.lookAheadSeconds=0;
    follow.positionHalfLife=.09f;follow.rotationHalfLife=.08f;follow.collision=false;
    camera->setFollow(follow);cameraRig->reset();present(0,16.f/9);
}
void StarterScene::start() {
    if(awaitingResults())return;
    if(leagueRace_&&phase!=Phase::Finished){startLeagueRace();return;}
    restoreLeagueTuning();leagueRace_=false;difficulty_=garageDifficulty_;
    applyUpgrades();
    CameraFollowSettings follow;follow.enabled=true;follow.target=player().savedId();
    follow.offset={0,4.8f,-10.5f};follow.focusOffset={0,.6f,2};follow.lookAheadSeconds=.18f;
    follow.positionHalfLife=.09f;follow.rotationHalfLife=.08f;follow.collision=false;
    camera->setFollow(follow);cameraRig->reset();phase=Phase::Countdown;
}
bool StarterScene::enterLeague(int league) {
    if(phase!=Phase::Ready)return false;
    return career.championship.enter(league);
}
bool StarterScene::startLeagueRace() {
    const auto& c=career.championship;
    if(awaitingResults()||!c.canRace()||c.track()!=static_cast<int>(track.kind)||
       (phase!=Phase::Ready&&!(leagueRace_&&phase!=Phase::Finished)))return false;
    if(trialMode)setTimeTrial(false);
    leagueRace_=true;leagueFinal_=c.isFinal();leagueRevision_=c.revision;
    difficulty_=static_cast<ai::Difficulty>(c.difficulty());
    applyUpgrades();
    CameraFollowSettings follow;follow.enabled=true;follow.target=player().savedId();
    follow.offset={0,4.8f,-10.5f};follow.focusOffset={0,.6f,2};follow.lookAheadSeconds=.18f;
    follow.positionHalfLife=.09f;follow.rotationHalfLife=.08f;follow.collision=false;
    camera->setFollow(follow);cameraRig->reset();phase=Phase::Countdown;return true;
}
bool StarterScene::setDifficulty(int value) {
    if(phase!=Phase::Ready||trialMode||value<0||value>2)return false;
    difficulty_=garageDifficulty_=static_cast<ai::Difficulty>(value);aiStates={};return true;
}
void StarterScene::restoreLeagueTuning() {
    for(int i=0;i<static_cast<int>(vehicles.size());++i)if(leagueDefinitions_[i]) {
        if(vehicles[i].wheels)vehicles[i].wheels->configure(leagueDefinitions_[i]);
        leagueDefinitions_[i].reset();
    }
}
void StarterScene::applyUpgrades() {
    for(int i=0;i<2;++i){vehicles[i].upgrades=trialMode?VehicleUpgrades{}:career.vehicles[i];
        if(leagueRace_)for(auto& rank:vehicles[i].upgrades.ranks)rank=std::min(rank,career.championship.rankCap());}
    for(int id=1;id<racerCount;++id){auto& v=racer(id);v.upgrades={};
        if(leagueRace_)v.upgrades.ranks.fill(career.championship.rankCap());}
    if(leagueRace_) {
        for(int i=0;i<static_cast<int>(vehicles.size());++i){auto& v=vehicles[i];
            if(v.wheels&&!leagueDefinitions_[i])leagueDefinitions_[i]=v.wheels->definition();
            tune(v);
        }
    } else tune(vehicles[0]);
    resetGrid();
}
bool StarterScene::purchase(int system) {
    if(trialMode||phase!=Phase::Ready||!career.buy(static_cast<int>(kind),system))return false;
    applyUpgrades();return true;
}
bool StarterScene::restoreProgression(std::string_view save) {
    if(phase!=Phase::Ready)return false;
    const auto restored=Progression::decode(save);if(!restored)return false;
    career=*restored;paintedTextures={};applyUpgrades();selectChassis(static_cast<Chassis>(career.chassis));return true;
}
void StarterScene::recover() {
    if(phase!=Phase::Racing||replay)return;
    if(trialMode)recorder.clean=false;
    race.recover();placeAtGate(race.lastGate());racer(0).progress=race;
}
DrivingInput StarterScene::pilot(int id,double dt) {
    const auto& v=racer(id);auto p=v.actor->transform.position;
    auto f=transformDirection(v.actor->worldMatrix(),{0,0,1});
    std::array<ai::Car,racerCount> traffic;
    for(int other=0;other<racerCount;++other){const auto& r=racer(other);
        traffic[other]={r.actor->transform.position,transformDirection(r.actor->worldMatrix(),{0,0,1}),r.body->linearVelocity(),r.actor->enabled};}
    auto& state=aiStates[id];
    // Recovery teleports invalidate the old passing commitment.
    if(v.offTrack>1.2||v.stalled>4)state={};
    auto planned=aiPlanner.update(id,traffic,state,difficulty_,float(dt));
    DrivingInput command;command.steer=planned.steer;command.throttle=planned.throttle;command.brake=planned.brake;
    // AI only shoots a visible forward target. Defense reacts to a nearby
    // attacker behind it, before shots resolve; it cannot inspect a future hit.
    command.fire=weaponTarget(id)>=0?1.f:0;
    for(int other=0;other<racerCount;++other)if(other!=id&&!racer(other).progress.finished) {
        auto d=p-racer(other).actor->transform.position;
        auto enemyForward=transformDirection(racer(other).actor->worldMatrix(),{0,0,1});
        if(magnitude(d)<32 && dot(normalized(d),enemyForward)>.9f && dot(d,f)>0 &&
            (v.combat.shield?v.combat.charge>0:v.combat.charge>35))command.shield=1;
    }
    return command;
}
int StarterScene::weaponTarget(int id) const {
    if(trialMode)return -1;
    const auto& shooter=racer(id);
    if(shooter.progress.finished)return -1;
    auto f=normalized(transformDirection(shooter.actor->worldMatrix(),{0,0,1}));
    auto origin=transformPoint(shooter.actor->worldMatrix(),shooter.weapon->muzzleLocal((shooter.combat.shots+1)%2));
    PhysicsQueryFilter filter;filter.excludedActors.push_back(shooter.actor->id());filter.includeSensors=false;
    int target=-1;float closest=CombatState::range;
    for(int other=0;other<racerCount;++other)if(other!=id&&!racer(other).progress.finished) {
        auto d=racer(other).actor->transform.position+Vec3{0,.15f,0}-origin;
        float length=magnitude(d);
        if(length<.1f || length>=closest || dot(d*(1/length),f)<.94f)continue;
        auto hit=physicsIfCreated()->raycast(origin,d*(1/length),length+1,filter);
        if(hit&&hit->actor==racer(other).actor->id()){target=other;closest=length;}
    }
    return target;
}
void StarterScene::firePulse(int id) {
    auto& shooter=racer(id);if(shooter.combat.cooldown>0)return;
    const int target=weaponTarget(id);
    if(!shooter.combat.fire())return;
    auto f=normalized(transformDirection(shooter.actor->worldMatrix(),{0,0,1}));
    const auto origin=transformPoint(shooter.actor->worldMatrix(),shooter.weapon->muzzleLocal(shooter.combat.shots%2));
    auto direction=target<0?f:normalized(racer(target).actor->transform.position+Vec3{0,.15f,0}-origin);
    PhysicsQueryFilter filter;filter.excludedActors.push_back(shooter.actor->id());filter.includeSensors=false;
    auto hit=physics().raycast(origin,direction,CombatState::range,filter);
    float length=hit?hit->distance:CombatState::range;
    auto impact=hit?LaserImpact::Surface:LaserImpact::None;
    auto normal=hit?hit->normal:direction*-1.f;
    if(hit)for(int other=0;other<racerCount;++other)if(other!=id&&!racer(other).progress.finished&&hit->actor==racer(other).actor->id()) {
        auto& victim=racer(other);
        impact=LaserImpact::Vehicle;
        if(victim.combat.shield) {
            impact=LaserImpact::Shield;
            auto local=inverseAffine(victim.actor->worldMatrix());
            float shieldDistance=victim.shieldEffect->impactRay(transformPoint(local,origin),transformDirection(local,direction));
            if(shieldDistance>=0)length=std::min(length,shieldDistance);
        }
        if(victim.combat.hit())++shooter.combat.hits;
        break;
    }
    // The same muzzle ray drives damage, cover and the visible contact point.
    shooter.beam->fire(origin,origin+direction*length,normal,impact,shooter.actor->worldMatrix());
}
void StarterScene::drive(VehicleRuntime& v,DrivingInput command,double dt) {
    v.audioThrottle=command.throttle;
    auto p=v.actor->transform.position;
    float delta=static_cast<float>(dt);
    v.steering+=(command.steer-v.steering)*(1-std::exp(-delta*9));
    auto f=transformDirection(v.actor->worldMatrix(),{0,0,1});f.y=0;f=normalized(f);
    Vec3 right{f.z,0,-f.x};auto velocity=v.body->linearVelocity();
    float forwardSpeed=dot(velocity,f),lateralSpeed=dot(velocity,right);
    if(v.kind==VehicleKind::Car) {
        // On the player car, holding the service brake selects reverse once the
        // forward roll is slow enough. X also brakes before changing direction.
        if(phase==Phase::Racing && !v.progress.finished && v.actor==&player() && command.brake>0) {
            if(forwardSpeed<=.8f) {command.throttle=-command.brake;command.brake=0;}
            else command.throttle=0;
        }
        if(command.throttle<0 && forwardSpeed>.8f) {
            command.brake=std::max(command.brake,-command.throttle);command.throttle=0;
        } else if(command.throttle>0 && forwardSpeed<-.8f) {
            command.brake=std::max(command.brake,command.throttle);command.throttle=0;
        }
    }
    if(v.combat.disruption>0) {
        command.throttle*=.35f;command.boost=0;
        v.body->addForce(f*(-forwardSpeed*(v.kind==VehicleKind::Car?650.f:480.f)));
    }
    v.boosting=phase==Phase::Racing&&command.boost>.5f&&command.throttle>0&&v.battery>1;
    v.battery=std::clamp(v.battery+delta*(v.boosting?-28.f:13.f),0.f,100.f);
    if(v.kind==VehicleKind::Car) {
        float throttle=command.throttle*std::clamp(((command.throttle>=0?(v.chassis==Chassis::Bastion?36.f:38.f)+v.upgrades.speedBonus():9.f)-std::abs(forwardSpeed))/5,0.f,1.f);
        // The chase camera looks along vehicle +Z: screen-right is cross(forward,up),
        // so a right turn decreases world yaw. Verify travel, not only command sign.
        v.wheels->submit({throttle,v.steering/(1+std::abs(forwardSpeed)*.035f),command.brake,command.slide});
        if(v.boosting)v.body->addForce(f*(9500*v.upgrades.driveMultiplier()));
    } else {
        float drive=command.throttle*(forwardSpeed<(v.chassis==Chassis::Vector?43:40)+v.upgrades.speedBonus()?
            (v.chassis==Chassis::Vector?10200.f:9500.f)*v.upgrades.driveMultiplier():0.f);
        if(command.throttle<0)drive=forwardSpeed> -9?command.throttle*6500:0;
        float lateralGrip=command.slide>.5f?.7f:2.2f;
        auto force=f*(drive-forwardSpeed*(command.brake>.5f?4200.f:180.f))-right*(lateralSpeed*850*lateralGrip);
        force.y=(1.15f-p.y)*26000-velocity.y*8500;
        if(v.boosting)force=force+f*(8000*v.upgrades.driveMultiplier());
        v.body->addForce(force);
        auto up=transformDirection(v.actor->worldMatrix(),{0,1,0});
        v.body->setAngularVelocity({-up.z*5,-v.steering*1.15f*std::clamp(std::abs(forwardSpeed)/8,.2f,1.f),up.x*5});
        if(v.shell)v.shell->transform.rotation.z=v.steering*.1f;
    }
}
sound::Frame StarterScene::audioFrame(bool paused) const {
    sound::Frame frame;frame.epoch=audioEpoch;frame.phase=static_cast<int>(phase);frame.countdown=countdown;frame.paused=paused;
    frame.activeRacers=trialMode?1:racerCount;frame.paused=paused||replay;
    auto cameraMatrix=camera->worldMatrix();
    frame.listener={transformPoint(cameraMatrix,{}),normalized(transformDirection(cameraMatrix,{0,0,-1})),normalized(transformDirection(cameraMatrix,{0,1,0}))};
    for(int i=0;i<racerCount;++i) {
        if(replay||!racer(i).actor->enabled)continue;
        const auto& v=racer(i);auto& r=frame.racers[i];auto velocity=v.body->linearVelocity();
        auto forward=normalized(transformDirection(v.actor->worldMatrix(),{0,0,1}));
        r.position=v.actor->transform.position;r.chassis=static_cast<unsigned>(v.chassis);
        r.speed=magnitude(velocity);r.throttle=v.audioThrottle;r.boost=v.boosting;
        r.rpm=v.wheels?v.wheels->state().rpm:1000;
        r.slip=std::abs(dot(velocity,Vec3{forward.z,0,-forward.x}));
        if(v.wheels)for(const auto& wheel:v.wheels->state().wheels)r.grounded=r.grounded||wheel.contact;
        r.shots=v.combat.shots;r.blocks=v.combat.blocks;r.received=v.combat.received;r.shieldUses=v.combat.shieldUses;
        r.impacts=v.audioImpacts;r.recoveries=v.progress.recoveries;
    }
    return frame;
}
void StarterScene::fixedStep(double dt) {
    worldTime+=dt;
    if(replay) {
        if(phase==Phase::Racing&&reference) {
            race.elapsed=std::min(reference->duration,race.elapsed+dt);
            for(int i=0;i<3;++i)if(race.elapsed>=reference->splits[i])recorder.run.splits[i]=reference->splits[i];
            if(race.elapsed>=reference->duration){race.finished=true;phase=Phase::Finished;}
        }
        return;
    }
    if(phase==Phase::Countdown){countdown-=dt;if(countdown<=0) {
        phase=Phase::Racing;
        if(trialMode)recorder.begin(static_cast<int>(track.kind),static_cast<int>(selectedChassis()),player().transform.position,player().transform.rotation);
    }}
    const int count=trialMode?1:racerCount;
    bool running=(phase==Phase::Racing || phase==Phase::Finished)&&!classificationFinal();
    std::array<DrivingInput,racerCount> orders{};
    // Update every defense before resolving any attack: grid index must not
    // grant a first-shot advantage against a shield activated on this tick.
    for(int id=0;id<count;++id) {
        auto& v=racer(id);
        if(running)orders[id]=id==0&&!v.progress.finished?input:pilot(id,dt);
        else orders[id].brake=1;
        if(trialMode){orders[id].fire=0;orders[id].shield=0;}
        v.combat.tick(static_cast<float>(dt),orders[id].shield>.5f,running&&!v.progress.finished);
    }
    for(int id=0;id<count;++id)
        if(running&&!racer(id).progress.finished&&orders[id].fire>.5f)firePulse(id);
    for(int id=0;id<count;++id) {
        auto& v=racer(id);auto p=v.actor->transform.position;
        v.shieldEffect->advance(static_cast<float>(dt),v.combat);
        if(running) {
            const auto before=v.progress;
            v.progress.step(track,v.previous,p,dt);v.previous=p;
            if(trialMode) {
                recorder.observe(before,v.progress,p,v.actor->transform.rotation);
                if(v.progress.completedLaps>=1||v.progress.elapsed>=TrialRun::maxSeconds) {
                    if(v.progress.completedLaps==0)recorder.clean=false;
                    v.progress.finished=true;
                    if(recorder.clean&&recorder.run.valid()&&(!personalBest()||v.progress.elapsed<personalBest()->duration)) {
                        bestRuns[static_cast<int>(track.kind)*4+static_cast<int>(selectedChassis())]=recorder.run;
                        newBest=true;++bestRevision;
                    }
                }
            }
            if(!v.progress.routeValid || p.y<-.5f || p.y>5 || transformDirection(v.actor->worldMatrix(),{0,1,0}).y<.25f)
                v.offTrack+=dt;
            else v.offTrack=0;
            if(id>0 && magnitude(v.body->linearVelocity())<1)v.stalled+=dt;else v.stalled=0;
        }
        drive(v,orders[id],dt);
    }
    race=racer(0).progress;battery=racer(0).battery;boosting=racer(0).boosting;
    if(race.finished && phase==Phase::Racing){phase=Phase::Finished;input={};}
    if(!trialMode&&phase==Phase::Finished&&classificationFinal()&&!rewardPaid) {
        const auto results=standings();Championship::Result positions{};
        for(const auto& r:results)positions[r.id]=r.position;
        // Commit points and money in the same career snapshot. No partial payout
        // if a stale scene result fails the championship revision guard.
        if(!leagueRace_||career.championship.record(leagueRevision_,positions))
            reward=career.award(positions[0]);
        rewardPaid=true;
    }
}
bool StarterScene::classificationFinal() const {
    if(trialMode)return race.finished;
    for(int id=0;id<racerCount;++id)if(!racer(id).progress.finished)return false;
    return true;
}
std::array<Standing,StarterScene::racerCount> StarterScene::standings() const {
    static constexpr const char* names[]={"YOU","AXIOM","NYX","KODA","ECHO","RIFT"};
    std::array<Standing,racerCount> result{};
    for(int id=0;id<racerCount;++id) {
        auto& v=racer(id);result[id]={id,0,names[id],v.kind,v.actor->transform.position,v.progress,v.combat};
    }
    std::stable_sort(result.begin(),result.end(),[&](const auto& a,const auto& b){
        if(a.progress.finished!=b.progress.finished)return a.progress.finished;
        if(a.progress.finished)return a.progress.elapsed<b.progress.elapsed;
        return a.progress.distanceScore(track,a.location)>b.progress.distanceScore(track,b.location);
    });
    for(int i=0;i<racerCount;++i)result[i].position=i+1;
    return result;
}
void StarterScene::present(double dt,float aspect) {
    presentGhost();
    trackKit.illuminate(*this,player().transform.position);
    // The host invokes presentation at an idle scene boundary after fixed steps.
    if(!replay&&(phase==Phase::Racing || (phase==Phase::Finished && !classificationFinal()))) {
        for(int id=0;id<(trialMode?1:racerCount);++id) {
            auto& v=racer(id);
            if(v.offTrack>1.2 || v.stalled>4) {
                if(id==0&&!v.progress.finished)recover();
                else {
                    if(!v.progress.finished)v.progress.recover();
                    place(v,v.progress.lastGate(),id%2?-4.f:4.f);
                    aiStates[id]={};
                    synchronize();
                }
            }
        }
    }
    gateMarker.present(track,race,!replay&&(phase==Phase::Countdown||phase==Phase::Racing),
        isDesert()?desertRaceKit.markerMountHeight():trackKit.markerMountHeight(race.nextGate));
    for(int id=0;id<racerCount;++id) {
        auto& v=racer(id);
        v.weapon->present(v.combat,v.upgrades.ranks[1]);
    }
    if(cameraRig)cameraRig->update(dt,aspect);
    for(int id=0;id<racerCount;++id) {
        auto& v=racer(id);
        v.shieldEffect->present(camera->transform.position);
        v.beam->present(v.combat.beamTime,camera->transform.position,physics().presentationMatrix(*v.actor));
    }
}
}
