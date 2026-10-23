#include "starter_scene.hpp"
#include "engine/collider_component.hpp"
#include "engine/physics.hpp"
#include <iostream>
#include <stdexcept>
using namespace racing;
using namespace engine;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){try {
    LaserBeam beam,other;
    beam.fire({0,1,0},{0,1,20},{0,0,-1},LaserImpact::Surface);
    beam.present(CombatState::beamDuration,{0,5,-10});
    check(beam.enabled&&beam.length()==20,"Shot has exact physical length");
    auto& axis=*beam.children()[0];
    auto& core=static_cast<MeshComponent&>(*axis.children()[0]);
    check(core.mesh==static_cast<MeshComponent&>(*other.children()[0]->children()[0]).mesh,"Shared immutable beam mesh");
    check(magnitude(transformPoint(core.worldMatrix(),{0,0,1})-beam.endpoint())<.001f,"Core ends at contact");
    auto matrix=core.worldMatrix().m;
    for(int i=0;i<20;++i)beam.present(CombatState::beamDuration,{0,5,-10});
    check(matrix==core.worldMatrix().m,"Presentation cannot advance the shot clock");
    beam.present(.02f,{0,5,-10});check(core.opacity<.5f,"Pulse fades with simulation time");
    beam.present(0,{0,5,-10});check(!beam.enabled,"Expired pulse submits no meshes");
    beam.fire({},{0,0,42},{0,0,-1},LaserImpact::None);beam.present(.14f,{});
    for(unsigned i=1;i<beam.children().size();++i)check(!beam.children()[i]->enabled,"Miss has no false contact flare or sparks");
    beam.fire({},{0,0,8},{0,0,-1},LaserImpact::Shield);beam.present(.14f,{});
    check(beam.children()[1]->enabled,"Shield receives contact flare");
    for(unsigned i=2;i<beam.children().size();++i)check(!beam.children()[i]->enabled,"Shield receives no metal sparks");
    beam.fire({},{},{},LaserImpact::Surface);beam.present(.14f,{});check(!beam.enabled,"Degenerate ray stays hidden");
    for(auto chassis:{Chassis::Vektor,Chassis::Bastion,Chassis::Skimmer,Chassis::Vector}) {
        StarterScene game;game.selectChassis(chassis);game.start();game.phase=Phase::Racing;
        // Isolate the player in a clear part of the world; use real Jolt queries.
        int parked=0;
        for(const auto& child:game.children())if(auto* a=dynamic_cast<Actor*>(child.get());a&&a->enabled&&a!=&game.player())
            if(auto* b=a->getComponent<RigidBodyComponent>();b&&b->settings().motion==BodyMotion::Dynamic) {
                Transform t;t.position={300.f+parked++*10,10,300};b->teleport(t);
            }
        Transform pose;pose.position={0,10,0};game.body().teleport(pose);game.synchronize();
        auto* weapon=game.player().getComponent<PulseWeapon>();
        const auto muzzle=transformPoint(game.player().worldMatrix(),weapon->muzzleLocal(1));
        auto& wall=game.spawn("Narrow muzzle-side cover");
        wall.transform.position=muzzle+Vec3{0,0,3};
        // This cover blocks the right muzzle but not the obsolete centreline ray.
        RigidBodySettings settings;settings.motion=BodyMotion::Static;
        wall.addComponent<RigidBodyComponent>("Cover body",settings);
        wall.addComponent<ColliderComponent>("Cover shape",CollisionShape::box({.13f,.6f,.1f}));game.synchronize();
        game.input.fire=1;game.fixedStep(1.f/60);game.present(0,1.6f);
        check(game.laser().enabled&&game.laser().impact()==LaserImpact::Surface,"Muzzle-side cover produces a real surface hit");
        check(std::abs(game.laser().length()-2.9f)<.02f,"Beam stops at cover without passing through it");
        check(magnitude(game.laser().origin()-muzzle)<.001f,"Beam starts at actual alternating aperture");
        game.input.fire=0;for(int i=0;i<60;++i)game.fixedStep(1.f/60);
        game.input.fire=1;game.fixedStep(1.f/60);game.present(0,1.6f);
        check(magnitude(game.laser().origin()-transformPoint(game.player().worldMatrix(),weapon->muzzleLocal(0)))<.001f,"Next pulse uses opposite aperture");
        check(game.laser().impact()==LaserImpact::None&&std::abs(game.laser().length()-42)<.001f,"Clear shot reaches exact range");
        const auto originalPose=game.player().worldMatrix();
        const auto localEnd=transformPoint(inverseAffine(originalPose),game.laser().endpoint());
        auto playerCombat=[&] {for(const auto& r:game.standings())if(r.id==0)return r.combat;return CombatState{};};
        const auto shots=playerCombat().shots,hits=playerCombat().hits;
        // Move and turn during a live pulse without advancing its combat clock.
        pose.position={5,12,7};pose.rotation={.12f,.6f,-.08f};game.body().teleport(pose);
        for(int frame=0;frame<8;++frame)game.present(0,1.6f);
        const auto shownPose=game.physics().presentationMatrix(game.player());
        check(magnitude(game.laser().origin()-transformPoint(shownPose,weapon->muzzleLocal(0)))<.001f,"Live beam follows translated and rotated cannon");
        check(magnitude(game.laser().endpoint()-transformPoint(shownPose,localEnd))<.001f,"Live endpoint follows car without accumulating drift");
        const auto& shownCore=*game.laser().children()[0]->children()[0];
        check(magnitude(transformPoint(shownCore.worldMatrix(),{})-game.laser().origin())<.001f,"Rendered core stays attached to moving muzzle");
        check(magnitude(transformPoint(shownCore.worldMatrix(),{0,0,1})-game.laser().endpoint())<.001f,"Rendered core reaches moving endpoint");
        check(std::abs(game.laser().length()-42)<.001f,"Following preserves shot range");
        check(playerCombat().shots==shots&&playerCombat().hits==hits,"Following never repeats combat damage");
        game.recover();check(!game.laser().enabled,"Recovery immediately clears the world shot");
        game.start();check(!game.laser().enabled&&game.laser().length()==0,"Restart clears retained contact data");
    }
    std::cout<<"Laser geometry, pause, expiry, impact types, muzzle cover, range, four-chassis motion and reset passed\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
