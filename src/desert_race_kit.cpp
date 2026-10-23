#include "desert_race_kit.hpp"
#include "static_art.hpp"
#include "gantry_mount.hpp"
#include "engine/glb_importer.hpp"
#include "engine/mesh_component.hpp"
#include "engine/rigid_body_component.hpp"
#include "engine/collider_component.hpp"
#include <stdexcept>
namespace racing::desertrace {
namespace { constexpr float gantryHeightScale=1.5f; }
using namespace engine;
void Kit::plan(const Circuit& track) {
    instances.clear();
    for(int i=0;i<Circuit::samples;i+=2)for(int side:{-1,1}) {
        auto a=track.point(i)+track.right(i)*(side*11.3f);
        auto b=track.point(i+2)+track.right(i+2)*(side*11.3f);
        auto delta=b-a;float length=magnitude(delta);
        int pieces=std::max(1,int(std::ceil(length/4.f)));
        float yaw=std::atan2(-delta.z,delta.x)+(side<0?pi:0);
        for(int j=0;j<pieces;++j){auto p=a+delta*((j+.5f)/pieces);p.y=.015f;
            instances.push_back({0,p,yaw,{(length/pieces+.08f)/models[0].dimensions.x,1.2f/models[0].dimensions.y,.65f/models[0].dimensions.z}});}
    }
    for(int g=0;g<Circuit::gates;++g){auto t=track.gateForward(g)*-1.f;
        instances.push_back({1,track.gate(g),std::atan2(t.x,t.z),{1,gantryHeightScale,1},g});}
}
void Kit::install(Scene& scene,unsigned model,std::span<const std::byte> bytes,bool emissiveMaps) {
    if(model>=loaded.size()||loaded[model])throw std::logic_error("Invalid or installed desert race model");
    const auto& contract=models[model];GlbImportOptions options;options.optimizeMeshes=false;options.maxTextureBytes=64*1024*1024;
    auto source=GlbImporter::loadMemory(bytes,options);const auto& b=source->localBounds();
    if(!source->warnings.empty()||source->parts.size()!=1||source->materials.size()!=contract.materials||
        source->indices.size()!=contract.triangles*3||!b.valid||magnitude(b.maximum-b.minimum-contract.dimensions)>.025f||std::abs(b.minimum.y)>.01f)
        throw std::runtime_error("Desert race model contract mismatch");
    const float mountingTop=model==1?gantryBeamTop(*source,gantryHeightScale):0;
    auto mesh=art::prepareStatic(*source);
    // Shared immutable geometry and LODs; dividers reuse the circuit's continuous
    // box collisions. Gates need an open mesh collider, never a solid arch box.
    std::vector<Vec3> vertices;if(model==1){vertices.reserve(source->vertices.size());for(const auto& v:source->vertices)vertices.push_back({v.position.x,v.position.y*gantryHeightScale,v.position.z});}
    std::vector<std::unique_ptr<Actor>> staged;
    for(const auto& p:instances)if(p.model==model){
        auto actor=std::make_unique<Actor>(model==0?"Helios crash divider":p.gate==0?"Helios finish gantry":"Helios checkpoint gantry");
        actor->transform.position=p.position;actor->transform.rotation.y=p.yaw;
        auto& visual=actor->addComponent<MeshComponent>("Tripo race infrastructure",mesh);visual.transform.scale=p.scale;visual.allowInstanceReordering=true;
        visual.color={1,1,1}; // Imported materials already supply their authored colour.
        // The pinned WebGL2 renderer adds the emissive factor without sampling its
        // mask. Keep the textured strips, but disable whole-surface emission there.
        if(!emissiveMaps){MaterialInstance material;material.emissive=Vec3{};visual.setMaterial(0,material);}
        if(model==1){RigidBodySettings settings;settings.motion=BodyMotion::Static;
            actor->addComponent<RigidBodyComponent>("Gantry structure",settings);
            actor->addComponent<ColliderComponent>("Open gantry collision",CollisionShape::triangleMesh(vertices,source->indices));}
        staged.push_back(std::move(actor));
    }
    for(auto& actor:staged)scene.adoptChild(std::move(actor));
    if(model==1)beamTop=mountingTop;
    loaded[model]=true;scene.synchronize();
}
}
