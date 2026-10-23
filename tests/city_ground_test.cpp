#include "starter_scene.hpp"
#include "engine/collider_component.hpp"
#include "engine/water.hpp"
#include <iostream>
#include <fstream>
#include <set>
using namespace racing;
using namespace engine;
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main() {
    StarterScene city;
    const auto& r=city.cityGround();
    std::cout<<"segments="<<r.promenadeSegments<<" drains="<<r.drainSegments<<" foundations="<<r.foundations
        <<" docks="<<r.loadingDocks<<" clusters="<<r.utilityClusters<<" vans="<<r.serviceVehicles
        <<" contacts="<<r.contactShadows<<" pools="<<r.lightPools<<" damp="<<r.dampPatches
        <<" meshes="<<r.meshes<<" triangles="<<r.triangles<<std::endl;
    require(r.promenadeSegments==332&&r.drainSegments==332,"Promenades must join existing plaza sectors without overlapping them");
    require(r.foundations>=150&&r.loadingDocks==4&&r.utilityClusters==12,"Every street frontage and skyline must be grounded");
    require(r.serviceVehicles==3,"Three clear parked service vehicles");
    require(r.contactShadows>=170&&r.lightPools==40&&r.dampPatches>=12,"Bounded contact shading, actual lamp pools and damp accents");
    require(r.triangles<60000&&r.meshes<160,"City dressing must retain a bounded mesh and triangle budget");
    Actor* foundation=nullptr; for(const auto& node:city.children())if(node->name=="Circuit foundation")foundation=dynamic_cast<Actor*>(node.get());
    require(foundation&&foundation->getComponent<ColliderComponent>(),"Physical foundation retained");
    require(std::abs(foundation->transform.position.y+.5f)<.0001f&&
        foundation->getComponent<MeshComponent>()->transform.position.y<-.05f,"Foundation collider stays in place while its visual clears the city floor");
    std::set<const Texture*> pavingMaps;unsigned seen=0,triangles=0;
    float minimum=1000;unsigned low=0,high=0;
    for(const auto& node:city.children()) {
        auto* actor=dynamic_cast<Actor*>(node.get());if(!actor||!actor->name.starts_with("City ground /"))continue;
        require(!actor->getComponent<ColliderComponent>()&&!actor->getComponent<RigidBodyComponent>(),"Scenic dressing must not add driving collision");
        for(auto* m:actor->getComponents<MeshComponent>()) {
            ++seen;triangles+=m->mesh->indices.size()/3;
            auto material=m->resolvedMaterial(0);
            require(material.water.planarReflectionStrength==0,"Small damp accents must not request full city reflection passes");
            if(material.baseColorTexture&&(material.name.starts_with("Weathered")||material.name.starts_with("Promenade")))pavingMaps.insert(material.baseColorTexture.get());
            if(m->name=="City ground / opaque batch")for(const auto& v:m->mesh->vertices) {
                minimum=std::min(minimum,city.track.nearest(v.position).distance);
                require(std::isfinite(v.position.x)&&std::isfinite(v.position.y)&&std::isfinite(v.position.z),"Finite ground geometry");
                require(std::abs(magnitude(v.normal)-1)<.01f,"Normalized geometry normals");
                low+=v.position.y>.02f&&v.position.y<.06f;
                high+=v.position.y>.17f&&v.position.y<.20f;
            }
        }
    }
    std::cout<<"Minimum opaque dressing clearance "<<minimum<<std::endl;
    require(minimum>13.8f,"New opaque geometry must remain behind the crash barriers and imported sidewalk");
    require(low>100&&high>100,"Drains must be recessed beneath raised paving");
    require(seen==r.meshes&&triangles==r.triangles,"Reported resources match actual scene assembly");
    require(pavingMaps.size()==2,"All paving shares two immutable procedural material sources");
    for(const auto& v:r.views)require(magnitude(v.eye-v.target)>1,"Usable authoring views");
    require(!waterPlanarReflectionRequired(city),"Damp patches must not activate planar city captures");
    std::ifstream file(std::string(RACING_ASSET_DIR)+"/models/bastion/bastion.glb",std::ios::binary|std::ios::ate);
    require(bool(file),"Existing Bastion asset available");std::vector<std::byte> bytes(static_cast<std::size_t>(file.tellg()));file.seekg(0);file.read(reinterpret_cast<char*>(bytes.data()),bytes.size());
    city.installVehicleArt(Chassis::Bastion,bytes);
    unsigned parked=0;const Mesh* shared=nullptr;
    for(const auto& node:city.children())if(node->name=="City service / parked Bastion") {
        auto* actor=dynamic_cast<Actor*>(node.get());require(actor&&!actor->getComponent<ColliderComponent>()&&!actor->getComponent<RigidBodyComponent>(),"Parked service art stays scenic");
        auto* body=actor->getComponent<MeshComponent>();require(body&&(!shared||shared==body->mesh.get()),"Parked vehicles share immutable model/maps/LODs");
        require(!body->mesh->lods.empty(),"Static parked models have distance LODs");shared=body->mesh.get();++parked;
    }
    require(parked==3,"Validated existing vehicle import publishes three service vehicles");
    StarterScene desert(TrackKind::DesertSolar);
    require(desert.cityGround().meshes==0&&desert.cityGround().contactShadows==0,"City dressing must not leak into the desert track");
    std::cout<<"City ground: clearances, heights, material sharing, bounds, scene ownership and desert isolation passed\n";
}
