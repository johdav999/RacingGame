#pragma once
#include "race_rules.hpp"
#include "racing_ai.hpp"
#include "time_trial.hpp"
#include "combat_rules.hpp"
#include "pulse_weapon.hpp"
#include "laser_beam.hpp"
#include "shield_effect.hpp"
#include "progression.hpp"
#include "vehicle_paint.hpp"
#include "building_art_contract.hpp"
#include "street_kit.hpp"
#include "city_kit_contract.hpp"
#include "city_ground.hpp"
#include "track_kit.hpp"
#include "market_plaza.hpp"
#include "gate_marker.hpp"
#include "desert_solar.hpp"
#include "desert_race_kit.hpp"
#include "dune_villages.hpp"
#include "race_audio.hpp"
#include "engine/scene.hpp"
#include "engine/camera_follow.hpp"
#include "engine/rigid_body_component.hpp"
#include "engine/vehicle_component.hpp"
#include "engine/mesh_component.hpp"
#include <algorithm>

namespace racing {
enum class VehicleKind { Car, Hover };
enum class Chassis { Vektor, Skimmer, Bastion, Vector };
inline VehicleKind chassisKind(Chassis c) { return c==Chassis::Vektor||c==Chassis::Bastion?VehicleKind::Car:VehicleKind::Hover; }
inline const char* chassisName(Chassis c) {
    constexpr const char* names[]={"Vektor","Skimmer","Bastion","Vector"};return names[static_cast<int>(c)];
}
enum class Phase { Ready, Countdown, Racing, Finished };
struct DrivingInput { float throttle=0,steer=0,brake=0,slide=0,boost=0,fire=0,shield=0; };
struct VehicleRuntime {
    engine::Actor* actor=nullptr;
    engine::RigidBodyComponent* body=nullptr;
    engine::VehicleComponent* wheels=nullptr;
    engine::MeshComponent* shell=nullptr;
    VehicleKind kind=VehicleKind::Car;
    Chassis chassis=Chassis::Vektor;
    RaceProgress progress;
    Vec3 previous{};
    float steering=0,battery=100;
    bool boosting=false;
    float audioThrottle=0;
    unsigned audioImpacts=0;
    double lastAudioImpact=-1;
    double offTrack=0,stalled=0;
    CombatState combat;
    VehicleUpgrades upgrades;
    ShieldEffect* shieldEffect=nullptr;
    LaserBeam* beam=nullptr;
    PulseWeapon* weapon=nullptr;
};
struct Standing {
    int id=0,position=1;
    const char* name="";
    VehicleKind kind=VehicleKind::Car;
    Vec3 location{};
    RaceProgress progress;
    CombatState combat;
};
class StarterScene final : public engine::Scene {
public:
    static constexpr int racerCount=6;
    explicit StarterScene(TrackKind selected=TrackKind::Kairox);
    ~StarterScene() override;
    void select(VehicleKind kind);
    void selectChassis(Chassis chassis);
    Chassis selectedChassis() const { return racer(0).chassis; }
    Chassis racerChassis(int id) const { return racer(id).chassis; }
    void start();
    bool enterLeague(int league);
    bool startLeagueRace();
    bool leagueRace() const {return leagueRace_;}
    bool leagueFinal() const {return leagueFinal_;}
    bool setDifficulty(int value);
    ai::Difficulty difficulty() const {return difficulty_;}
    const ai::State& aiState(int id) const {return aiStates.at(id);}
    void setTimeTrial(bool enabled);
    void watchBest();
    bool timeTrial() const {return trialMode;}
    bool watchingReplay() const {return replay;}
    bool trialClean() const {return recorder.clean;}
    bool newPersonalBest() const {return newBest;}
    const auto& trialSplits() const {return recorder.run.splits;}
    const TrialRun* personalBest() const;
    const TrialRun* comparisonRun() const {return reference?&*reference:nullptr;}
    bool restoreTrial(int track,int chassis,std::string_view bytes);
    unsigned trialRevision() const {return bestRevision;}
    const engine::Actor* ghostActor() const {return ghost;}
    bool purchase(int system);
    bool setPaint(int chassis,int rgb);
    bool restoreProgression(std::string_view save);
    const Progression& progression() const { return career; }
    int raceReward() const { return reward; }
    const VehicleUpgrades& activeUpgrades() const {return racer(0).upgrades;}
    bool awaitingResults() const { return phase==Phase::Finished&&!classificationFinal(); }
    void recover();
    void cancelInput();
    void fixedStep(double dt);
    void present(double dt,float aspect);
    // Installs the original GLB once, preserving its named part ranges and textures.
    void installPlayerCar(std::span<const std::byte> bytes);
    void installVehicleArt(Chassis chassis,std::span<const std::byte> bytes);
    void installWeaponArt(std::span<const std::byte> bytes);
    bool weaponArtLoaded() const { return weaponLoaded; }
    bool vehicleArtLoaded(Chassis c) const { return arts[static_cast<int>(c)].loaded; }
    bool allVehicleArtLoaded() const { return carLoaded&&arts[1].loaded&&arts[2].loaded&&arts[3].loaded; }
    void installBuildingArt(unsigned model,std::span<const std::byte> bytes);
    void installTrackArt(unsigned model,std::span<const std::byte> bytes) {trackKit.install(*this,model,bytes);}
    const auto& trackPlacements() const {return trackKit.placements();}
    bool allTrackArtLoaded() const {return trackKit.ready();}
    void installPlazaArt(unsigned model,std::span<const std::byte> bytes) {marketPlaza.install(*this,track,model,bytes);}
    void installGateMarker(std::span<const std::byte> bytes) {gateMarker.install(*this,bytes);}
    const GateMarker& nextGateMarker() const {return gateMarker;}
    void installDesertArt(unsigned model,std::span<const std::byte> bytes,bool emissiveMaps=true) {
        if(model<desert::models.size())desertKit.install(*this,model,bytes);
        else if(model<desert::models.size()+desertrace::models.size())desertRaceKit.install(*this,model-desert::models.size(),bytes,emissiveMaps);
        else duneVillages.install(*this,model-desert::models.size()-desertrace::models.size(),bytes);
    }
    const auto& duneVillagePlacements() const {return duneVillages.placements();}
    const auto& desertRacePlacements() const {return desertRaceKit.placements();}
    const auto& desertPlacements() const {return desertKit.placements();}
    bool allDesertArtLoaded() const {return desertKit.ready()&&desertRaceKit.ready()&&duneVillages.ready();}
    bool isDesert() const {return desertTrack(track.kind);}
    bool allSceneArtLoaded() const { return weaponLoaded&&allVehicleArtLoaded()&&gateMarker.ready()&&(isDesert()?allDesertArtLoaded():std::all_of(buildingsLoaded.begin(),buildingsLoaded.end(),[](bool loaded){return loaded;})&&allCityKitLoaded()&&streetKit.ready()&&trackKit.ready()&&marketPlaza.ready()); }
    void installStreetArt(unsigned model,std::span<const std::byte> bytes) {streetKit.install(*this,model,bytes);}
    const std::vector<streetkit::Placement>& streetPlacements() const {return streetKit.placements();}
    void installCityKit(unsigned model,std::span<const std::byte> bytes);
    bool allCityKitLoaded() const;
    const cityground::Report& cityGround() const {return cityGround_;}
    const std::vector<citykit::Placement>& cityKitPlacements() const { return cityDetails; }
    const std::vector<buildingart::Placement>& buildingPlacements() const { return buildings; }
    bool playerCarLoaded() const { return carLoaded; }
    const engine::VehicleComponent& playerDrivetrain() const { return *vehicles[0].wheels; }
    engine::Actor& player() const { return *vehicles[static_cast<int>(kind)].actor; }
    engine::RigidBodyComponent& body() const { return *vehicles[static_cast<int>(kind)].body; }
    std::array<Standing,racerCount> standings() const;
    bool classificationFinal() const;
    int weaponTarget(int id) const;
    const LaserBeam& laser(int id=0) const {return *racer(id).beam;}
    float speed() const { return replay?0:magnitude(body().linearVelocity())*3.6f; }
    float heading() const;
    sound::Frame audioFrame(bool paused=false) const;
    engine::CameraComponent* camera=nullptr;
    Circuit track;
    RaceProgress race;
    DrivingInput input;
    VehicleKind kind=VehicleKind::Car;
    Phase phase=Phase::Ready;
    double countdown=3,worldTime=0;
    float battery=100;
    bool boosting=false;
private:
    struct VehicleArt {
        bool loaded=false;
        std::vector<std::shared_ptr<engine::Mesh>> meshes;
        std::vector<std::string> names;
        std::shared_ptr<const engine::VehicleDefinition> definition;
    };
    void applyArt(VehicleRuntime& vehicle);
    void applyPaint(VehicleRuntime& vehicle);
    std::array<PaintSurface,4> paintSurfaces;
    std::array<std::shared_ptr<const engine::Texture>,4> paintedTextures;
    void tune(VehicleRuntime& vehicle);
    std::array<VehicleArt,4> arts;
    void placeAtGate(int gate);
    cityground::Report cityGround_;
    void buildCity();
    void buildDesert();
    void planBuildings();
    void planCityKit();
    void buildCityKitSupports();
    std::vector<citykit::Placement> cityDetails;
    std::array<bool,citykit::models.size()> cityKitLoaded{};
    std::vector<buildingart::Placement> buildings;
    streetkit::Kit streetKit;
    trackkit::Kit trackKit;
    plaza::Kit marketPlaza;
    desert::Kit desertKit;
    desertrace::Kit desertRaceKit;
    dune::Kit duneVillages;
    std::array<bool,buildingart::models.size()> buildingsLoaded{};
    void buildVehicles();
    void resetGrid();
    void applyUpgrades();
    void restoreLeagueTuning();
    std::array<std::shared_ptr<const engine::VehicleDefinition>,racerCount+1> leagueDefinitions_{};
    void place(VehicleRuntime& vehicle,int gate,float lane=0,float distance=2);
    void drive(VehicleRuntime& vehicle,DrivingInput command,double dt);
    DrivingInput pilot(int id,double dt);
    ai::Planner aiPlanner;
    ai::Difficulty difficulty_=ai::Difficulty::Normal;
    std::array<ai::State,racerCount> aiStates{};
    void firePulse(int id);
    VehicleRuntime& racer(int id) {return vehicles[id==0?static_cast<int>(kind):id+1];}
    const VehicleRuntime& racer(int id) const {return vehicles[id==0?static_cast<int>(kind):id+1];}
    std::array<VehicleRuntime,racerCount+1> vehicles{};
    GateMarker gateMarker;
    std::unique_ptr<engine::CameraFollowRig> cameraRig;
    bool carLoaded=false;
    bool weaponLoaded=false;
    Progression career;
    int reward=0;
    bool rewardPaid=false;
    bool leagueRace_=false,leagueFinal_=false;
    int leagueRevision_=0;
    ai::Difficulty garageDifficulty_=ai::Difficulty::Normal;
    std::uint64_t audioEpoch=0;
    bool trialMode=false,replay=false,newBest=false;
    unsigned bestRevision=0;
    TrialRecorder recorder;
    std::array<std::optional<TrialRun>,trackCount*4> bestRuns;
    std::optional<TrialRun> reference;
    engine::Actor* ghost=nullptr;
    void buildGhost();
    void presentGhost();
};
}
