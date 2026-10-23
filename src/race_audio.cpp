#include "race_audio.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace racing::sound {
using namespace engine;
RaceAudio::RaceAudio(std::unique_ptr<AudioBackend> backend):audio(std::move(backend),32) {
    engines=audio.createBus("Engines");effects=audio.createBus("Effects");cues=audio.createBus("Race cues");
    volumes(.65f,.7f,.8f,.85f,false);
}
void RaceAudio::install(unsigned id,std::shared_ptr<const SoundAsset> sound) {
    if(id>=Count||clips[id]||!sound||sound->duration()<.05||sound->duration()>10||sound->channels()!=1)
        throw std::invalid_argument("Invalid racing sound asset");
    clips[id]=std::move(sound);
}
unsigned RaceAudio::loaded() const {return unsigned(std::count_if(clips.begin(),clips.end(),[](const auto& p){return bool(p);}));}
bool RaceAudio::ready() const {return loaded()==Count;}
void RaceAudio::volumes(float master,float motor,float fx,float ui,bool muted) {
    for(float v:{master,motor,fx,ui})if(!std::isfinite(v)||v<0||v>1)throw std::invalid_argument("Invalid audio volume");
    audio.setBusVolume(audio.masterBus(),master);audio.setBusMuted(audio.masterBus(),muted);
    audio.setBusVolume(engines,motor);audio.setBusVolume(effects,fx);audio.setBusVolume(cues,ui);
}
void RaceAudio::activate(){audio.backend().suspend(false);audio.backend().activate();paused=false;}
void RaceAudio::stopLoops(){for(auto& l:loops){audio.stop(l.motor);audio.stop(l.boost);l={};}audio.stop(tire);tire={};}
void RaceAudio::stopShots(){for(auto v:oneShots)audio.stop(v);oneShots.clear();}
void RaceAudio::suspend(){stopShots();audio.backend().suspend(true);paused=true;}
AudioVoiceId RaceAudio::play(unsigned clip,float gain,bool loop,int racer,const Frame& f,float rate) {
    AudioPlayOptions options;options.bus=clip<=Vector?engines:clip>=Tick?cues:effects;
    options.volume=gain;options.loop=loop;options.playbackRate=rate;
    if(racer>0){options.spatial=true;options.position=f.racers[racer].position;options.referenceDistance=6;options.maxDistance=90;options.rolloff=1.4f;}
    auto id=audio.play(clips[clip],options);if(id)++started[clip];return id;
}
void RaceAudio::shot(unsigned clip,float gain,int racer,const Frame& f) {
    if(racer>0) {
        auto d=f.racers[racer].position-f.listener.position;
        if(dot(d,d)>90*90)return;
    }
    std::erase_if(oneShots,[&](auto id){return !audio.isPlaying(id);});
    if(oneShots.size()>=12){audio.stop(oneShots.front());oneShots.erase(oneShots.begin());}
    if(auto id=play(clip,gain,false,racer,f))oneShots.push_back(id);
}
void RaceAudio::update(const Frame& f,double seconds) {
    float dt=float(std::clamp(seconds,0.0,.1));
    bool reset=!initialized||f.epoch!=previous.epoch;
    if(reset){stopLoops();stopShots();}
    if(f.phase==3)stopLoops();
    if(f.paused&&!paused)suspend();
    if(!f.paused&&paused){audio.backend().suspend(false);paused=false;}
    audio.setListenerPose(f.listener);audio.update(f.paused?0:dt);
    // Always consume snapshots, including muted/inactive/loading frames. Never
    // replay historical shots or missed countdown ticks after a late activation.
    if(f.paused||!ready()||!audio.backend().active()) {previous=f;initialized=true;return;}
    const bool changed=reset||f.phase!=previous.phase;
    if(f.phase==0&&changed){stopLoops();stopShots();}
    if(f.phase==1&&(changed||int(std::ceil(f.countdown))!=int(std::ceil(previous.countdown))))shot(Tick,.7f,-1,f);
    if(!reset&&changed&&f.phase==2&&previous.phase==1)shot(Go,.8f,-1,f);
    if(!reset&&changed&&f.phase==3){stopLoops();stopShots();shot(Finish,.75f,-1,f);}
    if(f.phase==3){previous=f;initialized=true;return;}
    const float blend=1-std::exp(-dt*8);
    for(unsigned i=0;i<loops.size();++i) {
        auto& l=loops[i];const auto& r=f.racers[i];
        if(i>=static_cast<unsigned>(f.activeRacers)||(f.phase==0&&i>0)){audio.stop(l.motor);audio.stop(l.boost);l={};continue;}
        if(r.chassis>Vector)continue;
        if(l.chassis!=r.chassis){audio.stop(l.motor);l.motor={};l.chassis=r.chassis;}
        const bool car=r.chassis==Vektor||r.chassis==Bastion;
        const float load=f.phase==2?std::abs(r.throttle):0;
        float pitch=car?.62f+std::clamp((r.rpm-900)/5100.f,0.f,1.f)*1.25f:
            .72f+std::clamp(r.speed/42,0.f,1.f)*.85f+load*.12f;
        l.pitch+=(pitch-l.pitch)*blend;
        float gain=(i==0?.28f:.16f)*(f.phase==0?.5f:.65f+load*.35f);
        l.gain+=(gain-l.gain)*blend;
        if(!audio.isPlaying(l.motor))l.motor=play(r.chassis,l.gain,true,i,f,l.pitch);
        audio.setVoicePlaybackRate(l.motor,l.pitch);audio.setVoiceVolume(l.motor,l.gain);audio.setVoicePosition(l.motor,r.position);
        bool boosting=f.phase==2&&r.boost;
        if(boosting&&!audio.isPlaying(l.boost))l.boost=play(Boost,i==0?.3f:.14f,true,i,f);
        if(!boosting&&l.boost){audio.stop(l.boost,.08);l.boost={};}
        if(l.boost)audio.setVoicePosition(l.boost,r.position);
        if(!reset&&f.phase==2&&r.recoveries==previous.racers[i].recoveries) {
            const auto& old=previous.racers[i];float level=i==0?.6f:.3f;
            if(r.shots>old.shots)shot(Laser,level,i,f);
            if(r.shieldUses>old.shieldUses)shot(ShieldOn,level,i,f);
            if(r.blocks>old.blocks)shot(ShieldHit,level,i,f);
            if(r.received>old.received||r.impacts>old.impacts)shot(HullHit,level,i,f);
        }
    }
    const auto& player=f.racers[0];
    bool sliding=f.phase==2&&(player.chassis==Vektor||player.chassis==Bastion)&&player.grounded&&player.speed>5&&player.slip>1.5f;
    if(sliding&&!audio.isPlaying(tire))tire=play(Tires,.1f,true,0,f);
    if(sliding)audio.setVoiceVolume(tire,std::clamp(player.slip*.045f,.05f,.3f));
    else if(tire){audio.stop(tire,.08);tire={};}
    previous=f;initialized=true;
}
}
