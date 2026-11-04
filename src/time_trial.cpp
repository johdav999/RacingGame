#include "starter_scene.hpp"
#include "engine/physics.hpp"
#include "cybercar_contract.hpp"

namespace racing {
const TrialRun* StarterScene::personalBest() const {
    const auto& best=bestRuns[static_cast<int>(track.kind)*4+static_cast<int>(selectedChassis())];
    return best?&*best:nullptr;
}
bool StarterScene::restoreTrial(int track,int chassis,std::string_view bytes) {
    if(phase!=Phase::Ready||track<0||track>=trackCount||chassis<0||chassis>3)return false;
    auto candidate=TrialRun::decode(bytes,track,chassis);if(!candidate)return false;
    auto& best=bestRuns[track*4+chassis];
    if(!best||candidate->duration<best->duration)best=std::move(candidate);
    return true;
}
void StarterScene::setTimeTrial(bool enabled) {
    if(phase!=Phase::Ready)return;
    trialMode=enabled;applyUpgrades();selectChassis(selectedChassis());
}
void StarterScene::buildGhost() {
    if(ghost){removeChild(*ghost);ghost=nullptr;}
    if(!trialMode||!reference)return;
    ghost=&spawn("Personal best ghost");ghost->enabled=false;
    auto material=std::make_shared<engine::Material>();
    material->shader=engine::MaterialShader::Unlit;material->baseColor={.12f,1.f,.9f};
    material->alphaMode=engine::AlphaMode::Blend;
    const auto& art=arts[static_cast<int>(selectedChassis())];
    if(art.loaded)for(std::size_t i=0;i<art.meshes.size();++i) {
        auto& mesh=ghost->addComponent<engine::MeshComponent>(art.names[i],art.meshes[i]);
        mesh.color={1,1,1};mesh.opacity=.32f;mesh.blend=engine::BlendMode::Alpha;
        mesh.castsShadow=false;mesh.receivesShadow=false;mesh.receivesDecals=false;
        for(std::size_t slot=0;slot<std::max(std::size_t{1},art.meshes[i]->materials.size());++slot)
            mesh.setMaterial(slot,engine::MaterialInstance{material});
        if(art.definition)for(int w=0;w<4;++w) {
            constexpr const char* names[]={"wheel_front_left","wheel_front_right","wheel_rear_left","wheel_rear_right"};
            if(art.names[i]==names[w]) {
                mesh.transform.position=art.definition->wheels[w].position;
                mesh.transform.position.y-=cybercar::restSuspension;
            }
        }
    } else {
        auto& mesh=ghost->addComponent<engine::MeshComponent>("Ghost silhouette",engine::Mesh::cube());
        mesh.transform.scale={1.8f,.65f,4};mesh.color={.12f,1,.9f};mesh.opacity=.32f;
        mesh.blend=engine::BlendMode::Alpha;mesh.setMaterial(0,engine::MaterialInstance{material});mesh.castsShadow=false;
    }
}
void StarterScene::presentGhost() {
    if(!ghost||!reference)return;
    ghost->enabled=trialMode&&(phase==Phase::Countdown||phase==Phase::Racing||replay)&&race.elapsed<=reference->duration;
    const auto pose=reference->at(race.elapsed);
    ghost->transform.position=pose.position;ghost->transform.rotation=pose.rotation;
}
void StarterScene::watchBest() {
    if(!trialMode||!personalBest()||(phase!=Phase::Ready&&phase!=Phase::Finished))return;
    resetGrid();replay=true;phase=Phase::Racing;player().enabled=false;
    engine::CameraFollowSettings follow;follow.enabled=true;follow.target=ghost->savedId();
    follow.offset={0,4.8f,-10.5f};follow.focusOffset={0,.6f,2};follow.positionHalfLife=.09f;follow.rotationHalfLife=.08f;
    follow.collision=false;camera->setFollow(follow);cameraRig->reset();presentGhost();synchronize();
}
}
