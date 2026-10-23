#include "starter_scene.hpp"
#include "static_art.hpp"
#include <algorithm>
#include <stdexcept>

namespace racing {
using namespace engine;
namespace {
float radius(unsigned model) {
    auto d=citykit::models[model].dimensions;
    return std::hypot(d.x,d.z)*.5f;
}
float horizontalDistance(Vec3 a,Vec3 b) {a.y=b.y=0;return magnitude(a-b);}
}
bool StarterScene::allCityKitLoaded() const {
    return std::all_of(cityKitLoaded.begin(),cityKitLoaded.end(),[](bool loaded){return loaded;});
}
void StarterScene::planCityKit() {
    auto add=[&](unsigned model,Vec3 position,float yaw=0.f) {
        if(model<=4&&plaza::contains(track,position))position.y=plaza::surfaceHeight;
        cityDetails.push_back({model,position,yaw,radius(model)});
    };
    // A connected service access assembly beneath the elevated transit line.
    // Stair landing is at its -X end, ~4m high; catwalk's long axis is local Z.
    add(8,{35,0,25});
    add(9,{27,3.65f,25},pi*.5f);
    add(10,{20,4,23});
    add(5,{20,4,28});
    auto clear=[&](Vec3 p,float r) {
        if(track.nearest(p).distance<r+14.5f)return false;
        for(const auto& b:buildings)if(horizontalDistance(p,b.position)<r+b.radius+1.f)return false;
        for(const auto& b:cityDetails)if(horizontalDistance(p,b.position)<r+b.radius+1.f)return false;
        return true;
    };
    // Alternate silhouettes at four deliberately spaced skyline locations.
    for(auto [model,sample]:std::array<std::pair<unsigned,int>,4>{{{6,20},{7,76},{6,116},{7,166}}}) {
        auto outward=track.right(sample);
        auto p=track.point(sample)+outward*(radius(model)+30.f);
        for(int step=0;step<70&&!clear(p,radius(model));++step)p=p+outward*2.f;
        if(!clear(p,radius(model)))throw std::logic_error("Skyline placement overlaps reserved space");
        add(model,p,model==6?pi*.2f:-pi*.15f);
    }
    // Market bend and opposite plaza: sparse grouped dressing, never on asphalt.
    // Candidate offsets use full rotated-footprint circles, not only origins.
    constexpr std::pair<unsigned,int> requests[]={{0,6},{1,18},{0,102},{1,138},
        {2,42},{3,44},{2,126},{3,130},{4,8},{4,10},{4,104},{4,106}};
    for(auto [model,sample]:requests) {
        auto outward=track.right(sample),along=track.tangent(sample);
        bool placed=false;
        for(float offset=14.5f+radius(model);offset<65&&!placed;offset+=3.f) {
            for(float shift:{0.f,4.f,-4.f,8.f,-8.f}) {
                auto p=track.point(sample)+outward*offset+along*shift;
                if(!clear(p,radius(model)))continue;
                // Provider storefront fronts point +X; face the race corridor.
                auto front=outward*-1.f;
                add(model,p,std::atan2(-front.z,front.x));placed=true;break;
            }
        }
        if(!placed)throw std::logic_error("No clear city-kit placement");
    }
    // Replace the former support box after the original placements are planned,
    // so this intentionally connected assembly does not displace nearby props.
    add(11,{20,0,25});
}
void StarterScene::buildCityKitSupports() {
    auto& structure=spawn("Transit service access supports");
    auto part=[&](const char* name,Vec3 p,Vec3 size) {
        auto& mesh=structure.addComponent<MeshComponent>(name,Mesh::cube());
        mesh.transform.position=p;mesh.transform.scale=size;mesh.color={.18f,.21f,.26f};
        MaterialInstance material;material.roughness=.8f;mesh.setMaterial(0,material);
    };
    part("Catwalk outer support",{23.3f,1.85f,25},{.4f,3.7f,2.8f});
    part("Catwalk landing support",{30.3f,1.85f,25},{.4f,3.7f,2.8f});
}
void StarterScene::installCityKit(unsigned model,std::span<const std::byte> bytes) {
    if(model>=cityKitLoaded.size()||cityKitLoaded[model])throw std::logic_error("Invalid or installed city-kit model");
    const auto& contract=citykit::models[model];
    GlbImportOptions options;options.maxTextureBytes=16*1024*1024;
    auto mesh=GlbImporter::loadMemory(bytes,options);
    const auto& bounds=mesh->localBounds();
    if(!mesh->warnings.empty()||mesh->parts.size()!=contract.parts||mesh->materials.size()!=contract.materials||
       mesh->indices.size()!=contract.triangles*3||!bounds.valid||
       magnitude((bounds.maximum-bounds.minimum)-contract.dimensions)>.02f||std::abs(bounds.minimum.y)>.01f)
        throw std::runtime_error("City-kit model contract mismatch");
    mesh=art::prepareStatic(*mesh);
    std::vector<std::unique_ptr<Actor>> staged;
    for(const auto& p:cityDetails)if(p.model==model) {
        auto actor=std::make_unique<Actor>(contract.name);
        actor->transform.position=p.position;actor->transform.rotation.y=p.yaw;
        actor->transform.scale={p.scale,p.scale,p.scale};
        auto& visual=actor->addComponent<MeshComponent>("City detail",mesh);
        visual.allowInstanceReordering=true;
        // Decorative assets are behind the continuous track barriers.
        staged.push_back(std::move(actor));
    }
    for(auto& actor:staged)adoptChild(std::move(actor));
    cityKitLoaded[model]=true;synchronize();
}
}
