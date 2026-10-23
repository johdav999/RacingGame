#include "starter_scene.hpp"
#include <iostream>
#include <stdexcept>
using namespace racing;
using namespace engine;
void check(bool okay,const char* message){if(!okay)throw std::runtime_error(message);}
int main(){try {
    ShieldEffect field,other;CombatState combat;
    // Inspect owned children without requiring a scene registry.
    check(field.children().size()==6,"Fixed six-draw maximum per shield");
    auto& shell=static_cast<MeshComponent&>(*field.children()[0]);
    auto& second=static_cast<MeshComponent&>(*other.children()[0]);
    check(shell.mesh==second.mesh,"Cars share immutable shield geometry");
    check(shell.resolvedMaterial(0).baseColorTexture==second.resolvedMaterial(0).baseColorTexture,"Cars share lattice texture");
    field.present({0,4,-10});check(!field.enabled,"Inactive shield submits nothing");
    combat.tick(.016f,true,true);field.advance(.016f,combat);field.present({0,4,-10});
    check(field.enabled&&field.visibility()>0&&field.visibility()<1,"Activation ramps in");
    for(int i=0;i<12;++i){combat.tick(.016f,true,true);field.advance(.016f,combat);}
    check(field.visibility()==1,"Active field reaches full envelope");
    for(int i=0;i<20;++i)field.impact({float(i%3)-1,0,-1});
    check(field.activeImpacts()==3,"Rapid impacts stay bounded");
    field.present({0,4,-10});
    auto frozen=field.children()[0]->transform.matrix();auto visibility=field.visibility();
    for(int i=0;i<100;++i){field.advance(0,combat);field.present({0,4,-10});}
    check(field.visibility()==visibility&&field.activeImpacts()==3&&field.children()[0]->transform.matrix().m==frozen.m,"Presentation and paused frames never age effects");
    combat.charge=1;combat.hit();field.advance(.016f,combat);field.present({0,4,-10});
    check(!combat.shield&&field.enabled&&field.breakAmount()>0,"Depleting hit retains impact and collapse feedback");
    for(int i=0;i<50;++i){combat.tick(.016f,true,true);field.advance(.016f,combat);}
    field.present({0,4,-10});check(!field.enabled&&field.activeImpacts()==0,"Collapse retires without held-input resurrection");
    field.reset();combat={};combat.tick(.016f,true,true);combat.charge=1;
    field.impact({0,0,-1});combat.hit();field.advance(.016f,combat);
    check(field.breakAmount()>0,"A shield depleted on its first tick still collapses");
    field.reset();field.present({0,4,-10});check(!field.enabled&&field.activeImpacts()==0&&field.visibility()==0,"Reset clears all transient feedback");
    field.fit({0,0,0},{2,1,3});
    check(std::abs(field.impactRay({1,0,-10},{0,0,1})-(10-3*std::sqrt(.75f)))<.001f,"Off-centre shots stop at the actual ellipsoid intersection");
    field.present({0,4,-10});
    auto& hit=static_cast<MeshComponent&>(*field.children()[3]);
    check(hit.transform.position.x>0&&hit.transform.position.z<0,"Ripple remains on the struck side of the field");
    field.reset();
    combat={};combat.tick(.016f,true,true);field.advance(.2f,combat);field.present({0,0,0});
    check(!field.children()[1]->enabled,"Camera inside shield suppresses invalid silhouette");
    for(auto& child:field.children()) {
        auto& mesh=static_cast<MeshComponent&>(*child);
        check(!mesh.castsShadow&&!mesh.receivesDecals&&mesh.blend==BlendMode::Additive,"Shield uses depth-tested additive presentation only");
        for(auto index:mesh.mesh->indices)check(index<mesh.mesh->vertices.size(),"Valid shared mesh indices");
    }
    StarterScene game;auto* retained=game.player().getComponent<ShieldEffect>();
    check(retained!=nullptr,"Player owns the effect");
    game.start();game.phase=Phase::Racing;game.input.shield=1;
    game.fixedStep(1.f/60);game.present(1.f/60,1.6f);
    check(retained->enabled,"Authoritative shield drives actual scene visual");
    game.recover();check(!retained->enabled,"Recovery clears old impact location");
    game.start();check(!retained->enabled&&retained->activeImpacts()==0,"Race restart clears effects");
    // A real raycast into a defending AI must drive both the block counter
    // and the retained visual. Park unrelated cars to remove cover ambiguity.
    Vec3 target{};for(const auto& r:game.standings())if(r.id==1)target=r.location;
    Actor* victim=nullptr;int parked=0;
    for(const auto& child:game.children())if(auto* actor=dynamic_cast<Actor*>(child.get());actor&&actor->enabled&&actor!=&game.player())
        if(auto* body=actor->getComponent<RigidBodyComponent>();body&&body->settings().motion==BodyMotion::Dynamic) {
            if(magnitude(actor->transform.position-target)<.1f)victim=actor;
            else {Transform away;away.position={200.f+10*parked++,1,200};body->teleport(away);}
        }
    check(victim!=nullptr,"AI victim located");
    auto forward=normalized(transformDirection(victim->worldMatrix(),{0,0,1}));
    Transform attack;attack.position=target-forward*9;attack.rotation=victim->transform.rotation;
    game.body().teleport(attack);game.synchronize();game.phase=Phase::Racing;game.input.fire=1;
    game.fixedStep(1.f/60);game.present(1.f/60,1.6f);
    bool blocked=false;for(const auto& r:game.standings())if(r.id==1)blocked=r.combat.blocks==1;
    check(blocked&&victim->getComponent<ShieldEffect>()->activeImpacts()==1,"Real blocked shot produces an impact on the defending AI");
    std::cout<<"Shield lifecycle, shared geometry, bounded impacts and scene integration passed\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
