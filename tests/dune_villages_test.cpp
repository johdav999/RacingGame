#include "dune_villages.hpp"
#include "starter_scene.hpp"
#include "desert_solar.hpp"
#include "desert_surface.hpp"
#include "engine/glb_importer.hpp"
#include "engine/mesh_component.hpp"
#include "engine/physics.hpp"
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
using namespace racing;
using namespace engine;
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(){try{
    Circuit track(TrackKind::DesertSolar);dune::Kit kit;kit.plan(track);
    desert::Kit landscape;landscape.plan(track);Scene scene;
    for(bool stone:{false,true}){auto material=desert::surfaceMaterial(stone);
        require(material->baseColorTexture&&material->normalTexture&&material->metallicRoughnessTexture,"Desert surfaces supply albedo, normal and roughness");
        const auto& pixels=material->normalTexture->pixels;bool relief=false;
        for(std::size_t i=0;i<pixels.size();i+=4)if(std::abs(int(pixels[i])-128)>8)relief=true;
        require(relief,"Surface normals contain real relief");
    }
    unsigned operations=0;
    for(const auto& p:landscape.placements())if(p.model==12){++operations;require(track.nearest(p.position).distance>20,"Operations house outside driving envelope");}
    require(operations==1,"Exactly one requested operations house");
    {
        Scene operationsScene;std::ifstream file(std::string(RACING_ASSET_DIR)+"/models/desert-solar/solar-operations/solar-operations.glb",std::ios::binary|std::ios::ate);
        require(bool(file),"Operations runtime model exists");std::vector<std::byte> bytes(static_cast<std::size_t>(file.tellg()));file.seekg(0);file.read(reinterpret_cast<char*>(bytes.data()),bytes.size());
        landscape.install(operationsScene,12,bytes);require(operationsScene.children().size()==1,"Operations asset contract imports one house");
        require(bool(operationsScene.physics().raycast({40,20,-131},{0,-1,0},25)),"Operations house has grounded collision");
    }
    require(kit.placements().size()==12,"Two six-building villages");
    require(magnitude(dune::center(track,0)-dune::center(track,1))>200,"Villages separated along circuit");
    std::array<std::set<unsigned>,2> types;
    float minimumClearance=1000;
    for(const auto& p:kit.placements()) {
        types[p.village].insert(p.model);
        auto d=dune::models[p.model].dimensions;Transform t;t.position=p.position;t.rotation.y=p.yaw;
        for(int x=-4;x<=4;++x)for(int z=-4;z<=4;++z) {
            auto q=transformPoint(t.matrix(),{x*d.x/8,0,z*d.z/8});
            minimumClearance=std::min(minimumClearance,track.nearest(q).distance);
            // Ground flat across footprints, including the terrain grid vertices
            // used to interpolate the actual rendered triangles.
            float gx=std::floor(q.x/3.5f)*3.5f,gz=std::floor(q.z/3.5f)*3.5f;
            for(int dx=0;dx<2;++dx)for(int dz=0;dz<2;++dz)
                require(std::abs(landscape.height(track,gx+dx*3.5f,gz+dz*3.5f)-.002f)<.015f,"No floating or buried building footprint");
        }
        for(const auto& other:kit.placements())if(&p!=&other) {
            float ra=std::hypot(d.x,d.z)*.5f;
            auto od=dune::models[other.model].dimensions;
            require(magnitude(p.position-other.position)>ra+std::hypot(od.x,od.z)*.5f+1,"Buildings do not intersect");
        }
        for(const auto& other:landscape.placements()) {
            auto od=desert::models[other.model].dimensions*other.scale;
            require(magnitude(Vec3{p.position.x-other.position.x,0,p.position.z-other.position.z})>
                std::hypot(d.x,d.z)*.5f+std::hypot(od.x,od.z)*.5f,"Village clear of existing desert assets");
        }
    }
    require(types[0].size()==5&&types[1].size()==5,"All five designs appear in each village");
    require(minimumClearance>14,"Complete footprint clears road and crash barriers");
    StarterScene game(TrackKind::DesertSolar);
    bool foundationChecked=false;
    for(const auto& child:game.children())if(child->name=="Circuit foundation") {
        auto* a=dynamic_cast<Actor*>(child.get());auto* visual=a->getComponent<MeshComponent>();
        require(visual&&visual->transform.position.y<=-.1f,"Separate foundation visual from flat village terrain");
        require(std::abs(a->transform.position.y+.5f)<.001f,"Foundation physics position unchanged");
        foundationChecked=true;
    }
    require(foundationChecked,"Foundation checked");
    const std::array<std::byte,4> bad{};bool rejected=false;
    try{kit.install(scene,0,bad);}catch(const std::exception&){rejected=true;}
    require(rejected&&scene.children().empty()&&!kit.ready(),"Bad imports cannot partially publish");
    for(unsigned i=0;i<dune::models.size();++i) {
        std::string name=dune::models[i].name;
        std::ifstream f(std::string(RACING_ASSET_DIR)+"/models/dune-villages/"+name+"/"+name+".glb",std::ios::binary|std::ios::ate);
        require(bool(f),"Runtime GLB exists");std::vector<std::byte> bytes(static_cast<std::size_t>(f.tellg()));f.seekg(0);f.read(reinterpret_cast<char*>(bytes.data()),bytes.size());
        kit.install(scene,i,bytes);
    }
    require(kit.ready()&&scene.children().size()==12,"All buildings installed before readiness");
    std::array<const Mesh*,5> meshes{};
    for(const auto& child:scene.children()) {
        auto* a=dynamic_cast<Actor*>(child.get());auto* visual=a->getComponent<MeshComponent>();
        unsigned id=0;while(id<5&&visual->name!=dune::models[id].name)++id;
        require(id<5&&visual->allowInstanceReordering,"Imported named models with shared rendering");
        require(!meshes[id]||meshes[id]==visual->mesh.get(),"Instances share geometry and textures");meshes[id]=visual->mesh.get();
        require(bool(scene.physics().raycast(a->transform.position+Vec3{0,25,0},{0,-1,0},30)),"Building has solid collision");
    }
    for(int i=0;i<Circuit::samples;++i)for(float side:{-10.f,0.f,10.f})
        require(!scene.physics().raycast(track.point(i)+track.right(i)*side+Vec3{0,20,0},{0,-1,0},25),"Driving envelope clear of villages");
    std::cout<<"Dune villages: all five imports, twelve instances, shared resources, grounding, separation and collision passed. Minimum road-centre clearance "<<minimumClearance<<"m\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
