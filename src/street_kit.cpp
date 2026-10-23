#include "street_kit.hpp"
#include "static_art.hpp"
#include "engine/glb_importer.hpp"
#include "engine/mesh_component.hpp"
#include "engine/rigid_body_component.hpp"
#include "engine/collider_component.hpp"

namespace racing::streetkit {
using namespace engine;
namespace { Vec3 scaled(Vec3 a,Vec3 b){return {a.x*b.x,a.y*b.y,a.z*b.z};} }
void Kit::plan(const Circuit& track,const std::vector<buildingart::Placement>& buildings) {
    if(!instances.empty())throw std::logic_error("Street kit already planned");
    auto add=[&](unsigned model,Vec3 p,float yaw=0,Vec3 scale=Vec3{1,1,1},bool collision=true){
        instances.push_back({model,p,yaw,scale,collision});
    };
    // One supported crossing at the station approach. Local X crosses the road.
    const auto center=track.point(56),across=track.right(56),along=track.tangent(56);
    const float yaw=std::atan2(-across.z,across.x);
    for(float x:{-34.f,-18.f,18.f,34.f})add(0,center+across*x,yaw+pi/2);
    constexpr float supports[]={-34,-18,18,34};
    const float beamHeight=models[1].dimensions.y;
    for(int i=0;i<3;++i) {
        const float width=supports[i+1]-supports[i];
        add(1,center+across*((supports[i+1]+supports[i])*.5f)+Vec3{0,8,0},yaw,{width/8,1,1});
    }
    for(int i=0;i<9;++i)add(2,center+across*(-32.f+i*8)+Vec3{0,8+beamHeight,0},yaw);
    // Station sits outside the crash barriers; its canopy is a braking landmark.
    add(3,center+across*(35.5f+models[3].dimensions.z*.5f),yaw-pi/2);
    for(const auto& b:buildings) {
        const auto d=buildingart::models[b.model].dimensions;
        const Vec3 front{std::cos(b.yaw),0,-std::sin(b.yaw)};
        const Vec3 right{front.z,0,-front.x};
        const float faceYaw=b.yaw+pi/2;
        if(b.model==0) {
            // Freestanding modules meet the original facade, with varied bays.
            for(int bay=-1;bay<=1;++bay) {
                const unsigned model=bay==0?5:4;
                // Overall building bounds include projecting canopies. The
                // ground-floor mounting wall is 1.45m inside that envelope.
                auto base=b.position+front*(d.x*.5f-1.45f+models[model].dimensions.z*.5f-.05f)+right*(bay*4.8f);
                add(model,base,faceYaw,{1,3.4f/models[model].dimensions.y,1});
                if(bay!=1)add(6,base+front*.65f+Vec3{0,3.45f,0},faceYaw,{1,1,1},false);
                add(7,base+Vec3{0,5.1f,0},faceYaw,{.9f,.9f,.9f},false);
            }
        } else if(b.model==1) {
            const auto frontage=b.position+front*(d.x*.5f+3.5f);
            for(float side:{-4.f,4.f})add(10,frontage+right*side,faceYaw);
            add(11,b.position+front*(d.x*.5f+1.2f)+right*8.f,faceYaw);
        }
    }
    // Closed side-street mouths and spectator areas remain behind crash barriers.
    for(int sample:{20,76,116,164}) {
        const auto front=track.right(sample),side=track.tangent(sample);
        const auto p=track.point(sample)+front*15.5f;
        const float facing=std::atan2(-side.z,side.x);
        for(int panel=-1;panel<=1;++panel)add(8,p+side*(panel*4.f),facing);
    }
    for(int sample:{4,90,140}) {
        const auto front=track.right(sample),side=track.tangent(sample);
        const auto p=track.point(sample)+front*13.8f;
        const float facing=std::atan2(-side.z,side.x);
        for(int panel=-2;panel<=2;++panel)add(9,p+side*(panel*4.f),facing);
    }
}
bool Kit::ready() const {return std::all_of(loaded.begin(),loaded.end(),[](bool value){return value;});}
bool Kit::overlapsSkyline(Vec3 p,float radius) const {
    for(const auto& instance:instances) {
        const auto d=scaled(models[instance.model].dimensions,instance.scale);
        auto delta=p-instance.position;delta.y=0;
        if(magnitude(delta)<radius+std::hypot(d.x,d.z)*.5f)return true;
    }
    return false;
}
void Kit::install(Scene& scene,unsigned model,std::span<const std::byte> bytes) {
    if(model>=loaded.size()||loaded[model])throw std::logic_error("Invalid or installed street model");
    const auto& expected=models[model];
    GlbImportOptions options;options.optimizeMeshes=false;options.maxTextureBytes=16*1024*1024;
    auto mesh=GlbImporter::loadMemory(bytes,options);
    const auto& bounds=mesh->localBounds();
    if(!mesh->warnings.empty()||mesh->indices.size()!=expected.triangles*3||!bounds.valid||
       magnitude(bounds.maximum-bounds.minimum-expected.dimensions)>.03f||std::abs(bounds.minimum.y)>.01f)
        throw std::runtime_error("Street asset contract mismatch");
    mesh=art::prepareStatic(*mesh);
    std::vector<std::unique_ptr<Actor>> staged;
    for(const auto& p:instances)if(p.model==model) {
        auto actor=std::make_unique<Actor>(expected.name);
        actor->transform.position=p.position;actor->transform.rotation.y=p.yaw;
        // Scale visuals and collider extents explicitly; physics actor stays unit scale.
        auto& visual=actor->addComponent<MeshComponent>("Kairox street module",mesh);
        visual.transform.scale=p.scale;visual.allowInstanceReordering=true;
        if(p.collision) {
            RigidBodySettings settings;settings.motion=BodyMotion::Static;
            actor->addComponent<RigidBodyComponent>("Street static body",settings);
            auto size=scaled(expected.dimensions,p.scale);
            auto& collision=actor->addComponent<ColliderComponent>("Street envelope",CollisionShape::box(size*.5f));
            collision.transform.position={0,size.y*.5f,0};
        }
        staged.push_back(std::move(actor));
    }
    for(auto& actor:staged)scene.adoptChild(std::move(actor));
    loaded[model]=true;scene.synchronize();
}
}
