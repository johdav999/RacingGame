#include "starter_scene.hpp"
#include "street_kit.hpp"
#include "engine/physics.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace racing;
void require(bool yes,const char* message){if(!yes)throw std::runtime_error(message);}
int main(){
    StarterScene scene;
    streetkit::Kit kit;kit.plan(scene.track,scene.buildingPlacements());
    const auto initial=scene.children().size();
    bool invalidRejected=false;const std::array<std::byte,4> invalid{};
    try{kit.install(scene,0,invalid);}catch(const std::exception&){invalidRejected=true;}
    require(invalidRejected&&scene.children().size()==initial&&!kit.ready(),"Failed import must leave the scene and readiness unchanged");
    std::array<int,12> counts{};
    for(const auto& p:kit.placements()){
        ++counts[p.model];
        if(p.position.y>=7)continue;
        const auto d=streetkit::models[p.model].dimensions;
        // Sample the full rotated footprint, including the centre, conservatively.
        for(int x=-2;x<=2;++x)for(int z=-2;z<=2;++z){
            const auto local=Vec3{d.x*p.scale.x*x/4,0,d.z*p.scale.z*z/4};
            auto world=p.position+Vec3{local.x*std::cos(p.yaw)+local.z*std::sin(p.yaw),0,-local.x*std::sin(p.yaw)+local.z*std::cos(p.yaw)};
            require(scene.track.nearest(world).distance>11.7f,"Street asset intrudes into the crash-barrier envelope");
        }
    }
    for(auto count:counts)require(count>0,"Every requested module must be used");
    for(unsigned id=0;id<streetkit::models.size();++id){
        const std::string name=streetkit::models[id].name;
        std::ifstream file(std::string(RACING_ASSET_DIR)+"/models/street-kit/"+name+"/"+name+".glb",std::ios::binary|std::ios::ate);
        require(bool(file),"Prepared model missing");
        auto size=file.tellg();file.seekg(0);std::vector<std::byte> bytes(static_cast<std::size_t>(size));
        file.read(reinterpret_cast<char*>(bytes.data()),size);
        kit.install(scene,id,bytes);
        bool rejected=false;try{kit.install(scene,id,bytes);}catch(const std::logic_error&){rejected=true;}
        require(rejected,"Duplicate install must be rejected");
    }
    require(kit.ready(),"All 12 street assets imported successfully");
    std::array<const engine::Mesh*,12> shared{};std::array<int,12> installed{};
    for(const auto& actor:scene.children())for(unsigned id=0;id<12;++id)if(actor->name==streetkit::models[id].name){
        const auto* a=dynamic_cast<const engine::Actor*>(actor.get());
        require(a!=nullptr,"Street instance must be an actor");
        auto* visual=a->getComponent<engine::MeshComponent>();require(visual!=nullptr,"Street instance must have art");
        require(!shared[id]||shared[id]==visual->mesh.get(),"Repeated street modules must share geometry and maps");
        shared[id]=visual->mesh.get();++installed[id];
    }
    require(counts==installed,"All planned modules are installed once");
    // Actual physics ray confirms overhead structure and free vehicle space.
    const auto crossing=scene.track.point(56);
    auto hit=scene.physics().raycast(crossing+Vec3{0,2,0},{0,1,0},20);
    require(hit.has_value()&&hit->distance>5.5f,"Underpass must have real overhead structure and vehicle clearance");
    std::cout<<"Street kit: 12 model contracts, "<<kit.placements().size()<<" placements, race clearance, duplicates and underpass physics passed\n";
}
