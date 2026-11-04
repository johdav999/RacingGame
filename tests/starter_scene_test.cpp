#include "starter_scene.hpp"
#include "cybercar_contract.hpp"
#include "vehicle_art_contract.hpp"
#include "engine/application.hpp"
#include "engine/physics.hpp"
#include "engine/collider_component.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace racing;
using namespace engine;
void require(bool okay,const char* message){if(!okay)throw std::runtime_error(message);}
void progressionRules() {
    Progression p;
    require(!p.buy(0,0)&&p.credits==0,"No credit means no purchase");
    require(!p.buy(-1,0)&&!p.buy(0,3),"Invalid purchase rejected");
    for(int position=1;position<=6;++position) {
        Progression prize;require(prize.award(position)==Progression::payout(position)&&prize.completedRaces==1,"All finishing positions earn the listed prize");
    }
    require(p.award(0)==0&&p.completedRaces==0,"Invalid finish cannot award credits");
    p.award(1);require(p.buy(0,0)&&p.buy(0,1)&&p.buy(0,2)&&p.credits==200,"Purchases debit exact prices");
    require(p.vehicles[1].ranks==std::array<int,3>{},"Car purchases never upgrade hovercraft");
    auto saved=p.encode();auto copy=Progression::decode(saved);
    require(copy&&copy->encode()==saved,"Progression round trips without losing credits or ranks");
    for(const char* bad:{"","NA2 0 0 0 0 0 0 0 0 0","NA1 -1 0 0 0 0 0 0 0 0","NA1 0 0 2 0 0 0 0 0 0",
        "NA1 1000000000 0 0 0 0 0 0 0 0","NA1 0 0 0 4 0 0 0 0 0","NA1 0 0 0 0 0 0 0 0",
        "NA1 0 0 0 0 0 0 0 0 0 extra","NA1 1.5 0 0 0 0 0 0 0 0","NA1 999999999999999999999 0 0 0 0 0 0 0 0"})
        require(!Progression::decode(bad),"Malformed, unknown-version or out-of-range saves rejected atomically");
    p.credits=100000;
    for(int v=0;v<2;++v)for(int system=0;system<3;++system){
        while(p.vehicles[v].ranks[system]<3)require(p.buy(v,system),"All upgrade tiers purchasable");
        auto balance=p.credits;require(!p.buy(v,system)&&p.credits==balance&&p.price(v,system)==0,"Max rank cannot spend again");
    }
    p.credits=Progression::maxCredits-100;require(p.award(1)==100&&p.credits==Progression::maxCredits,"Wallet saturates without overflow");
    CombatState base,upgraded;auto u=p.vehicles[0];
    upgraded.interval=u.shotInterval();upgraded.capacity=u.shieldCapacity();upgraded.recharge=u.shieldRecharge();upgraded.charge=upgraded.capacity;
    base.fire();upgraded.fire();base.tick(.51f,false,true);upgraded.tick(.51f,false,true);
    require(!base.fire()&&upgraded.fire(),"Upgraded cannon fires sooner");
    base.tick(3,true,true);upgraded.tick(3,true,true);
    require(upgraded.charge>base.charge+74,"Upgraded shields last longer under identical drain");
    auto a=base.charge,b=upgraded.charge;base.tick(1,false,true);upgraded.tick(1,false,true);
    require(upgraded.charge-b>base.charge-a,"Upgraded shields recharge faster");
}
void combatRules() {
    CombatState c;
    require(c.fire()&&!c.fire()&&c.shots==1,"Pulse cooldown must reject repeated fire");
    c.tick(.81f,false,true);require(c.fire(),"Pulse must recharge");
    c.tick(.1f,true,true);require(c.shield && c.charge<100,"Shield drains while held");
    require(!c.hit()&&c.blocks==1&&c.disruption==0,"Shield must block drive disruption");
    for(int i=0;i<5;++i)c.hit();
    require(!c.shield&&c.releaseRequired,"Shield depletion requires release");
    c.tick(3,true,true);require(!c.shield,"Holding depleted shield must not flicker back on");
    c.tick(.1f,false,true);c.tick(.1f,true,true);require(c.shield,"Recharged shield can activate after release");
    c={};require(c.hit()&&c.received==1,"Unshielded hits disrupt");
    c.tick(.7f,false,true);require(c.disruption==0 && !c.hit(),"Hit grace outlasts disruption and prevents stun chains");
    c.tick(1,false,true);require(c.hit(),"Damage resumes after protection expires");
    c.tick(.1f,true,false);require(!c.shield&&c.disruption==0,"Inactive/finished combat clears active effects");
}
void rules() {
    Circuit track;RaceProgress race;
    auto initial=track.gate(0)+track.gateForward(0)*2;initial.y=1;
    auto previous=initial;
    for(int lap=0;lap<3;++lap)for(int i=1;i<=Circuit::samples;++i) {
        auto next=track.point(i);next.y=1;
        race.step(track,previous,next,1.0/60);previous=next;
    }
    // Cross the plane despite roundoff in the final exact center sample.
    auto after=track.gate(0)+track.gateForward(0)*2;after.y=1;race.step(track,previous,after,1.0/60);
    require(race.finished && race.completedLaps==3,"Three valid laps must finish");
    auto elapsed=race.elapsed;race.step(track,after,after,4);require(race.elapsed==elapsed,"Finished time must freeze");
    race.reset();auto gate=track.gate(1),forward=track.gateForward(1);
    race.step(track,gate+forward*2,gate-forward*2,.1);require(race.nextGate==1,"Reverse crossings must not advance");
    race.step(track,track.gate(4),track.gate(4)+track.gateForward(4),.1);require(race.nextGate==1,"Skipped gates must not advance");
    race.reset();race.step(track,initial,{0,1,0},.1);require(!race.routeValid,"Shortcut must invalidate route");
    race.step(track,gate-forward,gate+forward,.1);require(race.nextGate==1,"Invalid route must not advance");
    race.recover();require(race.nextGate==1 && race.recoveries==1 && race.penalty==3,"Recovery must preserve progress and add penalty");
    race.step(track,gate-forward,gate+forward,.1);require(race.nextGate==2,"Recovered route can progress");
    race.reset();
    float early=race.distanceScore(track,track.point(3)),late=race.distanceScore(track,track.point(12));
    require(late>early && late<1,"Position interpolates forward inside the validated sector");
    require(race.distanceScore(track,track.gate(8))<=1,"Skipping a sector cannot award ranking progress");
    race.completedLaps=1;
    require(race.distanceScore(track,track.point(3))>Circuit::gates,"Completed laps dominate sector distance");
}
struct Game {
    Application app;StarterScene* scene;double clock=0;
    explicit Game(bool loadLastBuilding=true,TrackKind track=TrackKind::Kairox){
        static const auto asset=[] {
            std::ifstream file(std::string(RACING_ASSET_DIR)+"/models/cybercar-clean-detail/cybercar.glb",std::ios::binary|std::ios::ate);
            if(!file)throw std::runtime_error("Missing textured car");
            std::vector<std::byte> bytes(static_cast<std::size_t>(file.tellg()));
            file.seekg(0);file.read(reinterpret_cast<char*>(bytes.data()),bytes.size());return bytes;
        }();
        auto s=std::make_unique<StarterScene>(track);scene=s.get();scene->installPlayerCar(asset);
        static const auto weapon=[] {
            std::ifstream file(std::string(RACING_ASSET_DIR)+"/models/weapons/pulse-cannon/pulse-cannon.glb",std::ios::binary|std::ios::ate);
            if(!file)throw std::runtime_error("Missing pulse cannon");
            std::vector<std::byte> bytes(static_cast<std::size_t>(file.tellg()));file.seekg(0);
            file.read(reinterpret_cast<char*>(bytes.data()),bytes.size());return bytes;
        }();
        scene->installWeaponArt(weapon);
        static const auto other=[] {
            std::array<std::vector<std::byte>,3> result;
            const char* names[]={"skimmer","bastion","vector"};
            for(int i=0;i<3;++i){
                std::ifstream file(std::string(RACING_ASSET_DIR)+"/models/"+names[i]+"/"+names[i]+".glb",std::ios::binary|std::ios::ate);
                if(!file)throw std::runtime_error("Missing prepared chassis");
                result[i].resize(static_cast<std::size_t>(file.tellg()));file.seekg(0);
                file.read(reinterpret_cast<char*>(result[i].data()),result[i].size());
            }return result;
        }();
        for(int i=0;i<3;++i)scene->installVehicleArt(static_cast<Chassis>(i+1),other[i]);
        if(desertTrack(track)) {
            for(unsigned i=0;i<desert::models.size();++i) {
                std::string name=desert::models[i].name;
                std::ifstream file(std::string(RACING_ASSET_DIR)+"/models/desert-solar/"+name+"/"+name+".glb",std::ios::binary|std::ios::ate);
                if(!file)throw std::runtime_error("Missing desert model");
                std::vector<std::byte> bytes(static_cast<std::size_t>(file.tellg()));file.seekg(0);
                file.read(reinterpret_cast<char*>(bytes.data()),bytes.size());scene->installDesertArt(i,bytes);
            }
            for(unsigned i=0;i<desertrace::models.size();++i) {
                std::string name=desertrace::models[i].name;
                std::ifstream file(std::string(RACING_ASSET_DIR)+"/models/desert-race-kit/"+name+"/"+name+".glb",std::ios::binary|std::ios::ate);
                if(!file)throw std::runtime_error("Missing desert race model");
                std::vector<std::byte> bytes(static_cast<std::size_t>(file.tellg()));file.seekg(0);
                file.read(reinterpret_cast<char*>(bytes.data()),bytes.size());scene->installDesertArt(desert::models.size()+i,bytes);
            }
            for(unsigned i=0;i<dune::models.size();++i) {
                std::string name=dune::models[i].name;
                std::ifstream file(std::string(RACING_ASSET_DIR)+"/models/dune-villages/"+name+"/"+name+".glb",std::ios::binary|std::ios::ate);
                if(!file)throw std::runtime_error("Missing dune building");
                std::vector<std::byte> bytes(static_cast<std::size_t>(file.tellg()));file.seekg(0);
                file.read(reinterpret_cast<char*>(bytes.data()),bytes.size());scene->installDesertArt(desert::models.size()+desertrace::models.size()+i,bytes);
            }
        } else {
        static const auto buildings=[] {
            std::array<std::vector<std::byte>,buildingart::models.size()> result;
            for(unsigned i=0;i<result.size();++i){
                std::string name=buildingart::models[i].name;
                std::ifstream file(std::string(RACING_ASSET_DIR)+"/models/buildings/"+name+"/"+name+".glb",std::ios::binary|std::ios::ate);
                if(!file)throw std::runtime_error("Missing prepared building");
                result[i].resize(static_cast<std::size_t>(file.tellg()));file.seekg(0);
                file.read(reinterpret_cast<char*>(result[i].data()),result[i].size());
            }return result;
        }();
        for(unsigned i=0;i<buildings.size()-(loadLastBuilding?0:1);++i)scene->installBuildingArt(i,buildings[i]);
        static const auto details=[] {
            std::array<std::vector<std::byte>,citykit::models.size()> result;
            for(unsigned i=0;i<result.size();++i) {
                std::string name=citykit::models[i].name;
                std::ifstream file(std::string(RACING_ASSET_DIR)+"/models/city-kit/"+name+"/"+name+".glb",std::ios::binary|std::ios::ate);
                if(!file)throw std::runtime_error("Missing city-kit model");
                result[i].resize(static_cast<std::size_t>(file.tellg()));file.seekg(0);
                file.read(reinterpret_cast<char*>(result[i].data()),result[i].size());
            }return result;
        }();
        for(unsigned i=0;i<details.size();++i)scene->installCityKit(i,details[i]);
        static const auto street=[] {
            std::array<std::vector<std::byte>,12> result;
            for(unsigned i=0;i<12;++i){
                const std::string name=streetkit::models[i].name;
                std::ifstream file(std::string(RACING_ASSET_DIR)+"/models/street-kit/"+name+"/"+name+".glb",std::ios::binary|std::ios::ate);
                if(!file)throw std::runtime_error("Missing prepared street kit");
                result[i].resize(static_cast<std::size_t>(file.tellg()));file.seekg(0);
                file.read(reinterpret_cast<char*>(result[i].data()),result[i].size());
            }return result;
        }();
        for(unsigned i=0;i<12;++i)scene->installStreetArt(i,street[i]);
        static const auto trackArt=[] {
            std::array<std::vector<std::byte>,13> result;
            for(unsigned i=0;i<result.size();++i) {
                std::string name=trackkit::models[i].name;
                std::ifstream file(std::string(RACING_ASSET_DIR)+"/models/track-kit/"+name+"/"+name+".glb",std::ios::binary|std::ios::ate);
                if(!file)throw std::runtime_error("Missing prepared track model");
                result[i].resize(static_cast<std::size_t>(file.tellg()));file.seekg(0);
                file.read(reinterpret_cast<char*>(result[i].data()),result[i].size());
            }return result;
        }();
        for(unsigned i=0;i<trackArt.size();++i)scene->installTrackArt(i,trackArt[i]);
        for(unsigned i=0;i<plaza::models.size();++i) {
            std::string name=plaza::models[i].name;
            std::ifstream file(std::string(RACING_ASSET_DIR)+"/models/plaza/"+name+"/"+name+".glb",std::ios::binary|std::ios::ate);
            if(!file)throw std::runtime_error("Missing plaza model");
            std::vector<std::byte> bytes(static_cast<std::size_t>(file.tellg()));file.seekg(0);
            file.read(reinterpret_cast<char*>(bytes.data()),bytes.size());scene->installPlazaArt(i,bytes);
        }
        }
        {
            std::ifstream file(std::string(RACING_ASSET_DIR)+"/models/navigation/next-gate-beacon/next-gate-beacon.glb",std::ios::binary|std::ios::ate);
            if(!file)throw std::runtime_error("Missing next-gate beacon");
            std::vector<std::byte> bytes(static_cast<std::size_t>(file.tellg()));file.seekg(0);
            file.read(reinterpret_cast<char*>(bytes.data()),bytes.size());scene->installGateMarker(bytes);
        }
        app.start(app.loadScene(std::move(s)));app.advance(0);
    }
    void step(DrivingInput input={},int frames=1){
        for(int i=0;i<frames;++i){scene->input=input;clock+=1.0/60;ApplicationFrameCallbacks cb;
            cb.afterSimulation=[&](Scene&,const ApplicationFrame& f){scene->present(f.presentation.delta,16.f/9);};
            app.advance(clock,cb);
        }
    }
    void begin(VehicleKind kind){scene->select(kind);scene->start();step({},190);require(scene->phase==Phase::Racing,"Countdown must enter racing");}
};
float angleDifference(float a,float b){return std::remainder(a-b,2*pi);}
void cityKitGameplay() {
    StarterScene unloaded;const std::array<std::byte,4> invalid{};
    auto initial=unloaded.children().size();bool rejected=false;
    try{unloaded.installCityKit(0,invalid);}catch(const std::exception&){rejected=true;}
    require(rejected&&!unloaded.allCityKitLoaded()&&unloaded.children().size()==initial,"Failed city-kit import is atomic");
    Game game;auto& s=*game.scene;require(s.allCityKitLoaded(),"Every city-kit model loads");
    std::array<unsigned,citykit::models.size()> counts{};std::array<const Mesh*,citykit::models.size()> shared{};
    for(const auto& p:s.cityKitPlacements()) {
        ++counts[p.model];
        if(s.track.nearest(p.position).distance<=p.radius+14)
            throw std::runtime_error(std::string(citykit::models[p.model].name)+" footprint clearance "+std::to_string(s.track.nearest(p.position).distance-p.radius));
        for(const auto& b:s.buildingPlacements())
            require(std::hypot(p.position.x-b.position.x,p.position.z-b.position.z)>p.radius+b.radius,"City kit avoids existing buildings");
    }
    require(s.cityKitPlacements().size()==164,"All 143 skyline sites replace boxes, alongside 21 authored details");
    require(counts==std::array<unsigned,14>{2,2,2,2,4,1,26,25,1,1,1,1,48,48},"Full skyline has deterministic shared-model counts");
    for(const auto& node:s.children())
        require(node->name!="Baked Kairox skyline"&&node->name!="City authoring","No procedural skyline fallback remains");
    for(const auto& p:s.cityKitPlacements())if(p.model>=12||(p.model>=6&&p.model<=7&&p.scale<1.f)) {
        require(p.radius<=15.01f,"Skyline replacements fit the original reserved footprint");
        for(const auto& other:s.cityKitPlacements())if(&p!=&other)
            require(std::hypot(p.position.x-other.position.x,p.position.z-other.position.z)>p.radius+other.radius,"Skyline avoids other city assets");
    }
    for(const auto& node:s.children())for(unsigned i=0;i<citykit::models.size();++i)if(node->name==citykit::models[i].name) {
        auto* actor=dynamic_cast<Actor*>(node.get());require(actor!=nullptr,"City detail is an actor");
        const auto visuals=actor->getComponents<MeshComponent>();require(visuals.size()==1,"One visual per detail");
        auto* mesh=visuals[0]->mesh.get();
        require(!shared[i]||shared[i]==mesh,"Instances share mesh and texture resources");shared[i]=mesh;
        require(actor->getComponents<ColliderComponent>().empty(),"Decoration adds no driving obstacles");
    }
    rejected=false;try{s.installCityKit(0,invalid);}catch(const std::exception&){rejected=true;}
    require(rejected&&s.allCityKitLoaded(),"Duplicate install preserves valid assets");
    std::cout<<"City-kit imports, sharing, readiness and road/building clearance passed: "+std::to_string(s.cityKitPlacements().size())+" instances\n";
}
void buildingGameplay() {
    StarterScene unloaded;
    const std::array<std::byte,4> invalid{};
    const auto initial=unloaded.children().size();bool rejected=false;
    try {unloaded.installBuildingArt(0,invalid);}catch(const std::exception&){rejected=true;}
    require(rejected&&unloaded.children().size()==initial&&!unloaded.allSceneArtLoaded(),"Invalid city asset preserves scene and readiness");
    {
        Game incomplete(false);
        require(incomplete.scene->allVehicleArtLoaded()&&incomplete.scene->allCityKitLoaded()&&!incomplete.scene->allSceneArtLoaded(),
            "The final expansion building must gate readiness even when all other art is installed");
        auto before=incomplete.scene->children().size();rejected=false;
        try{incomplete.scene->installBuildingArt(buildingart::models.size()-1,invalid);}catch(const std::exception&){rejected=true;}
        require(rejected&&incomplete.scene->children().size()==before&&!incomplete.scene->allSceneArtLoaded(),
            "Failed expansion import preserves previously installed buildings");
    }
    Game g;auto& s=*g.scene;
    require(s.allSceneArtLoaded()&&s.buildingPlacements().size()==12,"All buildings load before race readiness");
    std::array<const Mesh*,buildingart::models.size()> shared{};std::array<int,buildingart::models.size()> counts{};
    for(const auto& node:s.children())for(unsigned i=0;i<buildingart::models.size();++i)if(node->name==buildingart::models[i].name){
        auto* actor=dynamic_cast<Actor*>(node.get());require(actor!=nullptr,"Building must be an actor");
        auto visuals=actor->getComponents<MeshComponent>();require(visuals.size()==1,"Building has one shared visual");
        const auto* mesh=visuals[0]->mesh.get();
        require(!shared[i]||shared[i]==mesh,"Repeated buildings must share mesh and texture resources");
        shared[i]=mesh;++counts[i];
        require(actor->getComponents<ColliderComponent>().size()==1,"Each building has simple solid collision");
        const auto& bounds=mesh->localBounds();
        for(float x:{bounds.minimum.x,bounds.maximum.x})for(float z:{bounds.minimum.z,bounds.maximum.z}) {
            const auto corner=transformPoint(actor->worldMatrix(),{x,0,z});
            require(s.track.nearest(corner).distance>15,"Building corners clear road and barriers");
        }
    }
    require(counts==std::array<int,7>{3,3,2,1,1,1,1},"City contains all four new landmarks and retained original building types");
    const auto& places=s.buildingPlacements();
    for(unsigned i=0;i<places.size();++i)for(unsigned j=0;j<i;++j)
        require(magnitude(places[i].position-places[j].position)>places[i].radius+places[j].radius,"Building footprints do not overlap");
    std::cout<<"Building imports, shared resources, collision and clearance passed\n";
}
void chassisGameplay() {
    Game g;auto& s=*g.scene;require(s.allVehicleArtLoaded(),"Every chassis must finish loading");
    std::array<bool,4> field{};for(int id=1;id<StarterScene::racerCount;++id)field[int(s.racerChassis(id))]=true;
    require(std::all_of(field.begin(),field.end(),[](bool present){return present;}),"AI field uses all four model types");
    for(auto chassis:{Chassis::Bastion,Chassis::Skimmer,Chassis::Vector,Chassis::Vektor}) {
        s.selectChassis(chassis);require(s.selectedChassis()==chassis&&s.player().name==chassisName(chassis),"Selection installs the requested art");
        std::size_t triangles=0;
        for(auto* mesh:s.player().getComponents<MeshComponent>())if(!mesh->name.starts_with("Pulse ")&&!mesh->name.starts_with("Shield ")) {
            triangles+=mesh->mesh->indices.size()/3;
            require(mesh->mesh->materials.size()==1,"Chassis parts have shared decoded material resources");
        }
        unsigned bastionTriangles=0;
        for(const auto& part:vehicleart::models[0].parts)bastionTriangles+=part.indices/3;
        const unsigned expected[]={cybercar::triangles,47314,bastionTriangles,47399};
        require(triangles==expected[int(chassis)],"Runtime retains every prepared triangle exactly once");
        s.start();auto grid=s.player().transform.position;g.step({},175);
        require(magnitude(s.player().transform.position-grid)<1,"Countdown brakes must hold the grid rather than select reverse");
        g.step({},15);g.step({1,.18f},100);
        std::cout<<"chassis acceleration "<<chassisName(chassis)<<" "<<s.speed()<<'\n';
        require(s.speed()>15,"Each selected chassis physically accelerates");
        if(chassis==Chassis::Bastion) {
            const auto& wheels=s.playerDrivetrain();
            for(int i=0;i<4;++i){auto* node=s.findSavedNode(wheels.visuals()[i]);require(node&&node->name.starts_with("wheel_"),"Bastion has four separate wheel bindings");
                require(std::abs(wheels.state().wheels[i].rotation)>.1f,"Bastion wheel spin comes from physics");
                require(i<2?std::abs(wheels.state().wheels[i].steerAngle)>.01f:std::abs(wheels.state().wheels[i].steerAngle)<.001f,"Bastion steers front wheels only");}
        }
        auto before=s.player().transform.position;s.recover();g.step();require(magnitude(s.player().transform.position-before)>1,"Every chassis recovers through physics teleport");
    }
    s.selectChassis(Chassis::Vector);auto save=s.progression().encode();
    require(save.starts_with("NA5 ")&&s.restoreProgression(save)&&s.selectedChassis()==Chassis::Vector,"Chassis choice survives save/restore");
    require(s.restoreProgression("NA1 1234 5 1 0 1 2 3 2 1")&&s.selectedChassis()==Chassis::Skimmer&&s.progression().credits==1234,"Legacy careers migrate without losing upgrades or credits");
}
void upgradeDrive(VehicleKind kind) {
    std::array<float,2> speeds{};
    for(int tier=0;tier<2;++tier) {
        Game g;auto& s=*g.scene;
        if(tier){
            require(s.restoreProgression("NA1 10000 0 0 0 0 0 0 0 0"),"Load funded drive fixture");s.select(kind);
            for(int level=0;level<3;++level)require(s.purchase(0),"Purchase drive tiers");
            require(std::abs(s.playerDrivetrain().definition()->engineTorque-(kind==VehicleKind::Car?845.f:650.f))<.01f,"Car torque reflects only its own upgrades");
            auto saved=s.progression().encode();require(!s.restoreProgression("bad")&&s.progression().encode()==saved,"Bad save cannot partially replace a live career");
        }
        s.select(kind);
        for(const auto& child:s.children())if(auto* actor=dynamic_cast<Actor*>(child.get()))
            if(auto* body=actor->getComponent<RigidBodyComponent>();body&&body->settings().motion==BodyMotion::Dynamic){
                auto settings=body->settings();settings.collisionLayer=actor==&s.player()?1:2;body->configure(settings);
            }
        s.physics().setLayerCollision(1,2,false);s.synchronize();
        // Disabling hull contacts does not disable weapon rays. Shield both
        // fixtures identically so an AI hit cannot masquerade as slower drive.
        s.start();g.step({0,0,0,0,0,0,1},190);g.step({1,0,0,0,0,0,1},80);speeds[tier]=s.speed();
        for(const auto& r:s.standings())if(r.id==0)
            require(r.combat.received==0,"Acceleration benchmark must be free of combat disruption");
    }
    std::cout<<"drive upgrade "<<int(kind)<<" stock "<<speeds[0]<<" upgraded "<<speeds[1]<<'\n';
    require(speeds[1]>speeds[0]*1.05f,"Purchased drive upgrades must improve actual acceleration");
}
void combatGeometry(VehicleKind kind) {
    Game g;auto& s=*g.scene;s.select(kind);s.start();g.step({},2);
    auto field=s.standings();auto target=std::find_if(field.begin(),field.end(),[](const auto& r){return r.id==1;})->location;
    Transform pose;pose.position=target+Vec3{12,0,0};pose.rotation.y=-pi/2;
    s.body().teleport(pose);s.synchronize();
    require(s.weaponTarget(0)==1,"Both chassis acquire a visible forward rival");
    auto& wall=s.spawn("Combat cover test");wall.transform.position=target+Vec3{6,0,0};
    RigidBodySettings settings;settings.motion=BodyMotion::Static;
    wall.addComponent<RigidBodyComponent>("Cover",settings);
    wall.addComponent<ColliderComponent>("Cover collider",CollisionShape::box({.5f,2,3}));
    s.synchronize();require(s.weaponTarget(0)==-1,"Physical cover blocks aim acquisition");
    wall.enabled=false;pose.rotation.y=pi/2;s.body().teleport(pose);s.synchronize();
    require(s.weaponTarget(0)==-1,"Rear rivals cannot be targeted");
    pose.position=target+Vec3{60,0,0};pose.rotation.y=-pi/2;s.body().teleport(pose);s.synchronize();
    require(s.weaponTarget(0)==-1,"Out-of-range rivals cannot be targeted");
    // Move unrelated attackers away so the target has no rear threat that
    // would trigger its AI shield during this unshielded-hit fixture.
    int parked=0;
    for(const auto& child:s.children())if(auto* actor=dynamic_cast<Actor*>(child.get());actor&&actor->enabled&&actor!=&s.player())
        if(auto* body=actor->getComponent<RigidBodyComponent>();body&&body->settings().motion==BodyMotion::Dynamic&&magnitude(actor->transform.position-target)>1) {
            Transform away;away.position={200.f+10*parked++,1,200};body->teleport(away);
        }
    pose.position=target+Vec3{12,0,0};s.body().teleport(pose);s.synchronize();s.phase=Phase::Racing;
    g.step({0,0,0,0,0,1,0});
    auto fired=s.standings();
    auto shooter=std::find_if(fired.begin(),fired.end(),[](const auto& r){return r.id==0;});
    auto victim=std::find_if(fired.begin(),fired.end(),[](const auto& r){return r.id==1;});
    std::cout<<"targeted pulse "<<int(kind)<<" target "<<s.weaponTarget(0)<<" shots "<<shooter->combat.shots
        <<" hits "<<shooter->combat.hits<<" victim shield "<<victim->combat.shield<<" immunity "<<victim->combat.immunity
        <<" received "<<victim->combat.received<<" disruption "<<victim->combat.disruption<<'\n';
    require(shooter->combat.shots==1&&shooter->combat.hits==1&&victim->combat.disruption>0,
        "Both player chassis must deliver a real raycast hit and drive disruption");
}
void driving(VehicleKind kind) {
    Game g;auto& s=*g.scene;s.select(kind);
    // Isolate handling measurements from rear-end pushes by the live AI field.
    // Full-race tests below retain normal racer-to-racer collision.
    for(const auto& child:s.children())if(auto* actor=dynamic_cast<Actor*>(child.get()))
        if(auto* body=actor->getComponent<RigidBodyComponent>();body && body->settings().motion==BodyMotion::Dynamic) {
            auto settings=body->settings();settings.collisionLayer=actor==&s.player()?1:2;body->configure(settings);
        }
    s.physics().setLayerCollision(1,2,false);s.synchronize();g.begin(kind);
    std::array<float,4> initialSpin{};
    if(kind==VehicleKind::Car) {
        auto meshes=s.player().getComponents<MeshComponent>();
        std::erase_if(meshes,[](auto* m){return m->name.starts_with("Pulse ")||m->name.starts_with("Shield ");});
        require(s.playerCarLoaded() && meshes.size()==6,"Cybercar body, wheel wells and four closed wheels replace the placeholder");
        std::size_t triangles=0;const Material* wheelMaterial=nullptr;
        bool chassis=false;
        for(auto* mesh:meshes) {
            triangles+=mesh->mesh->indices.size()/3;
            const auto* shared=mesh->mesh->materials[0].get();
            if(mesh->name.starts_with("wheel_")) {
                require(!wheelMaterial || wheelMaterial==shared,"Wheels share the preserved decoded PBR maps");
                wheelMaterial=shared;
            }
            if(mesh->name=="cybercar_chassis")chassis=true;
            if(mesh->name=="cybercar_chassis")require(shared->baseColorTexture&&shared->baseColorTexture->width==4096&&shared->baseColorTexture->height==4096,
                "New cybercar retains its original 4K albedo");
        }
        require(triangles==cybercar::triangles&&chassis,"Every prepared cybercar triangle survives import");
        require(wheelMaterial&&wheelMaterial->baseColorTexture&&wheelMaterial->baseColorTexture->width==256&&
            wheelMaterial->metallicRoughnessTexture&&!wheelMaterial->normalTexture,"Authored wheels use their own rubber/alloy palette without damaged source normal maps");
        for(const auto& wheel:s.playerDrivetrain().definition()->wheels)
            require(wheel.suspensionMin==cybercar::suspensionMin&&wheel.suspensionMax==cybercar::suspensionMax,
                "Wheel suspension must stay inside the validated wheel-well envelope");
        for(int i=0;i<4;++i)initialSpin[i]=s.playerDrivetrain().state().wheels[i].rotation;
    }
    auto start=s.player().transform.position;auto heading=s.heading();
    g.step({1,0,0,0,0},100);
    std::cout<<"kind "<<int(kind)<<" speed "<<s.speed()<<" travel "<<magnitude(s.player().transform.position-start)<<'\n';
    require(s.speed()>15 && magnitude(s.player().transform.position-start)>3,"Throttle must drive vehicle forward");
    if(kind==VehicleKind::Car)for(int i=0;i<4;++i) {
        const auto& drive=s.playerDrivetrain();const auto& wheel=drive.state().wheels[i];
        auto* visual=s.findSavedNode(drive.visuals()[i]);
        require(visual && visual->parent()==&s.player(),"Wheel visual must bind to the player chassis");
        require(std::abs(wheel.rotation-initialSpin[i])>.01f && std::abs(wheel.angularVelocity)>1,"All four textured wheels must spin");
        // At alpha=1 the visible center must coincide with the physics wheel.
        s.physics().setInterpolationAlpha(1);
        const auto actual=s.physics().presentationMatrix(*visual);
        const auto expected=s.player().worldMatrix()*wheel.localPose;
        require(magnitude(transformPoint(actual,{})-transformPoint(expected,{}))<.001f,"Wheel visual must rotate at its physics pivot");
        require(magnitude(transformDirection(actual,{0,1,0})-transformDirection(expected,{0,1,0}))<.001f,"Visible wheel orientation must include physics spin");
    }
    // Brake on the clear center lane, before steering into a rival's driving line.
    // A rear-end collision can legitimately push a car even with all wheels locked.
    float moving=s.speed();g.step({0,0,1,0,0},90);
    require(s.speed()<moving*.5f,"Service brake must reduce speed");
    s.start();g.step({},190);g.step({1,0,0,0,0},100);heading=s.heading();
    auto cameraRight=s.camera->right();auto beforeTurn=s.player().transform.position;
    auto forwardBefore=transformDirection(s.player().worldMatrix(),{0,0,1});
    g.step({.5f,.6f,0,0,0},35);
    std::cout<<"right turn "<<angleDifference(s.heading(),heading)<<'\n';
    auto forwardAfter=transformDirection(s.player().worldMatrix(),{0,0,1});
    require(dot(forwardAfter-forwardBefore,cameraRight)>.04f,"Right steering must face screen-right");
    require(dot(s.player().transform.position-beforeTurn,cameraRight)>.1f,"Right steering must travel toward screen-right");
    if(kind==VehicleKind::Car) {
        const auto& drive=s.playerDrivetrain();
        require(std::abs(drive.state().wheels[0].steerAngle)>.02f && std::abs(drive.state().wheels[1].steerAngle)>.02f,"Both front wheels must steer");
        require(drive.state().wheels[2].steerAngle==0 && drive.state().wheels[3].steerAngle==0,"Rear wheels must not steer");
        s.physics().setInterpolationAlpha(1);
        for(int i=0;i<4;++i) {
            const auto actual=s.physics().presentationMatrix(*s.findSavedNode(drive.visuals()[i]));
            const auto expected=s.player().worldMatrix()*drive.state().wheels[i].localPose;
            require(magnitude(transformDirection(actual,{1,0,0})-transformDirection(expected,{1,0,0}))<.001f,"Rendered axle must follow physics steering");
        }
    }
    s.start();g.step({},190);g.step({1,0,0,0,1},80);require(s.battery<90,"Boost must consume battery");
    auto battery=s.battery;g.step({0,0,1,0,0},60);require(s.battery>battery,"Battery must recharge");
    s.recover();require(s.race.recoveries==1 && s.race.penalty==3,"Recovery penalty");
    require(s.speed()<1 && s.race.nextGate==1,"Recovery clears velocity without awarding checkpoint");
    auto elapsed=s.race.elapsed;s.cancelInput();g.app.pause();g.step({},120);
    auto pausedField=s.standings();g.step({},60);auto stillField=s.standings();
    for(int i=0;i<StarterScene::racerCount;++i)
        require(magnitude(pausedField[i].location-stillField[i].location)==0 && pausedField[i].progress.elapsed==stillField[i].progress.elapsed,
            "Pause must freeze every opponent and its clock");
    require(s.race.elapsed==elapsed,"Pause must freeze race clock");
    g.app.setSuspended(true);g.app.resume();g.step({},60);require(s.race.elapsed==elapsed,"Suspension must freeze race clock");
    g.app.setSuspended(false);g.step();s.start();require(s.race.elapsed==0 && s.race.recoveries==0,"Restart resets race");
    g.step({},190);
    Transform outside;outside.position={0,1,0};s.body().teleport(outside);s.synchronize();
    g.step({},100);
    require(s.race.recoveries==1 && s.race.nextGate==1,"Off-course driving automatically recovers without advancing a gate");
    require(s.track.nearest(s.player().transform.position).distance<1,"Automatic recovery returns to the circuit");
    for(int i=0;i<240;++i) {
        g.step({1,0,0,0,1});
        require(s.track.nearest(s.player().transform.position).distance<12,"Continuous hull collision must contain a boosted barrier impact");
    }
    require(s.race.recoveries==1,"Barrier containment must not rely on automatic recovery");
}
void wheelTravel(Chassis chassis) {
    Game g;auto& s=*g.scene;s.selectChassis(chassis);s.start();g.step({},190);
    const auto& definition=*s.playerDrivetrain().definition();
    MeshComponent* liner=nullptr;
    for(auto* mesh:s.player().getComponents<MeshComponent>())
        if(mesh->name==(chassis==Chassis::Bastion?"wheel_wells":"cybercar_wheel_wells"))liner=mesh;
    require(liner&&liner->parent()==&s.player(),"Both cars install fixed wheel-well liners");
    for(const auto& child:s.children())if(auto* actor=dynamic_cast<Actor*>(child.get()))
        if(auto* body=actor->getComponent<RigidBodyComponent>();body&&body->settings().motion==BodyMotion::Dynamic) {
            auto settings=body->settings();settings.collisionLayer=actor==&s.player()?1:2;body->configure(settings);
        }
    s.physics().setLayerCollision(1,2,false);s.synchronize();
    std::array<float,4> minimum{1,1,1,1},maximum{};
    for(float steering:{-1.f,1.f}) {
        s.recover();Transform pose;pose.position=s.player().transform.position+Vec3{0,.8f,0};
        pose.rotation=s.player().transform.rotation;s.body().teleport(pose);
        s.body().setLinearVelocity({0,-3,0});s.synchronize();
        for(int frame=0;frame<120;++frame) {
            g.step({.25f,steering,0,0,0});s.physics().setInterpolationAlpha(1);
            const auto linerPose=s.physics().presentationMatrix(*liner),bodyPose=s.player().worldMatrix();
            require(magnitude(transformPoint(linerPose,{})-transformPoint(bodyPose,{}))<.001f&&
                magnitude(transformDirection(linerPose,{0,1,0})-transformDirection(bodyPose,{0,1,0}))<.001f,
                "Wheel-well liners stay fixed to the body while the four wheels move");
            for(int i=0;i<4;++i) {
                const auto& drive=s.playerDrivetrain();const auto& wheel=drive.state().wheels[i];
                minimum[i]=std::min(minimum[i],wheel.suspensionLength);maximum[i]=std::max(maximum[i],wheel.suspensionLength);
                // Jolt's bump stop is a constraint, not a hard clamp: allow the
                // 10 mm transient overtravel also checked by the asset envelope.
                require(wheel.suspensionLength>=definition.wheels[i].suspensionMin-.01f&&wheel.suspensionLength<=definition.wheels[i].suspensionMax+.001f,
                    "Physics suspension remains inside the authored envelope including bump-stop tolerance");
                const auto actual=s.physics().presentationMatrix(*s.findSavedNode(drive.visuals()[i]));
                const auto expected=s.player().worldMatrix()*wheel.localPose;
                require(magnitude(transformPoint(actual,{})-transformPoint(expected,{}))<.001f,"Wheel stays on its physics hub during travel");
                require(magnitude(transformDirection(actual,{0,1,0})-transformDirection(expected,{0,1,0}))<.001f,
                    "Rotation and steering remain applied during suspension movement");
            }
        }
    }
    for(int i=0;i<4;++i) {
        std::cout<<"chassis "<<static_cast<int>(chassis)<<" wheel "<<i<<" suspension "<<minimum[i]<<".."<<maximum[i]<<'\n';
        require(minimum[i]<definition.wheels[i].suspensionMin+.005f&&maximum[i]>definition.wheels[i].suspensionMax-.005f,
            "Drop test reaches full bump and droop on every wheel");
    }
}
void reverseCars() {
    for(auto chassis:{Chassis::Vektor,Chassis::Bastion}) {
        Game g;auto& s=*g.scene;s.selectChassis(chassis);
        for(const auto& child:s.children())if(auto* actor=dynamic_cast<Actor*>(child.get()))
            if(auto* body=actor->getComponent<RigidBodyComponent>();body && body->settings().motion==BodyMotion::Dynamic) {
                auto settings=body->settings();settings.collisionLayer=actor==&s.player()?1:2;body->configure(settings);
            }
        s.physics().setLayerCollision(1,2,false);s.synchronize();s.start();g.step({},190);
        const auto forward=normalized(transformDirection(s.player().worldMatrix(),{0,0,1}));
        const auto start=s.player().transform.position;
        g.step({0,0,1},20);
        require(dot(s.player().transform.position-start,forward)<-.1f,
            "Holding the car brake from rest must move it backwards");
        require(s.playerDrivetrain().state().gear==-1,"Both wheeled chassis must engage reverse gear");
        g.step({},1);
        require(s.playerDrivetrain().command().throttle==0,"Releasing reverse must cancel engine input");
        g.step({1},1);
        require(s.playerDrivetrain().command().throttle==0 && s.playerDrivetrain().command().brake>0,
            "Forward input must brake a reversing car before shifting");
        g.step({1},90);
        require(dot(s.body().linearVelocity(),forward)>1,"Car must drive forward after reversing");
    }
}
void complete(VehicleKind kind,bool upgraded=false,std::optional<Chassis> chassis={},TrackKind track=TrackKind::Kairox) {
    Game g(true,track);auto& s=*g.scene;
    if(upgraded){require(s.restoreProgression("NA1 2000 7 0 3 3 3 3 3 3"),"Load upgraded career");}
    if(chassis){s.selectChassis(*chassis);s.start();g.step({},190);}else g.begin(kind);
    ai::Planner testPlanner(s.track);ai::State testDriver;int reverseTicks=0,slowTicks=0;
    auto openingBalance=s.progression().credits,openingRaces=s.progression().completedRaces;
    require(!s.purchase(0),"Cannot buy during a race");
    auto grid=s.standings();
    require(grid[0].id==0,"Player starts at the front of the grid");
    for(int i=0;i<(desertTrack(track)&&track!=TrackKind::DesertSolar?30000:18000) && !s.classificationFinal();++i) {
        if(s.awaitingResults()) {
            s.start();s.select(kind==VehicleKind::Car?VehicleKind::Hover:VehicleKind::Car);
            require(s.phase==Phase::Finished&&s.kind==kind&&s.progression().credits==openingBalance,"Pending classification cannot be skipped or paid early");
        }
        auto p=s.player().transform.position;auto nearest=s.track.nearest(p);
        auto target=s.track.point(nearest.segment+6);auto d=target-p;
        float angle=angleDifference(std::atan2(d.x,d.z),s.heading());
        DrivingInput input;input.steer=std::clamp(-angle*2,-1.f,1.f);
        input.throttle=s.speed()<65?1.f:.1f;input.brake=s.speed()>76? .5f:0;
        if(track==TrackKind::SiroccoSolar||track==TrackKind::CanyonRelay) {
            std::array<ai::Car,1> cars{{{p,{std::sin(s.heading()),0,std::cos(s.heading())},s.body().linearVelocity()}}};
            auto planned=testPlanner.update(0,cars,testDriver,ai::Difficulty::Easy,1.f/60);
            input.throttle=planned.throttle;input.steer=planned.steer;input.brake=planned.brake;
            // Physical traffic can pin the test driver against a wall. Back out
            // using ordinary reverse input rather than teleporting the player.
            slowTicks=s.speed()<3?slowTicks+1:0;
            if(slowTicks>120&&reverseTicks==0){reverseTicks=90;slowTicks=0;}
            if(reverseTicks>0){--reverseTicks;input.throttle=-1;input.brake=0;input.steer=-planned.steer;}
        }
        input.fire=1;input.shield=std::fmod(s.race.elapsed,4)<1.2?1.f:0;
        g.step(input);
    }
    std::cout<<"completion kind "<<int(kind)<<" laps "<<s.race.completedLaps<<" gate "<<s.race.nextGate<<" time "<<s.race.elapsed<<" recoveries "<<s.race.recoveries<<" valid "<<s.race.routeValid<<" speed "<<s.speed()<<"\n";
    std::cout<<"  position "<<s.player().transform.position.x<<","<<s.player().transform.position.z<<" nearest "<<s.track.nearest(s.player().transform.position).segment<<" heading "<<s.heading()<<std::endl;
    require(s.phase==Phase::Finished,"Both vehicles must physically complete three laps");
    require(s.race.recoveries==0,"Centerline driver must finish without recovery");
    require(s.camera->worldToScreen(s.player().transform.position,{0,0,1280,720}).visible,"Chase camera must see player");
    auto results=s.standings();int cars=0,hovers=0,aiShots=0,aiHits=0,blocks=0;
    for(const auto& r:results) {
        std::cout<<"  "<<r.position<<" "<<r.name<<" class "<<int(r.kind)<<" laps "<<r.progress.completedLaps
            <<" gate "<<r.progress.nextGate<<" time "<<r.progress.elapsed<<" recoveries "<<r.progress.recoveries
            <<" shots "<<r.combat.shots<<" hits "<<r.combat.hits<<" blocks "<<r.combat.blocks<<'\n';
        if(r.id){aiShots+=r.combat.shots;aiHits+=r.combat.hits;}blocks+=r.combat.blocks;
        if(r.kind==VehicleKind::Car)++cars;else ++hovers;
    }
    require(s.classificationFinal(),"All AI racers must physically finish three valid laps");
    auto you=std::find_if(results.begin(),results.end(),[](const auto& r){return r.id==0;});
    require(s.raceReward()==Progression::payout(you->position)&&s.progression().credits==openingBalance+s.raceReward(),"Final classification awards the exact prize");
    require(s.progression().completedRaces==openingRaces+1,"One completed race recorded");
    require(cars>=2 && hovers>=2,"Race field must contain cars and hovercraft");
    require(aiShots>0 && aiHits>0 && blocks>0,"Full races must exercise AI attacks, disruption and shield blocks");
    for(int i=0;i<StarterScene::racerCount;++i) {
        require(results[i].position==i+1 && results[i].progress.completedLaps==3,"Complete, unique finishing positions");
        if(i)require(results[i-1].progress.elapsed<=results[i].progress.elapsed,"Final times including penalties determine results");
    }
    auto frozen=s.standings();g.step({},120);
    require(s.progression().completedRaces==openingRaces+1&&s.progression().credits==openingBalance+s.raceReward(),"Repeated finished ticks cannot pay twice");
    for(int i=0;i<StarterScene::racerCount;++i) {
        auto now=s.standings()[i];
        require(now.progress.elapsed==frozen[i].progress.elapsed,"Final times freeze");
        require(now.combat.shots==frozen[i].combat.shots&&now.combat.received==frozen[i].combat.received,"Finished racers cannot fire or receive hits");
    }
    s.start();
    for(auto r:s.standings()) {
        require(!r.progress.finished && r.progress.elapsed==0 && r.progress.completedLaps==0,"Restart resets the entire field");
        require(r.combat.shots==0&&r.combat.blocks==0&&r.combat.charge==(upgraded&&r.id==0?175:100)&&!r.combat.shield,"Restart resets combat with installed capacity");
    }
}
void desertGameplay(TrackKind selected=TrackKind::DesertSolar) {
    Game g(true,selected);auto& s=*g.scene;
    require(s.allSceneArtLoaded(),"Desert must wait for environment, dividers, gates, vehicles, weapon and beacon");
    require(s.buildingPlacements().empty()&&s.trackPlacements().empty()&&s.cityKitPlacements().empty(),"Desert must not instantiate Kairox assets");
    std::array<int,desert::models.size()> counts{};for(const auto& p:s.desertPlacements())++counts[p.model];
    for(int count:counts)require(count>0,"Every desert kit model must appear");
    desert::Kit landscape;landscape.plan(s.track);
    int embedded=0;
    for(const auto& p:landscape.placements())if(p.model>=3&&p.model<=6) {
        require(landscape.height(s.track,p.position.x,p.position.z)>p.position.y+1,"Cliffs must be embedded in continuous terrain");
        ++embedded;
        auto d=desert::models[p.model].dimensions*p.scale;
        for(float x=-d.x*.5f;x<=d.x*.5f;x+=1.f)for(float z=-d.z*.5f;z<=d.z*.5f;z+=1.f) {
            auto q=p.position+Vec3{std::cos(p.yaw)*x+std::sin(p.yaw)*z,0,-std::sin(p.yaw)*x+std::cos(p.yaw)*z};
            require(s.track.nearest(q).distance>13,"Canyon faces must leave road and shoulders clear");
            require(landscape.height(s.track,q.x,q.z)>p.position.y+.25f,"Downhill cliff edges must remain buried");
        }
    }
    require(embedded>50,"Canyon needs continuous walls and deeper formations");
    // Check the actual triangulated terrain height across the asphalt, including
    // between route samples. Physics-only raycasts cannot detect visual burial.
    auto terrainHeight=[&](Vec3 p) {
        constexpr float step=3.5f;
        float x=std::floor(p.x/step)*step,z=std::floor(p.z/step)*step,u=(p.x-x)/step,v=(p.z-z)/step;
        float a=landscape.height(s.track,x,z),b=landscape.height(s.track,x+step,z),c=landscape.height(s.track,x,z+step);
        if(u+v<=1)return a+(b-a)*u+(c-a)*v;
        float d=landscape.height(s.track,x+step,z+step);return d+(c-d)*(1-u)+(b-d)*(1-v);
    };
    for(int i=0;i<Circuit::samples;++i)for(float u:{0.f,.5f})for(float side:{-10.f,0.f,10.f}) {
        auto a=s.track.point(i)+s.track.right(i)*side,b=s.track.point(i+1)+s.track.right(i+1)*side;
        require(terrainHeight(a+(b-a)*u)<.015f,"Dunes must not cover rendered asphalt");
    }
    float length=0;
    for(int i=0;i<Circuit::samples;++i) {
        float segment=magnitude(s.track.point(i+1)-s.track.point(i));length+=segment;
        require(segment>4&&segment<9,"Uniform desert route spacing");
        for(int j=i+10;j<i+Circuit::samples-10;++j)
            require(magnitude(s.track.point(i)-s.track.point(j))>28,"Road sections must not intersect");
    }
    require(length>900,"Desert must be a distinct longer circuit");
    PhysicsQueryFilter filter;filter.includeDynamic=false;
    for(int sample=0;sample<Circuit::samples;++sample)for(float lane:{-7.f,0.f,7.f}) {
        auto p=s.track.point(sample)+s.track.right(sample)*lane+Vec3{0,1.3f,0};
        auto delta=s.track.point(sample+1)+s.track.right(sample+1)*lane+Vec3{0,1.3f,0}-p;
        require(!s.physics().raycast(p,normalized(delta),magnitude(delta),filter),"Full driving envelope must clear the arch and barriers");
    }
    g.begin(VehicleKind::Car);g.step({1,0,0,0,1},90);s.recover();
    require(s.race.recoveries==1&&s.race.penalty==3&&s.race.nextGate==1,"Desert recovery keeps valid gate and applies penalty");
    auto elapsed=s.race.elapsed;g.app.pause();g.step({},60);require(s.race.elapsed==elapsed,"Desert pause freezes race");
    std::cout<<"Desert loaded "<<s.desertPlacements().size()<<" placements, length "<<length<<"m; driving envelope clear\n";
}
void completeTrial(bool additions=false) {
    for(auto track:{TrackKind::Kairox,TrackKind::DesertSolar,TrackKind::SiroccoSolar,TrackKind::CanyonRelay}) {
        if(additions!=(track==TrackKind::SiroccoSolar||track==TrackKind::CanyonRelay))continue;
        Game g(true,track);auto& s=*g.scene;
        require(s.restoreProgression("NA1 2000 7 0 3 3 3 3 3 3"),"Upgraded career fixture");
        s.setTimeTrial(true);
        for(auto chassis:{Chassis::Vektor,Chassis::Skimmer,Chassis::Bastion,Chassis::Vector}) {
            if(additions&&chassis!=Chassis::Vektor)continue;
            s.selectChassis(chassis);require(!s.personalBest(),"Each chassis starts with its own empty record");
            require(!s.purchase(0),"Trials cannot buy upgrades");
            if(chassisKind(chassis)==VehicleKind::Car)require(std::abs(s.playerDrivetrain().definition()->engineTorque-(chassis==Chassis::Bastion?720:650))<.01f,"Trial chassis use stock power despite career upgrades");
            s.start();g.step({},190);
            ai::Planner testPlanner(s.track);ai::State testDriver;
            const auto grid=s.standings();
            for(int i=0;i<12000&&s.phase!=Phase::Finished;++i) {
                auto p=s.player().transform.position;auto nearest=s.track.nearest(p);auto d=s.track.point(nearest.segment+6)-p;
                const float angle=angleDifference(std::atan2(d.x,d.z),s.heading());
                DrivingInput input;input.steer=std::clamp(-angle*2,-1.f,1.f);
                input.throttle=s.speed()<65?1.f:.1f;input.brake=s.speed()>76?.5f:0;input.fire=1;input.shield=1;
                if(additions) {
                    std::array<ai::Car,1> cars{{{p,{std::sin(s.heading()),0,std::cos(s.heading())},s.body().linearVelocity()}}};
                    auto planned=testPlanner.update(0,cars,testDriver,ai::Difficulty::Normal,1.f/60);
                    input.throttle=planned.throttle;input.steer=planned.steer;input.brake=planned.brake;
                }
                g.step(input);
            }
            std::cout<<"trial track "<<int(track)<<" chassis "<<int(chassis)<<" time "<<s.race.elapsed<<" clean "<<s.trialClean()<<'\n';
            require(s.phase==Phase::Finished&&s.race.completedLaps==1&&s.classificationFinal(),"One physical lap finishes trial");
            require(s.trialClean()&&s.newPersonalBest()&&s.personalBest()&&s.personalBest()->valid(),"Clean physical run stores a valid PB replay");
            for(const auto& r:s.standings()) {
                require(r.combat.shots==0&&!r.combat.shield,"Solo trials disable combat");
                if(r.id) {auto old=std::find_if(grid.begin(),grid.end(),[&](const auto& v){return v.id==r.id;});require(magnitude(old->location-r.location)<.001f,"No active AI bodies in a trial");}
            }
            require(s.progression().credits==2000&&s.progression().completedRaces==7&&s.raceReward()==0,"Trials cannot award career credits");
            const auto record=s.personalBest()->encode();const auto duration=s.personalBest()->duration;auto revision=s.trialRevision();
            require(!s.restoreTrial(int(track),int(chassis),record),"Cannot restore records during results");
            s.watchBest();require(s.watchingReplay()&&s.ghostActor()&&!s.player().enabled,"Replay follows separate ghost with player disabled");
            g.step({},120);auto clock=s.race.elapsed;g.app.pause();g.step({},60);require(s.race.elapsed==clock,"Pause freezes replay clock");g.app.resume();
            for(int i=0;i<12000&&s.phase!=Phase::Finished;++i)g.step();
            require(std::abs(s.race.elapsed-duration)<1e-6&&s.personalBest()->encode()==record&&s.trialRevision()==revision,"Replay stops at exact endpoint without resaving");
            s.start();g.step({},190);require(s.ghostActor()->enabled&&s.player().enabled,"Retry starts racing saved ghost");
            require(!s.ghostActor()->getComponent<RigidBodyComponent>(),"Ghost has no physics body");
            s.recover();require(!s.trialClean()&&s.personalBest()->encode()==record,"Recovery cannot overwrite PB");
            s.selectChassis(chassis);require(s.restoreTrial(int(track),int(chassis),record),"Record reload accepted in garage");
            auto slower=*s.personalBest();slower.duration*=1.1;
            for(auto& split:slower.splits)split*=1.1;
            // Keep sample density within the codec's supported cadence.
            for(auto& frame:slower.frames)frame.time*=1.1;
            require(s.restoreTrial(int(track),int(chassis),slower.encode())&&s.personalBest()->encode()==record,"Loading a slower valid record cannot replace the best");
        }
        // Explicit clock-boundary fixture, separate from the physical laps above.
        s.start();g.step({},190);s.fixedStep(TrialRun::maxSeconds-s.race.elapsed);
        require(s.phase==Phase::Finished&&!s.trialClean()&&!s.newPersonalBest(),"Exact ten-minute limit invalidates unfinished attempt");
        s.selectChassis(s.selectedChassis());
        s.setTimeTrial(false);require(!s.timeTrial()&&!s.ghostActor()&&s.player().enabled,"Return to race restores scene");
        s.start();g.step({},190);g.step({},120);
        require(s.audioFrame().activeRacers==6,"All rival audio restored");
    }
}
void desertLighting() {
    StarterScene desert(TrackKind::DesertSolar),city(TrackKind::Kairox),restart(TrackKind::DesertSolar);
    require(desert.environment!=Environment::studio(),"Desert must replace the studio lighting environment");
    require(city.environment==Environment::studio(),"City lighting must remain unchanged");
    require(desert.environment==restart.environment,"Fixed daylight bake must be shared across restarts");
    desert.environment->validate();
    const LightComponent* sun=nullptr;
    for(const auto& node:desert.children())if(auto actor=dynamic_cast<const Actor*>(node.get()))
        for(const auto* light:actor->getComponents<LightComponent>())if(light->type==LightType::Directional)sun=light;
    require(sun!=nullptr,"Desert has a directional sun");
    const auto& cube=desert.environment->specular.front();
    float brightest=-1;Vec3 highlight{};
    for(unsigned face=0;face<6;++face)for(unsigned y=0;y<cube.size;++y)for(unsigned x=0;x<cube.size;++x) {
        const auto& pixel=cube.pixels[(face*cube.size+y)*cube.size+x];
        float brightness=pixel[0]+pixel[1]+pixel[2];
        if(brightness>brightest){brightest=brightness;highlight=cubeDirection(face,2*(x+.5f)/cube.size-1,2*(y+.5f)/cube.size-1);}
    }
    require(dot(highlight,normalized(sun->direction*-1.f))>.998f,"Reflection sun must align with direct illumination");
    const auto& diffuse=desert.environment->diffuse;
    Vec3 ground{};
    for(unsigned i=3*diffuse.size*diffuse.size;i<4*diffuse.size*diffuse.size;++i) {
        const auto& p=diffuse.pixels[i];ground=ground+Vec3{p[0],p[1],p[2]};
    }
    require(ground.x>ground.y&&ground.y>ground.z,"Lower hemisphere must provide warm sand bounce");
    const auto& sky=*desert.sky();
    require(sky.zenith.z/sky.zenith.x<1.7f,"Visible desert sky must stay pale rather than saturated blue");
    std::cout<<"Desert lighting: valid shared bake, aligned sun, warm ground bounce and unchanged city passed\n";
}
int main(int argc,char** argv){try{
    if(argc>1&&std::string(argv[1])=="desert-circuits") {
        for(auto track:{TrackKind::SiroccoSolar,TrackKind::CanyonRelay}) {
            desertGameplay(track);
            for(auto chassis:{Chassis::Vektor,Chassis::Skimmer,Chassis::Bastion,Chassis::Vector})
                complete(chassis==Chassis::Vektor||chassis==Chassis::Bastion?VehicleKind::Car:VehicleKind::Hover,false,chassis,track);
        }return 0;
    }
    if(argc>1&&std::string(argv[1])=="lighting"){desertLighting();return 0;}
    if(argc>1&&std::string(argv[1])=="wheel-travel"){wheelTravel(Chassis::Vektor);wheelTravel(Chassis::Bastion);return 0;}
    if(argc>1&&std::string(argv[1])=="desert-trials"){completeTrial(true);return 0;}
    if(argc>1&&std::string(argv[1])=="trial"){completeTrial();return 0;}
    if(argc>1&&std::string(argv[1])=="upgrade-drive"){upgradeDrive(VehicleKind::Car);upgradeDrive(VehicleKind::Hover);return 0;}
    if(argc>1&&std::string(argv[1])=="desert") {
        desertGameplay();complete(VehicleKind::Car,false,Chassis::Vektor,TrackKind::DesertSolar);
        complete(VehicleKind::Hover,false,Chassis::Skimmer,TrackKind::DesertSolar);
        complete(VehicleKind::Car,false,Chassis::Bastion,TrackKind::DesertSolar);
        complete(VehicleKind::Hover,false,Chassis::Vector,TrackKind::DesertSolar);
        return 0;
    }
    if(argc>1&&std::string(argv[1])=="city-kit"){cityKitGameplay();return 0;}
    if(argc>1&&std::string(argv[1])=="buildings"){buildingGameplay();return 0;}
    if(argc>1&&std::string(argv[1])=="chassis-races"){
        complete(VehicleKind::Car,false,Chassis::Bastion);complete(VehicleKind::Hover,false,Chassis::Vector);
        std::cout<<"Bastion and Vector complete races passed\n";return 0;
    }
    if(argc>1 && std::string(argv[1])=="reverse"){reverseCars();std::cout<<"Both cars reverse and return to forward drive\n";return 0;}
    rules();progressionRules();combatRules();cityKitGameplay();buildingGameplay();chassisGameplay();combatGeometry(VehicleKind::Car);combatGeometry(VehicleKind::Hover);
    upgradeDrive(VehicleKind::Car);upgradeDrive(VehicleKind::Hover);
    if(argc==1 || std::string(argv[1])!="combat"){driving(VehicleKind::Car);driving(VehicleKind::Hover);reverseCars();}
    complete(VehicleKind::Car);complete(VehicleKind::Hover);
    complete(VehicleKind::Car,true);complete(VehicleKind::Hover,true);
    std::cout<<"Racing prototype rules, physics, lifecycle and complete races passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
