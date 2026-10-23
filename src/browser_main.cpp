#include "starter_scene.hpp"
#include "engine/application.hpp"
#include "engine/renderer.hpp"
#include "engine/physics.hpp"
#include "engine/audio_browser.hpp"
#include "cybercar_contract.hpp"
#include "vehicle_art_contract.hpp"
#include "weapon_art_contract.hpp"
#include "performance_settings.hpp"
#include <emscripten.h>
#include <memory>
#include <vector>

namespace {
engine::Application app;
std::unique_ptr<engine::Renderer> renderer;
racing::StarterScene* game=nullptr;
std::vector<int> commands;
std::unique_ptr<racing::sound::RaceAudio> raceAudio;
EM_JS(void, read_trials, (), {window.restoreTimeTrials();});
EM_JS(int, selected_mode, (), {return new URLSearchParams(location.search).get('mode')==='trial';});
EM_JS(int, selected_difficulty, (), {return window.readRacingDifficulty();});
EM_JS(void, publish_ai, (int difficulty,int id,float target,float lane,float desired,float brake,int following,int passing), {
    if(id===0)window.racingAi={difficulty,racers:[]};
    window.racingAi.racers.push({id,targetSpeed:target*3.6,lane,desiredLane:desired,brake,following:!!following,passing});
    if(id===5)window.paintRacingDifficulty?.();
});
EM_JS(void, save_trial, (int track,int chassis,const char* bytes), {window.saveTimeTrial(track,chassis,UTF8ToString(bytes));});
EM_JS(void, publish_trial, (int active,int replay,int clean,int newBest,double best,const double* splits,const double* reference,
    int ghost,float x,float y,float z), {
    window.racingTrial={active:!!active,replay:!!replay,clean:!!clean,newBest:!!newBest,best,
        splits:Array.from(new Float64Array(HEAPU8.buffer,splits,3)),
        reference:Array.from(new Float64Array(HEAPU8.buffer,reference,3)),ghost:!!ghost,x,y,z};
});
EM_JS(void, init_audio, (unsigned device), {window.initRaceAudio(device);});
EM_JS(void, load_audio_clip, (unsigned id,const char* name,const char* hash), {window.queueRaceAudio(id,UTF8ToString(name),UTF8ToString(hash));});
EM_JS(void, publish_audio, (int ready,int loaded,int active,int voices,float pitch,const unsigned* starts), {
    window.racingAudioTelemetry={ready:!!ready,loaded,active:!!active,voices,pitch,starts:Array.from(new Uint32Array(HEAPU8.buffer,starts,13))};
    window.paintRaceAudio?.();
});
int qualityPreset=1, resolutionPercent=100;
bool qualityPending=true;
EM_JS(void, performance_frame, (double now,double cpu,int active), {
    window.recordRacingFrame(now,cpu,!!active);
});
EM_JS(void, performance_applied, (int preset,int scale), {
    window.racingQualityApplied={preset,scale};
    window.resetRacingMeasurements();
});
EM_JS(void, read_quality, (), {window.applyRacingSettings();});
EM_JS(int, read_progress, (char* buffer), {
    const save=window.readCareer();
    if(save===null)return 0;
    if(save.length>16384)return -1;
    for(let i=0;i<save.length;i++){if(save.charCodeAt(i)>127)return -1;HEAPU8[buffer+i]=save.charCodeAt(i);}
    HEAPU8[buffer+save.length]=0;return 1;
});
EM_JS(void, invalid_progress, (), {window.careerInvalid();});
EM_JS(void, publish_league, (const char* bytes), {window.racingLeague=JSON.parse(UTF8ToString(bytes));});
EM_JS(void, publish_progress, (const char* save,int reward,float drive,float speed,float interval,float capacity,float recharge,int p0,int p1,int p2), {
    window.updateCareer(UTF8ToString(save),{reward,drive,speed,interval,capacity,recharge,prices:[p0,p1,p2]});
});
EM_JS(int, selected_backend, (), {
    const backend = new URLSearchParams(location.search).get('renderer');
    return backend === 'webgpu' ? 1 : backend === 'webgl2' ? 2 : 0;
});
EM_JS(int, selected_track, (), {return new URLSearchParams(location.search).get('track')==='desert'?1:0;});
EM_JS(void, load_desert_art, (int id,const char* name,const char* hash), {window.queueDesertArt(id,UTF8ToString(name),UTF8ToString(hash));});
EM_JS(void, desert_race_placement, (int model,float x,float y,float z,float yaw,float sx,float sy,float sz,int gate), {
    (window.racingDesertRacePlacements ||= []).push({model,x,y,z,yaw,scale:[sx,sy,sz],gate});
});
EM_JS(int, desert_review, (), {
    const params=new URLSearchParams(location.search);
    if(!params.has('desertReview')&&!params.has('operationsReview'))return -1;
    const sample=window.racingDesertReview??(params.has('operationsReview')?192:Number(params.get('desertReview')));
    return Number.isInteger(sample)&&sample>=0&&sample<=192?sample:-1;
});
EM_JS(int, village_review, (), {
    const value=window.racingVillageReview ?? Number(new URLSearchParams(location.search).get('villageReview') ?? -1);
    return value===0||value===1?value:-1;
});
EM_JS(void, village_placement, (int model,int village,float x,float y,float z,float yaw), {
    (window.racingVillagePlacements ||= []).push({model,village,x,y,z,yaw});
});
EM_JS(int, hidden, (), { return document.hidden ? 1 : 0; });
EM_JS(int, ground_review, (), {
    const p=new URLSearchParams(location.search);if(!p.has('groundReview'))return -1;
    const id=window.racingGroundReview??Number(p.get('groundReview'));
    return Number.isInteger(id)&&id>=0&&id<5?id:-1;
});
EM_JS(void, publish_ground, (unsigned walks,unsigned drains,unsigned foundations,unsigned docks,unsigned clusters,unsigned vans,unsigned shadows,unsigned pools,unsigned damp,unsigned meshes,unsigned triangles), {
    window.racingCityGround={walks,drains,foundations,docks,clusters,vans,shadows,pools,damp,meshes,triangles};
});
EM_JS(int, plaza_review, (), {
    if(!new URLSearchParams(location.search).has('plazaReview'))return -1;
    const id=window.racingPlazaReview??Number(new URLSearchParams(location.search).get('plazaReview'));
    return Number.isInteger(id)&&id>=0&&id<4?id:-1;
});
EM_JS(void, load_plaza_art, (int id,const char* name,const char* hash), { window.queuePlazaArt(id,UTF8ToString(name),UTF8ToString(hash)); });
EM_JS(int, street_review, (), {
    if(!new URLSearchParams(location.search).has('streetReview'))return -1;
    const id=window.racingStreetReview??Number(new URLSearchParams(location.search).get('streetReview'));
    return Number.isInteger(id)&&id>=0&&id<6?id:-1;
});
EM_JS(int, track_review, (), {
    if(!new URLSearchParams(location.search).has('trackReview'))return -1;
    const id=window.racingTrackReview??Number(new URLSearchParams(location.search).get('trackReview'));
    return Number.isInteger(id)&&id>=0&&id<13?id:-1;
});
EM_JS(int, city_review, (int count), {
    if(!new URLSearchParams(location.search).has('cityReview'))return -1;
    const id=window.racingCityReview??Number(new URLSearchParams(location.search).get('cityReview'));
    return Number.isInteger(id)&&id>=0&&id<count?id:-1;
});
EM_JS(double, aspect, (), { return Module.canvas.width/Math.max(1,Module.canvas.height); });
EM_JS(int, building_review, (int count), {
    if(!new URLSearchParams(location.search).has('buildingReview'))return -1;
    const id=window.racingBuildingReview??Number(new URLSearchParams(location.search).get('buildingReview'));
    return Number.isInteger(id)&&id>=0&&id<count?id:-1;
});
EM_JS(void, ready_status, (int gl), {
    document.getElementById('status').textContent = 'LIVE / ' + (gl ? 'WEBGL 2' : 'WEBGPU');
    window.racingReady = true;
    window.racingBackend = gl ? 'webgl2' : 'webgpu';
});
EM_JS(void, load_gate_marker, (const char* name,const char* hash), {window.queueGateMarker(UTF8ToString(name),UTF8ToString(hash));});
EM_JS(void, publish_gate_marker, (int visible,int gate,float x,float y,float z,float yaw,float mountHeight), {
    window.racingGateMarker={visible:!!visible,gate,x,y,z,yaw,mountHeight};
});
EM_JS(void, load_car, (const char* hash), { window.loadPlayerCar(UTF8ToString(hash)); });
EM_JS(void, load_weapon, (const char* hash), { window.queueWeaponArt(0,'pulse-cannon',UTF8ToString(hash)); });
EM_JS(void, publish_weapon, (int rank,float recoil,int loaded), {window.racingWeaponVisual={rank,recoil,loaded:!!loaded};});
EM_JS(void, publish_laser, (int visible,int impact,float length,float x,float y,float z,float ex,float ey,float ez), {
    window.racingLaserVisual={visible:!!visible,impact,length,origin:[x,y,z],endpoint:[ex,ey,ez]};
});
EM_JS(int, weapon_review, (), {return new URLSearchParams(location.search).has('weaponReview')?1:0;});
EM_JS(int, wheel_review, (), {
    if(!new URLSearchParams(location.search).has('wheelReview'))return 0;
    return window.racingWheelReviewSide===2?2:1;
});
EM_JS(int, consume_wheel_review_drop, (), {
    if(!new URLSearchParams(location.search).has('wheelReview') || !window.racingWheelReviewDrop)return 0;
    window.racingWheelReviewDrop=false;return 1;
});
EM_JS(void, load_track_art, (int id,const char* name,const char* hash), { window.queueTrackArt(id,UTF8ToString(name),UTF8ToString(hash)); });
EM_JS(void, track_placement, (int model,float x,float y,float z,float yaw,int marking), {
    (window.racingTrackPlacements ||= []).push({model,x,y,z,yaw,marking});
});
EM_JS(void, load_vehicle, (int id,const char* name,const char* hash), { window.queueVehicleArt(id,UTF8ToString(name),UTF8ToString(hash)); });
EM_JS(void, load_building, (int id,const char* name,const char* hash), { window.queueBuildingArt(id,UTF8ToString(name),UTF8ToString(hash)); });
EM_JS(void, load_street, (int id,const char* name,const char* hash), { window.queueStreetArt(id,UTF8ToString(name),UTF8ToString(hash)); });
EM_JS(void, street_placement, (int model,float x,float y,float z,float yaw), {
    (window.racingStreetPlacements ||= []).push({model,x,y,z,yaw});
});
EM_JS(void, load_city_kit, (int id,const char* name,const char* hash), { window.queueCityKitArt(id,UTF8ToString(name),UTF8ToString(hash)); });
EM_JS(void, city_kit_placement, (int model,float x,float y,float z,float yaw,float radius,float scale), {
    (window.racingCityDetails ||= []).push({model,x,y,z,yaw,radius,scale});
});
EM_JS(void, building_placement, (int model,float x,float z,float yaw,float radius), {
    (window.racingBuildings ||= []).push({model,x,z,yaw,radius});
});
EM_JS(void, car_error, (const char* message), { window.racingCarError=UTF8ToString(message); });
EM_JS(void, begin_wheels, (), {window.racingWheels=[];});
EM_JS(void, publish_wheel, (const char* name,float spin,float steer,float omega,
    float x,float y,float z,float xx,float xy,float xz,float yx,float yy,float yz,
    float suspension,float minimum,float maximum,int contact), {
    window.racingWheels.push({name:UTF8ToString(name),spin,steer,omega,
        center:[x,y,z],axle:[xx,xy,xz],up:[yx,yy,yz],suspension,minimum,maximum,contact:!!contact});
});
EM_JS(void, publish, (int chassis,int phase,int kind,int paused,int lap,int gate,double elapsed,double best,double last,
    double countdown,float speed,float battery,float x,float y,float z,float heading,int recoveries,int boosting,int valid), {
    const s={chassis,phase,kind,paused:!!paused,lap,gate,elapsed,best,last,countdown,speed,battery,x,y,z,heading,recoveries,boosting:!!boosting,valid:!!valid,
        standings:window.racingStandings,classificationFinal:window.racingFinal,target:window.racingTarget,career:window.racingCareer,trial:window.racingTrial};
    s.combat=s.standings.find(r=>r.id===0).combat;
    window.racingState=s;
    if(window.updateRaceUI)window.updateRaceUI(s);
});
EM_JS(void, begin_standings, (int final), {window.racingStandings=[];window.racingFinal=!!final;});
EM_JS(void, publish_racer, (int chassis,int id,int position,const char* name,int kind,float x,float z,
    int lap,int gate,int finished,double elapsed,int recoveries,float charge,float cooldown,int shield,float disruption,
    int shots,int hits,int blocks,int received,int shieldUses,int releaseRequired), {
    window.racingStandings.push({chassis,id,position,name:UTF8ToString(name),kind,x,z,lap,gate,finished:!!finished,elapsed,recoveries,
        combat:{charge,cooldown,shield:!!shield,disruption,shots,hits,blocks,received,shieldUses,releaseRequired:!!releaseRequired}});
});
EM_JS(void, publish_target, (int id), {window.racingTarget=id;});
EM_JS(void, publish_shield_visual, (float visibility,float collapse,int impacts,int visible), {
    window.racingShieldVisual={visibility,collapse,impacts,visible:!!visible};
});
EM_JS(void, track_point, (float x,float z), { (window.racingTrack ||= []).push([x,z]); });
void clearInput() {
    engine::InputPacket p;p.type=static_cast<int>(engine::InputEventType::CancelAll);
    app.submitInput({&p,1});game->cancelInput();
}
void frame() try {
    const double frameStart=emscripten_get_now();
    renderer->serviceRecovery();
    if(qualityPending && renderer->ready()) {
        renderer->setQuality(racing::performanceQuality(qualityPreset,resolutionPercent));
        qualityPending=false;
        performance_applied(qualityPreset,resolutionPercent);
    }
    for(int command:commands) {
        clearInput();
        switch(command) {
        case 1:if(game->allSceneArtLoaded()){app.resume();game->start();}break;
        case 2:app.resume();game->select(racing::VehicleKind::Car);break;
        case 3:app.resume();game->select(racing::VehicleKind::Hover);break;
        case 4:game->recover();break;
        case 5:if(app.state()==engine::ApplicationState::Paused)app.resume();else app.pause();break;
        case 6:if(game->phase!=racing::Phase::Ready)app.pause();break;
        case 7:app.resume();game->selectChassis(game->selectedChassis());break;
        case 12:app.resume();game->selectChassis(racing::Chassis::Bastion);break;
        case 13:app.resume();game->selectChassis(racing::Chassis::Vector);break;
        case 30:game->setTimeTrial(false);break;
        case 31:game->setTimeTrial(true);break;
        case 32:if(game->allSceneArtLoaded()){app.resume();game->watchBest();}break;
        case 40:case 41:case 42:game->setDifficulty(command-40);break;
        case 50:case 51:case 52:case 53:case 54:game->enterLeague(command-50);break;
        case 60:if(game->allSceneArtLoaded()&&game->startLeagueRace())app.resume();break;
        case 20:case 21:case 22:game->purchase(command-20);break;
        default:break;
        }
    }
    commands.clear();
    // Inspection-only physical drop: the solver still owns all wheel poses.
    // Keep it out of ordinary races and never reposition individual visuals.
    if(wheel_review() && consume_wheel_review_drop() && game->phase==racing::Phase::Racing &&
       game->kind==racing::VehicleKind::Car && game->timeTrial() && app.state()==engine::ApplicationState::Running) {
        engine::Transform pose;pose.position=game->player().transform.position+engine::Vec3{0,.8f,0};
        pose.rotation=game->player().transform.rotation;
        game->body().teleport(pose);game->body().setLinearVelocity({0,-3,0});game->synchronize();
    }
    bool suspended=hidden()!=0 || !renderer->ready();
    if(suspended)game->cancelInput();
    app.setSuspended(suspended);
    engine::ApplicationFrameCallbacks callbacks;
    callbacks.input=[](engine::Scene&,const engine::InputSystem& input,const engine::ApplicationFrame&) {
        if(app.state()!=engine::ApplicationState::Running){game->cancelInput();return;}
        game->input={input.action(100).value.x,input.action(101).value.x,
            input.action(102).value.x,input.action(103).value.x,input.action(104).value.x,
            input.action(105).value.x,input.action(106).value.x};
    };
    callbacks.afterSimulation=[](engine::Scene&,const engine::ApplicationFrame& f){game->present(f.presentation.delta,static_cast<float>(aspect()));};
    app.advance(emscripten_get_now()/1000.0,callbacks);
    static unsigned savedTrialRevision=0;
    if(savedTrialRevision!=game->trialRevision()) {
        savedTrialRevision=game->trialRevision();
        if(const auto* run=game->personalBest())save_trial(run->track,run->chassis,run->encode().c_str());
    }
    const auto* best=game->personalBest();const auto* reference=game->comparisonRun();
    const std::array<double,3> emptySplits{};
    const auto* ghost=game->ghostActor();const auto ghostPosition=ghost?ghost->transform.position:engine::Vec3{};
    publish_trial(game->timeTrial(),game->watchingReplay(),game->trialClean(),game->newPersonalBest(),best?best->duration:0,
        game->trialSplits().data(),reference?reference->splits.data():emptySplits.data(),
        ghost&&ghost->enabled,ghostPosition.x,ghostPosition.y,ghostPosition.z);
    static double audioLast=emscripten_get_now();const double audioNow=emscripten_get_now();
    raceAudio->update(game->audioFrame(suspended||app.state()!=engine::ApplicationState::Running),(audioNow-audioLast)/1000);
    audioLast=audioNow;
    publish_audio(raceAudio->ready(),raceAudio->loaded(),raceAudio->system().backend().active(),
        raceAudio->system().voiceCount(),raceAudio->playerPitch(),raceAudio->starts().data());
    const auto& career=game->progression();const int selected=static_cast<int>(game->kind);
    const auto& upgrade=game->activeUpgrades();
    const auto* weapon=game->player().getComponent<racing::PulseWeapon>();
    publish_weapon(weapon->rank(),weapon->recoil(),weapon->loaded());
    const auto& laser=game->laser();const auto from=laser.origin(),to=laser.endpoint();
    publish_laser(laser.enabled,static_cast<int>(laser.impact()),laser.length(),from.x,from.y,from.z,to.x,to.y,to.z);
    static int publishedLeagueRevision=-1;static bool publishedLeagueRace=false,publishedLeagueFinal=false;
    if(publishedLeagueRevision!=career.championship.revision||publishedLeagueRace!=game->leagueRace()||publishedLeagueFinal!=game->leagueFinal()) {
        publish_league(career.championship.json(game->leagueRace(),game->leagueFinal()).c_str());
        publishedLeagueRevision=career.championship.revision;publishedLeagueRace=game->leagueRace();publishedLeagueFinal=game->leagueFinal();
    }
    publish_progress(career.encode().c_str(),game->raceReward(),upgrade.driveMultiplier(),upgrade.speedBonus(),
        upgrade.shotInterval(),upgrade.shieldCapacity(),upgrade.shieldRecharge(),
        career.price(selected,0),career.price(selected,1),career.price(selected,2));
    if (renderer->ready()) {
        // Explicit authoring review only; normal race cameras are unaffected.
        const int review=city_review(racing::citykit::models.size());
        if(review>=0) {
            for(const auto& p:game->cityKitPlacements())if(p.model==static_cast<unsigned>(review)) {
                auto d=racing::citykit::models[review].dimensions*p.scale;
                float extent=std::max({d.x,d.y,d.z,4.f});
                auto target=p.position+engine::Vec3{0,d.y*.45f,0};
                auto offset=engine::Vec3{extent*1.1f,extent*.7f,extent*1.1f};
                if(review==6||review==7||review>=12) {
                    const auto direction=review>=12?game->track.nearest(p.position).position-p.position:engine::Vec3{-p.position.x,0,-p.position.z};
                    offset=engine::normalized(direction)*(extent*(review>=12?1.4f:.95f));
                    offset.y=extent*.3f;
                }
                game->camera->transform.position=target+offset;
                game->camera->lookAt(target);break;
            }
        }
        const int buildingReview=building_review(racing::buildingart::models.size());
        if(buildingReview>=0) {
            for(const auto& p:game->buildingPlacements())if(p.model==static_cast<unsigned>(buildingReview)) {
                const auto d=racing::buildingart::models[buildingReview].dimensions;
                const float extent=std::max({d.x,d.y,d.z});
                const engine::Vec3 front{std::cos(p.yaw),0,-std::sin(p.yaw)};
                const engine::Vec3 side{front.z,0,-front.x};
                const auto target=p.position+engine::Vec3{0,d.y*.45f,0};
                game->camera->transform.position=target+front*(extent*1.25f)+side*(extent*.65f)+engine::Vec3{0,extent*.65f,0};
                game->camera->lookAt(target);break;
            }
        }
        renderer->resizeCanvas();
        const int trackReview=track_review();
        if(trackReview>=0)for(const auto& p:game->trackPlacements())if(p.model==static_cast<unsigned>(trackReview)) {
            auto d=racing::trackkit::models[trackReview].dimensions;
            float extent=std::max({d.x,d.y,d.z,2.f});
            auto target=p.position+engine::Vec3{0,d.y*.45f,0};
            auto direction=engine::transformDirection(engine::Transform{{}, {0,p.yaw,0}, {1,1,1}}.matrix(),{.6f,.5f,1.2f});
            game->camera->transform.position=target+direction*extent;game->camera->lookAt(target);break;
        }
        const int streetReview=street_review();
        if(streetReview>=0) {
            if(streetReview==0) {
                const auto center=game->track.point(56),across=game->track.right(56),along=game->track.tangent(56);
                game->camera->transform.position=center-along*36.f-across*27.f+engine::Vec3{0,23,0};
                game->camera->lookAt(center+across*9.f+engine::Vec3{0,8,0});
            } else {
                constexpr unsigned reviewModels[]={2,3,5,8,9,10};
                for(const auto& p:game->streetPlacements())if(p.model==reviewModels[streetReview]) {
                    auto d=racing::streetkit::models[p.model].dimensions;
                    const float extent=streetReview==2?10.f:streetReview==5?12.f:std::max({d.x,d.y,d.z,5.f});
                    const auto target=p.position+engine::Vec3{0,streetReview==2?3.2f:d.y*.45f,0};
                    const auto direction=engine::transformDirection(engine::Transform{{}, {0,p.yaw,0}, {1,1,1}}.matrix(),{.45f,.3f,1.25f});
                    game->camera->transform.position=target+direction*extent;
                    game->camera->lookAt(target);break;
                }
            }
        }
        const int plazaReview=plaza_review();
        if(plazaReview>=0) {
            bool close=plazaReview>=2;int sample=(plazaReview%2?92:0)+(close?5:12);
            auto target=racing::plaza::surfacePoint(game->track,float(sample),close?16.f:22.f);
            game->camera->transform.position=target+game->track.right(sample)*(close?5.f:-22.f)-game->track.tangent(sample)*(close?3.f:20.f)+engine::Vec3{0,close?1.2f:14.f,0};
            game->camera->lookAt(target);
        }
        const int groundReview=ground_review();
        if(!game->isDesert()&&groundReview>=0) {
            const auto& view=game->cityGround().views[groundReview];
            game->camera->transform.position=view.eye;game->camera->lookAt(view.target);
        }
        if(weapon_review()) {
            const auto matrix=game->player().worldMatrix();
            game->camera->transform.position=engine::transformPoint(matrix,{4.f,2.6f,5.5f});
            game->camera->lookAt(engine::transformPoint(matrix,{0,.15f,.6f}));
        }
        if(const int side=wheel_review()) {
            const auto matrix=game->player().worldMatrix();
            game->camera->transform.position=engine::transformPoint(matrix,{side==2?-3.7f:3.7f,1.0f,3.1f});
            game->camera->lookAt(engine::transformPoint(matrix,{0,-.23f,.2f}));
        }
        const int desertReview=desert_review();
        if(game->isDesert()&&desertReview>=0) {
            auto target=game->track.point(desertReview);
            game->camera->transform.position=target-game->track.tangent(desertReview)*32.f+engine::Vec3{0,18,0};
            game->camera->lookAt(target+game->track.tangent(desertReview)*32.f+engine::Vec3{0,5,0});
            if(desertReview==192){game->camera->transform.position={58,9,-149};game->camera->lookAt({40,2,-131});}
        }
        const int villageReview=village_review();
        if(game->isDesert()&&villageReview>=0) {
            int sample=racing::dune::villageSamples[villageReview];
            auto target=racing::dune::center(game->track,villageReview);
            game->camera->transform.position=target-game->track.right(sample)*65.f-game->track.tangent(sample)*42.f+engine::Vec3{0,32,0};
            game->camera->lookAt(target+engine::Vec3{0,3,0});
        }
        renderer->render();
        if(const auto* marker=game->nextGateMarker().actor()) {
            const auto& p=marker->transform.position;
            publish_gate_marker(marker->enabled,game->race.nextGate,p.x,p.y,p.z,marker->transform.rotation.y,game->nextGateMarker().mountHeight());
        }
        if(game->allSceneArtLoaded())ready_status(renderer->capabilities().backend == engine::RenderBackend::WebGl2);
        if(auto* shield=game->player().getComponent<racing::ShieldEffect>())
            publish_shield_visual(shield->visibility(),shield->breakAmount(),shield->activeImpacts(),shield->enabled);
        begin_wheels();
        const auto& drivetrain=game->playerDrivetrain();
        for(int i=0;i<4;++i)if(auto* visual=game->findSavedNode(drivetrain.visuals()[i])) {
            auto pose=game->physics().presentationMatrix(*visual);
            auto center=engine::transformPoint(pose,{});
            auto axle=engine::transformDirection(pose,{1,0,0}),up=engine::transformDirection(pose,{0,1,0});
            const auto& wheel=drivetrain.state().wheels[i];
            publish_wheel(visual->name.c_str(),wheel.rotation,wheel.steerAngle,wheel.angularVelocity,
                center.x,center.y,center.z,axle.x,axle.y,axle.z,up.x,up.y,up.z,
                wheel.suspensionLength,drivetrain.definition()->wheels[i].suspensionMin,
                drivetrain.definition()->wheels[i].suspensionMax,wheel.contact);
        }
        auto p=game->player().transform.position;
        begin_standings(game->classificationFinal());
        for(int id=0;id<racing::StarterScene::racerCount;++id){const auto& ai=game->aiState(id);
            publish_ai(static_cast<int>(game->difficulty()),id,ai.targetSpeed,ai.lane,ai.desiredLane,ai.brake,ai.following,ai.passing);}
        for(const auto& r:game->standings())
            publish_racer(static_cast<int>(game->racerChassis(r.id)),r.id,r.position,r.name,static_cast<int>(r.kind),r.location.x,r.location.z,
                r.progress.completedLaps,r.progress.nextGate,r.progress.finished,r.progress.elapsed,r.progress.recoveries,
                r.combat.charge,r.combat.cooldown,r.combat.shield,r.combat.disruption,r.combat.shots,r.combat.hits,
                r.combat.blocks,r.combat.received,r.combat.shieldUses,r.combat.releaseRequired);
        publish_target(game->phase==racing::Phase::Racing?game->weaponTarget(0):-1);
        publish(static_cast<int>(game->selectedChassis()),static_cast<int>(game->phase),static_cast<int>(game->kind),app.state()==engine::ApplicationState::Paused,
            game->race.completedLaps,game->race.nextGate,game->race.elapsed,game->race.bestLap,game->race.lastLap,
            game->countdown,game->speed(),game->battery,p.x,p.y,p.z,game->heading(),game->race.recoveries,game->boosting,game->race.routeValid);
    }
    performance_frame(frameStart,emscripten_get_now()-frameStart,
        !suspended && game->allSceneArtLoaded());
} catch(const std::exception& error) {
    emscripten_log(EM_LOG_ERROR,"Racing frame failed: %s",error.what());
    throw;
}
}
extern "C" {
EMSCRIPTEN_KEEPALIVE int race_load_trial(int track,int chassis,const char* bytes,int count) {
    if(!game||!bytes||count<=0||count>static_cast<int>(racing::TrialRun::maxBytes))return 0;
    return game->restoreTrial(track,chassis,std::string_view(bytes,static_cast<std::size_t>(count)));
}
EMSCRIPTEN_KEEPALIVE int race_quality(int preset,int scale) {
    try { racing::performanceQuality(preset,scale); }
    catch(const std::invalid_argument&) {return 0;}
    qualityPreset=preset;resolutionPercent=scale;qualityPending=true;return 1;
}
EMSCRIPTEN_KEEPALIVE void race_input(int action,int source,float value) {
    if(action<100 || action>106 || source<0 || source>2048 || !std::isfinite(value))return;
    engine::InputPacket packet;packet.id=action;packet.source=source;packet.x=std::clamp(value,-1.f,1.f);
    app.submitInput({&packet,1});
}
EMSCRIPTEN_KEEPALIVE int race_paint(int chassis,int rgb) {
    return game&&game->setPaint(chassis,rgb);
}
EMSCRIPTEN_KEEPALIVE void race_action(int action) {
    if(raceAudio&&!hidden()&&(action==1||action==2||action==3||action==7||action==12||action==13||
        action==60||(action==5&&app.state()==engine::ApplicationState::Paused)))raceAudio->activate();
    if(commands.size()<16)commands.push_back(action);
}
EMSCRIPTEN_KEEPALIVE int race_load_audio(unsigned id,unsigned rate,unsigned channels,const float* samples,unsigned count) {
    try {
        if(!raceAudio||!samples||count==0||count>1920000)throw std::runtime_error("Invalid audio samples");
        raceAudio->install(id,engine::SoundAsset::create(rate,channels,{samples,count}));return 1;
    }catch(const std::exception&){return 0;}
}
EMSCRIPTEN_KEEPALIVE void race_audio_activate() {
    if(raceAudio&&!hidden()&&app.state()==engine::ApplicationState::Running)raceAudio->activate();
}
EMSCRIPTEN_KEEPALIVE void race_audio_suspend(){if(raceAudio)raceAudio->suspend();}
EMSCRIPTEN_KEEPALIVE void race_audio_volumes(float master,float engines,float effects,float cues,int muted) {
    if(raceAudio)try{raceAudio->volumes(master,engines,effects,cues,muted!=0);}catch(const std::exception&){}
}
EMSCRIPTEN_KEEPALIVE void race_cancel() { if(game)clearInput(); }
EMSCRIPTEN_KEEPALIVE int race_load_desert_art(int model,const unsigned char* bytes,int count) {
    try {
        if(!game||!game->isDesert()||count<=0)throw std::runtime_error("Missing desert asset");
        game->installDesertArt(static_cast<unsigned>(model),{reinterpret_cast<const std::byte*>(bytes),static_cast<std::size_t>(count)},renderer->capabilities().backend!=engine::RenderBackend::WebGl2);
        return 1;
    }catch(const std::exception& error){car_error(error.what());return 0;}
}
EMSCRIPTEN_KEEPALIVE int race_load_car(const unsigned char* bytes,int count) {
    try {
        if(!game || count<=0)throw std::runtime_error("Missing car asset");
        game->installPlayerCar({reinterpret_cast<const std::byte*>(bytes),static_cast<std::size_t>(count)});
        return 1;
    } catch(const std::exception& error) { car_error(error.what());return 0; }
}
EMSCRIPTEN_KEEPALIVE int race_load_vehicle(int chassis,const unsigned char* bytes,int count) {
    try {
        if(!game||count<=0)throw std::runtime_error("Missing vehicle asset");
        game->installVehicleArt(static_cast<racing::Chassis>(chassis),{reinterpret_cast<const std::byte*>(bytes),static_cast<std::size_t>(count)});
        return 1;
    }catch(const std::exception& error){car_error(error.what());return 0;}
}
EMSCRIPTEN_KEEPALIVE int race_load_weapon(int,const unsigned char* bytes,int count) {
    try {
        if(!game||count<=0)throw std::runtime_error("Missing weapon asset");
        game->installWeaponArt({reinterpret_cast<const std::byte*>(bytes),static_cast<std::size_t>(count)});
        return 1;
    }catch(const std::exception& error){car_error(error.what());return 0;}
}
EMSCRIPTEN_KEEPALIVE int race_load_building(int model,const unsigned char* bytes,int count) {
    try {
        if(!game||count<=0)throw std::runtime_error("Missing building asset");
        game->installBuildingArt(static_cast<unsigned>(model),{reinterpret_cast<const std::byte*>(bytes),static_cast<std::size_t>(count)});
        return 1;
    }catch(const std::exception& error){car_error(error.what());return 0;}
}
EMSCRIPTEN_KEEPALIVE int race_load_street(int model,const unsigned char* bytes,int count) {
    try {
        if(!game||count<=0)throw std::runtime_error("Missing street asset");
        game->installStreetArt(static_cast<unsigned>(model),{reinterpret_cast<const std::byte*>(bytes),static_cast<std::size_t>(count)});
        return 1;
    }catch(const std::exception& error){car_error(error.what());return 0;}
}
EMSCRIPTEN_KEEPALIVE int race_load_city_kit(int model,const unsigned char* bytes,int count) {
    try {
        if(!game||count<=0)throw std::runtime_error("Missing city-kit asset");
        game->installCityKit(static_cast<unsigned>(model),{reinterpret_cast<const std::byte*>(bytes),static_cast<std::size_t>(count)});
        return 1;
    }catch(const std::exception& error){car_error(error.what());return 0;}
}
EMSCRIPTEN_KEEPALIVE int race_load_track_art(int model,const unsigned char* bytes,int count) {
    try {
        if(!game||count<=0)throw std::runtime_error("Missing track-kit asset");
        game->installTrackArt(static_cast<unsigned>(model),{reinterpret_cast<const std::byte*>(bytes),static_cast<std::size_t>(count)});
        return 1;
    }catch(const std::exception& error){car_error(error.what());return 0;}
}
EMSCRIPTEN_KEEPALIVE int race_load_gate_marker(int,const unsigned char* bytes,int count) {
    try {
        if(!game||count<=0)throw std::runtime_error("Missing next-gate beacon");
        game->installGateMarker({reinterpret_cast<const std::byte*>(bytes),static_cast<std::size_t>(count)});
        return 1;
    } catch(const std::exception& error) {car_error(error.what());return 0;}
}
EMSCRIPTEN_KEEPALIVE int race_load_plaza_art(int model,const unsigned char* bytes,int count) {
    try {
        if(!game||count<=0)throw std::runtime_error("Missing plaza asset");
        game->installPlazaArt(static_cast<unsigned>(model),{reinterpret_cast<const std::byte*>(bytes),static_cast<std::size_t>(count)});
        return 1;
    }catch(const std::exception& error){car_error(error.what());return 0;}
}
}
int main() {
    auto audioBackend=std::make_unique<engine::BrowserAudioBackend>();const auto audioDevice=audioBackend->deviceId();
    raceAudio=std::make_unique<racing::sound::RaceAudio>(std::move(audioBackend));
    auto scene = std::make_unique<racing::StarterScene>(selected_track()?racing::TrackKind::DesertSolar:racing::TrackKind::Kairox);
    auto* selectedScene = scene.get();
    game=selectedScene;
    char saved[16385]{};int loaded=read_progress(saved);
    if(loaded<0||(loaded>0&&!game->restoreProgression(saved)))invalid_progress();
    game->setDifficulty(selected_difficulty());
    read_trials();game->setTimeTrial(selected_mode()!=0);
    for(auto p:game->track.points)track_point(p.x,p.z);
    const auto& ground=game->cityGround();
    publish_ground(ground.promenadeSegments,ground.drainSegments,ground.foundations,ground.loadingDocks,
        ground.utilityClusters,ground.serviceVehicles,ground.contactShadows,ground.lightPools,ground.dampPatches,ground.meshes,ground.triangles);
    auto id = app.loadScene(std::move(scene));
    app.start(id);
    renderer = std::make_unique<engine::Renderer>(*selectedScene, *selectedScene->camera);
    renderer->initialize(static_cast<engine::RenderBackendPreference>(selected_backend()));
    init_audio(audioDevice);
    for(unsigned i=0;i<racing::sound::assets.size();++i){const auto& a=racing::sound::assets[i];load_audio_clip(i,a.name,a.hash);}
    read_quality();
    load_car(racing::cybercar::sha256);
    for(int i=0;i<3;++i){const auto& model=racing::vehicleart::models[i];load_vehicle(i==0?2:i==1?1:3,model.name,model.hash);}
    load_weapon(racing::weaponart::hash);
    if(game->isDesert()) {
        for(unsigned i=0;i<racing::desert::models.size();++i){const auto& model=racing::desert::models[i];load_desert_art(i,model.name,model.hash);}
        for(unsigned i=0;i<racing::desertrace::models.size();++i){const auto& model=racing::desertrace::models[i];load_desert_art(racing::desert::models.size()+i,model.name,model.hash);}
        for(unsigned i=0;i<racing::dune::models.size();++i){const auto& model=racing::dune::models[i];load_desert_art(racing::desert::models.size()+racing::desertrace::models.size()+i,model.name,model.hash);}
        for(const auto& p:game->duneVillagePlacements())village_placement(p.model,p.village,p.position.x,p.position.y,p.position.z,p.yaw);
        for(const auto& p:game->desertRacePlacements())desert_race_placement(p.model,p.position.x,p.position.y,p.position.z,p.yaw,p.scale.x,p.scale.y,p.scale.z,p.gate);
    } else {
    for(unsigned i=0;i<racing::buildingart::models.size();++i){const auto& model=racing::buildingart::models[i];load_building(i,model.name,model.hash);}
    for(const auto& b:game->buildingPlacements())building_placement(b.model,b.position.x,b.position.z,b.yaw,b.radius);
    for(unsigned i=0;i<racing::citykit::models.size();++i){const auto& model=racing::citykit::models[i];load_city_kit(i,model.name,model.hash);}
    for(const auto& p:game->cityKitPlacements())city_kit_placement(p.model,p.position.x,p.position.y,p.position.z,p.yaw,p.radius,p.scale);
    for(int i=0;i<12;++i){const auto& model=racing::streetkit::models[i];load_street(i,model.name,model.hash);}
    for(const auto& p:game->streetPlacements())street_placement(p.model,p.position.x,p.position.y,p.position.z,p.yaw);
    for(unsigned i=0;i<racing::trackkit::models.size();++i){const auto& model=racing::trackkit::models[i];load_track_art(i,model.name,model.hash);}
    for(const auto& p:game->trackPlacements())track_placement(p.model,p.position.x,p.position.y,p.position.z,p.yaw,p.marking);
    for(unsigned i=0;i<racing::plaza::models.size();++i){const auto& model=racing::plaza::models[i];load_plaza_art(i,model.name,model.hash);}
    }
    load_gate_marker(racing::gateart::name,racing::gateart::hash);
    emscripten_set_main_loop(frame, 0, true);
}
