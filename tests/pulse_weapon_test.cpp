#include "starter_scene.hpp"
#include "weapon_art_contract.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace racing;
using namespace engine;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::vector<std::byte> read(const char* relative) {
    std::ifstream in(std::string(RACING_ASSET_DIR)+relative,std::ios::binary|std::ios::ate);
    check(bool(in),"Asset exists");std::vector<std::byte> bytes(static_cast<std::size_t>(in.tellg()));
    in.seekg(0);in.read(reinterpret_cast<char*>(bytes.data()),bytes.size());return bytes;
}
int main(){try {
    StarterScene scene;
    try{scene.installWeaponArt({});check(false,"Bad weapon must be rejected");}catch(const std::exception&){}
    check(!scene.weaponArtLoaded(),"Failed import cannot publish readiness");
    auto bytes=read("/models/weapons/pulse-cannon/pulse-cannon.glb");scene.installWeaponArt(bytes);
    check(scene.weaponArtLoaded(),"Weapon installs");
    bool rejected=false;try{scene.installWeaponArt(bytes);}catch(const std::exception&){rejected=true;}
    check(rejected,"Duplicate install rejected");
    scene.installPlayerCar(read("/models/cybercar-clean-detail/cybercar.glb"));
    scene.installVehicleArt(Chassis::Bastion,read("/models/bastion/bastion.glb"));
    scene.installVehicleArt(Chassis::Skimmer,read("/models/skimmer/skimmer.glb"));
    scene.installVehicleArt(Chassis::Vector,read("/models/vector/vector.glb"));
    std::shared_ptr<const Mesh> shared;
    for(auto chassis:{Chassis::Vektor,Chassis::Bastion,Chassis::Skimmer,Chassis::Vector,Chassis::Vektor}) {
        scene.selectChassis(chassis);scene.present(0,16.f/9);
        auto* weapon=scene.player().getComponent<PulseWeapon>();check(weapon&&weapon->loaded(),"Chassis switch preserves assembly");
        int modules=0;
        for(auto* m:scene.player().getComponents<MeshComponent>())if(m->name=="Pulse cannon module") {
            ++modules;if(!shared)shared=m->mesh;
            check(m->mesh==shared,"Instances share geometry and materials across chassis");
            check(m->mesh->indices.size()==weaponart::indices&&weaponart::indices/3<=6000,"Equipment budget");
            check(m->mesh->materials[0]->baseColorTexture&&m->mesh->materials[0]->normalTexture&&m->mesh->materials[0]->metallicRoughnessTexture,"PBR maps retained");
        }
        check(modules==2,"Exactly two modules after repeated swaps");
        auto left=weapon->muzzleLocal(0),right=weapon->muzzleLocal(1);
        check(left.x<0&&right.x>0&&left.z>1.5f&&std::abs(left.y-right.y)<.001f,"Forward symmetric hardpoints");
        // Check the fitted support against actual chassis triangles, not a stub.
        const float scale=chassisKind(chassis)==VehicleKind::Hover?.78f:1.f;
        for(float x:{left.x,right.x}) {
            float surface=-100;
            for(auto* m:scene.player().getComponents<MeshComponent>())if(m->parent()==&scene.player()&&!m->name.starts_with("wheel")) {
                const auto matrix=m->transform.matrix();
                for(std::size_t i=0;i<m->mesh->indices.size();i+=3) {
                    auto a=transformPoint(matrix,m->mesh->vertices[m->mesh->indices[i]].position);
                    auto b=transformPoint(matrix,m->mesh->vertices[m->mesh->indices[i+1]].position);
                    auto c=transformPoint(matrix,m->mesh->vertices[m->mesh->indices[i+2]].position);
                    const float denominator=(b.z-c.z)*(a.x-c.x)+(c.x-b.x)*(a.z-c.z);
                    if(std::abs(denominator)<1e-8f)continue;
                    const float u=((b.z-c.z)*(x-c.x)+(c.x-b.x)*(1.15f-c.z))/denominator;
                    const float v=((c.z-a.z)*(x-c.x)+(a.x-c.x)*(1.15f-c.z))/denominator;
                    if(u>=0&&v>=0&&u+v<=1)surface=std::max(surface,u*a.y+v*b.y+(1-u-v)*c.y);
                }
            }
            const float base=left.y-.18f*scale;
            std::cout<<chassisName(chassis)<<" hardpoint "<<x<<" surface "<<surface<<" base "<<base<<'\n';
            check(surface>=base-.085f*scale-.025f&&surface<=base+.025f,"Mount shoe touches chassis without burying the cannon");
        }
        CombatState combat;combat.fire();weapon->present(combat,3);
        check(weapon->rank()==3&&weapon->recoil()>.04f,"Upgrade indicators and firing recoil");
        int flashes=0,lights=0;
        for(auto* m:scene.player().getComponents<MeshComponent>()){
            if(m->name=="Pulse muzzle flash"&&m->enabled)++flashes;
            if(m->name=="Pulse upgrade indicator"&&m->enabled)++lights;
        }
        check(flashes==1&&lights==6,"One firing aperture and three indicators per mount");
        combat.tick(.2f,false,true);weapon->present(combat,0);
        check(weapon->recoil()==0,"Recoil expires with simulation timer");
        for(auto* m:scene.player().getComponents<MeshComponent>())if(m->name=="Pulse muzzle flash"||m->name=="Pulse upgrade indicator")check(!m->enabled,"Reset/stock clears transient and upgrade visuals");
    }
    check(scene.restoreProgression("NA2 5000 1 0 0 0 0 0 0 0 0"),"Restore career");
    for(int rank=1;rank<=3;++rank){check(scene.purchase(1),"Cannon purchase");scene.present(0,1);check(scene.player().getComponent<PulseWeapon>()->rank()==rank,"Purchase updates actual model indicators");}
    scene.start();check(!scene.purchase(1),"No purchases during race");
    scene.selectChassis(Chassis::Skimmer);scene.present(0,1);check(scene.player().getComponent<PulseWeapon>()->rank()==0,"Hover keeps its own ranks");
    std::cout<<"Pulse weapon asset, materials, sharing, mounts, lifecycle and purchases passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
