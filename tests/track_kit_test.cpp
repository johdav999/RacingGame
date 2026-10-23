#include "starter_scene.hpp"
#include "engine/collider_component.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace racing;
using namespace engine;
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main() {
    StarterScene scene;trackkit::Kit kit;kit.plan(scene.track,scene.buildingPlacements());
    const auto initial=scene.children().size();const std::array<std::byte,4> invalid{};
    bool rejected=false;try{kit.install(scene,0,invalid);}catch(const std::exception&){rejected=true;}
    require(rejected&&!kit.ready()&&scene.children().size()==initial,"Invalid import must be atomic");
    std::array<unsigned,13> counts{};
    for(const auto& p:kit.placements()) {
        ++counts[p.model];auto d=trackkit::models[p.model].dimensions;
        if(p.model==3||p.model==4)continue; // The gantry opening is verified from its actual mesh below.
        for(int x=-2;x<=2;++x)for(int z=-2;z<=2;++z) {
            auto local=Vec3{d.x*p.scale.x*x/4,0,d.z*p.scale.z*z/4};
            Transform t;t.position=p.position;t.rotation.y=p.yaw;
            require(scene.track.nearest(transformPoint(t.matrix(),local)).distance>10.6f,"Track prop enters the racing envelope");
        }
        if(p.model==7)require(p.marking==-1||p.marking==1,"Every chevron has an explicit turn direction");
        if(p.model==8)require(p.marking==50||p.marking==100,"Braking board has a metric distance");
    }
    for(auto count:counts)require(count>0,"Every requested model must be placed");
    require(counts[0]==192&&counts[3]==1&&counts[4]==11,"Continuous walls and all twelve gates are dressed");
    for(unsigned id=0;id<13;++id) {
        std::string name=trackkit::models[id].name;
        std::ifstream file(std::string(RACING_ASSET_DIR)+"/models/track-kit/"+name+"/"+name+".glb",std::ios::binary|std::ios::ate);
        require(bool(file),"Missing prepared track asset");auto size=file.tellg();file.seekg(0);
        std::vector<std::byte> bytes(static_cast<std::size_t>(size));file.read(reinterpret_cast<char*>(bytes.data()),size);
        kit.install(scene,id,bytes);
        rejected=false;try{kit.install(scene,id,bytes);}catch(const std::logic_error&){rejected=true;}
        require(rejected,"Duplicate install rejected");
    }
    require(kit.ready(),"All track contracts imported");
    require(kit.markerMountHeight(0)>7.6f&&kit.markerMountHeight(0)<7.8f&&
        kit.markerMountHeight(1)>7.9f&&kit.markerMountHeight(1)<8.1f,"Finish and checkpoint expose their distinct actual beam tops");
    std::array<unsigned,13> installed{};std::array<const Mesh*,13> shared{};
    for(const auto& node:scene.children())for(unsigned id=0;id<13;++id)if(node->name==trackkit::models[id].name) {
        const auto* actor=dynamic_cast<const Actor*>(node.get());auto* visual=actor->getComponent<MeshComponent>();
        require(!shared[id]||shared[id]==visual->mesh.get(),"Repeated modules must share meshes and maps");
        shared[id]=visual->mesh.get();++installed[id];
        if(id==3||id==4)for(const auto& vertex:visual->mesh->vertices)
            require(vertex.position.y>4.f||std::abs(vertex.position.x)>11.8f,"Gantry posts must clear the racing envelope");
    }
    require(counts==installed,"Each placement installed once");
    kit.illuminate(scene,scene.track.point(0));kit.illuminate(scene,scene.track.point(96));
    unsigned lights=0;for(const auto& node:scene.children())if(node->name=="Nearest street lighting") {
        auto* actor=dynamic_cast<Actor*>(node.get());lights+=actor->getComponents<LightComponent>().size();
    }
    require(lights==6,"Street lighting stays below the eight-light renderer budget");
    unsigned barriers=0;
    for(const auto& node:scene.children())if(node->name=="Crash barrier") {
        auto* actor=dynamic_cast<const Actor*>(node.get());++barriers;
        require(actor->getComponent<ColliderComponent>()!=nullptr,"Crash barriers retain collision");
    }
    require(barriers==192,"Complete continuous barrier collision preserved");
    std::cout<<"Track kit: 13 asset contracts, "<<kit.placements().size()<<" placements, clearance, shared meshes, atomic import and lighting passed\n";
}
