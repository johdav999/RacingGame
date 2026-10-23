#pragma once
#include "engine/audio.hpp"
#include "race_audio_contract.hpp"
#include <array>
#include <vector>

namespace racing::sound {
enum Clip : unsigned { Vektor,Skimmer,Bastion,Vector,Tires,Boost,Laser,ShieldOn,ShieldHit,HullHit,Tick,Go,Finish,Count };
struct Racer {
    engine::Vec3 position{};
    unsigned chassis=0;
    float speed=0,rpm=1000,throttle=0,slip=0;
    bool boost=false,grounded=false;
    int shots=0,blocks=0,received=0,shieldUses=0,impacts=0,recoveries=0;
};
struct Frame {
    std::array<Racer,6> racers{};
    engine::AudioListenerPose listener;
    std::uint64_t epoch=0;
    int phase=0; // Garage, countdown, racing, finished: presentation snapshot only.
    double countdown=3;
    bool paused=false;
    int activeRacers=6;
};
class RaceAudio {
public:
    explicit RaceAudio(std::unique_ptr<engine::AudioBackend>);
    void install(unsigned,std::shared_ptr<const engine::SoundAsset>);
    bool ready() const;
    void update(const Frame&,double dt);
    void activate(); // Host gesture only; caller rejects hidden/paused requests.
    void suspend(); // Immediate visibility/page-lifecycle boundary.
    void volumes(float master,float engines,float effects,float cues,bool muted);
    engine::AudioSystem& system() {return audio;}
    unsigned loaded() const;
    const auto& starts() const {return started;}
    float playerPitch() const {return loops[0].pitch;}
private:
    struct Loops {engine::AudioVoiceId motor{},boost{};unsigned chassis=Count;float pitch=1,gain=0;};
    engine::AudioSystem audio;
    engine::AudioBusId engines,effects,cues;
    std::array<std::shared_ptr<const engine::SoundAsset>,Count> clips{};
    std::array<Loops,6> loops{};
    std::array<unsigned,Count> started{};
    std::vector<engine::AudioVoiceId> oneShots;
    engine::AudioVoiceId tire{};
    Frame previous{};
    bool initialized=false,paused=false;
    void stopLoops();
    void stopShots();
    engine::AudioVoiceId play(unsigned,float,bool,int,const Frame&,float rate=1);
    void shot(unsigned,float,int,const Frame&);
};
}
