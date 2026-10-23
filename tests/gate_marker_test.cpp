#include "gate_marker.hpp"
#include "gantry_mount.hpp"
#include "engine/collider_component.hpp"
#include "engine/mesh_component.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
using namespace racing;
using namespace engine;
void require(bool okay,const char* message){if(!okay)throw std::runtime_error(message);}
std::shared_ptr<const Mesh> loadGantry(const std::string& relative) {
    std::ifstream file(std::string(RACING_ASSET_DIR)+"/models/"+relative,std::ios::binary|std::ios::ate);
    require(bool(file),"Gantry reference exists");
    std::vector<std::byte> bytes(static_cast<std::size_t>(file.tellg()));file.seekg(0);file.read(reinterpret_cast<char*>(bytes.data()),bytes.size());
    return GlbImporter::loadMemory(bytes);
}
void gateMarkerRules(TrackKind kind) {
    Scene scene;GateMarker marker;Circuit circuit(kind);RaceProgress race;
    const bool desert=kind==TrackKind::DesertSolar;
    const auto checkpoint=loadGantry(desert?"desert-race-kit/desert-race-gate/desert-race-gate.glb":"track-kit/kairox-checkpoint-gantry/kairox-checkpoint-gantry.glb");
    const auto finish=desert?checkpoint:loadGantry("track-kit/kairox-finish-gantry/kairox-finish-gantry.glb");
    const std::array<std::byte,4> invalid{};bool rejected=false;
    try{marker.install(scene,invalid);}catch(const std::exception&){rejected=true;}
    require(rejected&&!marker.ready()&&scene.children().empty(),"Bad beacon import must be atomic and retryable");
    std::ifstream file(std::string(RACING_ASSET_DIR)+"/models/navigation/next-gate-beacon/next-gate-beacon.glb",std::ios::binary|std::ios::ate);
    require(bool(file),"Prepared beacon exists");
    std::vector<std::byte> bytes(static_cast<std::size_t>(file.tellg()));file.seekg(0);file.read(reinterpret_cast<char*>(bytes.data()),bytes.size());
    marker.install(scene,bytes);require(marker.ready()&&scene.children().size()==1,"Exactly one beacon installed");
    const auto* actor=marker.actor();require(!actor->enabled,"Beacon hidden until racing starts");
    require(actor->getComponents<ColliderComponent>().empty(),"Beacon cannot obstruct the race");
    const auto mesh=actor->getComponents<MeshComponent>()[0]->mesh;
    require(mesh->materials.size()==2,"Housing and light must be separate materials for WebGL2");
    int luminous=0;
    for(const auto& m:mesh->materials) {
        if(magnitude(m->emissive)>.1f)++luminous;
        require(!m->emissiveTexture&&!m->baseColorTexture,"Precision sign uses geometry without texture masks or atlases");
    }
    require(luminous==1,"Only amber faces emit light; housing stays dark");
    auto verify=[&] {
        const float beam=gantryBeamTop(race.nextGate==0?*finish:*checkpoint,desert?1.5f:1.f);
        marker.present(circuit,race,true,beam);
        require(std::abs(marker.mountHeight()-(beam-.25f))<.001f,"Feet embed in the actual beam, not a fixed road height");
        require(marker.mountHeight()+.65f>beam,"Sign casing clears the beam top");
        require(actor->getComponents<MeshComponent>()[1]->transform.position.y<.1f,"Road brackets remain grounded");
        require(magnitude(actor->transform.position-circuit.gate(race.nextGate))<.001f,"Beacon follows authoritative gate");
        Vec3 facing{std::sin(actor->transform.rotation.y),0,std::cos(actor->transform.rotation.y)};
        require(dot(facing,circuit.gateForward(race.nextGate))>.999f,"Beacon faces approaching traffic");
    };
    auto g=circuit.gate(1),f=circuit.gateForward(1);
    race.step(circuit,g+f,g-f,.1);verify();require(race.nextGate==1,"Reverse crossing cannot move beacon");
    race.step(circuit,circuit.gate(4),circuit.gate(4)+circuit.gateForward(4),.1);verify();
    require(race.nextGate==1,"Skipping cannot move beacon");race.recover();verify();require(race.nextGate==1,"Recovery preserves target");
    race.reset();Vec3 previous=circuit.gate(0)+circuit.gateForward(0)*2;
    std::array<bool,12> seen{};
    for(int lap=0;lap<3;++lap)for(int i=1;i<=Circuit::samples;++i) {
        auto next=circuit.point(i);race.step(circuit,previous,next,1./60);previous=next;
        verify();seen[race.nextGate]=true;
    }
    race.step(circuit,previous,circuit.gate(0)+circuit.gateForward(0)*2,1./60);verify();
    require(std::all_of(seen.begin(),seen.end(),[](bool v){return v;}),"All twelve beacon targets traversed");
    require(race.finished&&!actor->enabled,"Finished race hides the beacon");
    race.reset();verify();require(actor->enabled&&race.nextGate==1,"Restart resets the target");
    marker.present(circuit,race,true,0);require(!actor->enabled,"Unloaded gantry cannot display a floating sign");
    marker.present(circuit,race,false,8);require(!actor->enabled,"Garage hides beacon");
    rejected=false;try{marker.install(scene,bytes);}catch(const std::exception&){rejected=true;}
    require(rejected&&scene.children().size()==1,"Duplicate beacon install preserves the original");
    std::cout<<"Beacon imports, retry, all gates, reverse, recovery, finish and restart passed\n";
}
int main(){try{gateMarkerRules(TrackKind::Kairox);gateMarkerRules(TrackKind::DesertSolar);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
