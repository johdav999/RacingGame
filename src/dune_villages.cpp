#include "dune_villages.hpp"
#include "static_art.hpp"
#include "engine/glb_importer.hpp"
#include "engine/mesh_component.hpp"
#include "engine/collider_component.hpp"
#include "engine/rigid_body_component.hpp"
#include <stdexcept>

namespace racing::dune {
using namespace engine;
void Kit::plan(const Circuit& track) {
    instances.clear();
    constexpr unsigned order[2][6]={{0,3,1,2,4,0},{3,0,2,4,1,3}};
    for(unsigned village=0;village<2;++village) {
        int sample=villageSamples[village];auto t=track.tangent(sample),out=track.right(sample);
        float yaw=std::atan2(-out.x,-out.z);
        for(unsigned i=0;i<6;++i) {
            auto p=track.point(sample)+t*((int(i%3)-1)*24.f)+out*(i<3?30.f:56.f);
            p.y=.012f;
            instances.push_back({order[village][i],village,p,yaw});
        }
    }
}
void Kit::install(Scene& scene,unsigned model,std::span<const std::byte> bytes) {
    if(model>=loaded.size()||loaded[model])throw std::logic_error("Invalid or installed dune building");
    const auto& contract=models[model];GlbImportOptions options;
    options.optimizeMeshes=false;options.maxTextureBytes=32*1024*1024;
    auto source=GlbImporter::loadMemory(bytes,options);const auto& b=source->localBounds();
    if(!source->warnings.empty()||source->parts.size()!=1||source->materials.size()!=contract.materials||
       source->indices.size()!=contract.triangles*3||!b.valid||
       magnitude(b.maximum-b.minimum-contract.dimensions)>.025f||std::abs(b.minimum.y)>.01f)
        throw std::runtime_error("Dune building contract mismatch");
    auto mesh=art::prepareStatic(*source);
    std::vector<std::unique_ptr<Actor>> staged;
    for(const auto& p:instances)if(p.model==model) {
        auto actor=std::make_unique<Actor>(p.village==0?"Sunward dune village":"Westwind dune village");
        actor->transform.position=p.position;actor->transform.rotation.y=p.yaw;
        auto& visual=actor->addComponent<MeshComponent>(contract.name,mesh);visual.allowInstanceReordering=true;
        visual.color={1,1,1}; // Imported materials already supply their authored colour.
        // Solid scenery outside the road barriers; no drivable building interiors.
        RigidBodySettings body;body.motion=BodyMotion::Static;
        actor->addComponent<RigidBodyComponent>("Village building",body);
        auto& collider=actor->addComponent<ColliderComponent>("Building footprint",CollisionShape::box(contract.dimensions*.5f));
        collider.transform.position={0,contract.dimensions.y*.5f,0};
        staged.push_back(std::move(actor));
    }
    for(auto& actor:staged)scene.adoptChild(std::move(actor));
    loaded[model]=true;scene.synchronize();
}
}
