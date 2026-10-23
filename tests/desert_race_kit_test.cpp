#include "starter_scene.hpp"
#include "engine/collider_component.hpp"
#include "engine/physics.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace racing;
using namespace engine;
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(){try{
    Circuit track(TrackKind::DesertSolar);Scene scene,fallbackScene;desertrace::Kit kit,fallback;kit.plan(track);fallback.plan(track);
    std::array<unsigned,2> counts{};
    float minimumClearance=1000;
    for(const auto& p:kit.placements()){
        ++counts[p.model];require(std::isfinite(p.yaw),"Finite placement rotation");
        if(p.model==0){
            auto d=desertrace::models[0].dimensions;Transform t;t.position=p.position;t.rotation.y=p.yaw;
            require(d.x*p.scale.x<=4.1f,"Repeat modules retain sane length");
            for(int x:{-1,0,1})for(int z:{-1,1}){
                auto point=transformPoint(t.matrix(),{x*d.x*p.scale.x*.5f,0,z*d.z*p.scale.z*.5f});
                minimumClearance=std::min(minimumClearance,track.nearest(point).distance);
            }
        }else require(magnitude(p.position-track.gate(p.gate))<.01,"Physical gates match ordered checkpoints");
    }
    std::cout<<"Minimum divider distance from centerline: "<<minimumClearance<<"m\n";
    require(minimumClearance>Circuit::halfWidth,"Dividers leave the entire 20m racing surface clear");
    require(counts[0]>=384&&counts[0]<=600&&counts[1]==12,"Complete edge coverage and twelve gates");
    const std::array<std::byte,4> bad{};bool rejected=false;
    try{kit.install(scene,0,bad);}catch(const std::exception&){rejected=true;}
    require(rejected&&scene.children().empty()&&!kit.ready(),"Failed import publishes no partial models");
    for(unsigned id=0;id<2;++id){
        std::string name=desertrace::models[id].name;
        std::ifstream f(std::string(RACING_ASSET_DIR)+"/models/desert-race-kit/"+name+"/"+name+".glb",std::ios::binary|std::ios::ate);
        require(bool(f),"Prepared GLB exists");std::vector<std::byte> bytes(static_cast<std::size_t>(f.tellg()));f.seekg(0);f.read(reinterpret_cast<char*>(bytes.data()),bytes.size());
        kit.install(scene,id,bytes);fallback.install(fallbackScene,id,bytes,false);rejected=false;try{kit.install(scene,id,bytes);}catch(const std::logic_error&){rejected=true;}
        require(rejected,"Duplicate imports rejected");
    }
    require(kit.ready(),"Both generated contracts loaded");
    require(kit.markerMountHeight()>10.5f&&kit.markerMountHeight()<10.7f&&
        kit.markerMountHeight()==fallback.markerMountHeight(),"Marker follows the scaled central beam on both backends");
    for(const auto& node:fallbackScene.children()){auto* a=dynamic_cast<const Actor*>(node.get());auto material=a->getComponent<MeshComponent>()->resolvedMaterial(0);require(magnitude(material.emissive)==0&&material.baseColorTexture!=nullptr,"Fallback keeps albedo without unmasked emission");}
    std::array<const Mesh*,2> meshes{};std::array<unsigned,2> actual{};
    for(const auto& node:scene.children()){
        const auto* actor=dynamic_cast<const Actor*>(node.get());if(!actor)continue;
        unsigned id=actor->name=="Helios crash divider"?0:1;auto* visual=actor->getComponent<MeshComponent>();
        require(visual&&visual->allowInstanceReordering,"Model participates in shared rendering");
        require(!meshes[id]||meshes[id]==visual->mesh.get(),"All repeated models share meshes and textures");meshes[id]=visual->mesh.get();++actual[id];
        require((actor->getComponent<ColliderComponent>()!=nullptr)==(id==1),"Only gates add new collision; divider uses existing track boundary");
    }
    require(counts==actual,"Every planned piece installed exactly once");
    for(int g=0;g<Circuit::gates;++g){auto t=track.gateForward(g),r=track.right(g*Circuit::samples/Circuit::gates);
        for(float x:{-10.f,0.f,10.f})for(float y:{.4f,1.5f,3.5f,6.5f})
            require(!scene.physics().raycast(track.gate(g)+r*x+Vec3{0,y,0}-t*4,t,8),"Open gantry collision clears cars, hovercraft and chase camera");
        require(bool(scene.physics().raycast(track.gate(g)+r*13.f+Vec3{0,.5f,0}-t*4,t,8)),"Gantry feet have actual collision");
    }
    StarterScene game(TrackKind::DesertSolar);unsigned barriers=0;
    for(const auto& node:game.children())if(node->name=="Crash barrier"){
        auto* a=dynamic_cast<const Actor*>(node.get());require(a->getComponent<ColliderComponent>()!=nullptr,"Original solid road envelope retained");
        require(a->getComponent<MeshComponent>()==nullptr,"Old desert barrier visuals removed");++barriers;
    }
    require(barriers==192&&!game.allDesertArtLoaded(),"Readiness waits for both new models and existing environment");
    std::cout<<"Desert race kit: "<<counts[0]<<" shared dividers, 12 open gates, physical clearance and original boundary verified\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
