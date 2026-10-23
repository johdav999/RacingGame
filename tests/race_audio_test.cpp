#include "race_audio.hpp"
#include "starter_scene.hpp"
#include "engine/application.hpp"
#include <iostream>
#include <map>
#include <stdexcept>
using namespace engine;
using namespace racing::sound;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
struct Backend final:AudioBackend {
    struct Voice {AudioPlayOptions options;float gain;};
    std::map<unsigned,Voice> voices;bool enabled=false,held=false;AudioListenerPose pose;
    bool start(unsigned id,const SoundAsset&,const AudioPlayOptions& options,float gain) override {voices.emplace(id,Voice{options,gain});return true;}
    void stop(unsigned id) noexcept override {voices.erase(id);}
    bool playing(unsigned id) const noexcept override {return voices.contains(id);}
    void gain(unsigned id,float gain) noexcept override {if(voices.contains(id))voices[id].gain=gain;}
    void position(unsigned id,Vec3 p) noexcept override {if(voices.contains(id))voices[id].options.position=p;}
    bool playbackRate(unsigned id,float rate) noexcept override {if(!voices.contains(id))return false;voices[id].options.playbackRate=rate;return true;}
    void listener(const AudioListenerPose& p) noexcept override {pose=p;}
    bool active() const noexcept override {return enabled&&!held;}
    void activate() noexcept override {enabled=true;}
    void suspend(bool value) noexcept override {held=value;if(value)enabled=false;}
};
void controls() {
    auto backend=std::make_unique<Backend>();auto* device=backend.get();RaceAudio audio(std::move(backend));
    std::array<float,2400> pcm{};for(unsigned i=0;i<pcm.size();++i)pcm[i]=.1f*std::sin(float(i)*.1f);
    auto clip=SoundAsset::create(24000,1,pcm);
    for(unsigned i=0;i<Count;++i)audio.install(i,clip);
    bool rejected=false;try{audio.install(0,clip);}catch(const std::exception&){rejected=true;}
    require(rejected&&audio.ready(),"Reject duplicate imports without losing valid sounds");
    Frame f;f.epoch=1;for(unsigned i=0;i<6;++i){f.racers[i].chassis=i%4;f.racers[i].position={float(i*10),0,0};}
    audio.update(f,.016);require(audio.system().voiceCount()==0,"No autoplay before activation");
    audio.activate();audio.update(f,.016);require(audio.system().voiceCount()==1,"Garage previews only the selected engine");
    f.epoch=2;f.phase=1;audio.update(f,.016);
    require(audio.starts()[Tick]==1&&audio.system().voiceCount()==7,"One initial beep and six engines");
    f.countdown=2;audio.update(f,.016);audio.update(f,.016);f.countdown=1;audio.update(f,.016);
    require(audio.starts()[Tick]==3,"Exactly one beep per countdown number");
    f.phase=2;audio.update(f,.016);require(audio.starts()[Go]==1,"Single race-start signal");
    auto before=audio.starts()[Vektor];float idle=audio.playerPitch();
    f.racers[0].rpm=5500;f.racers[0].throttle=1;f.racers[0].speed=30;
    for(int i=0;i<40;++i)audio.update(f,.016);
    require(audio.playerPitch()>idle+.5&&audio.starts()[Vektor]==before,"RPM changes pitch without restarting loops");
    bool spatial=false;for(const auto& [id,v]:device->voices)if(v.options.spatial&&v.options.position.x>0)spatial=true;
    require(spatial,"Opponent voices have world-space positions");
    f.racers[0].boost=true;f.racers[0].grounded=true;f.racers[0].slip=4;
    f.racers[0].shots=1;f.racers[0].blocks=1;f.racers[0].received=1;f.racers[0].shieldUses=1;
    audio.update(f,.016);audio.update(f,.016);
    for(unsigned id:{Boost,Tires,Laser,ShieldOn,ShieldHit,HullHit})require(audio.starts()[id]==1,"Confirmed events produce one sound, loops start once");
    f.racers[0].boost=false;f.racers[0].slip=0;audio.update(f,.1);
    audio.volumes(0,.5f,.6f,.7f,false);for(const auto& [id,v]:device->voices)require(v.gain==0,"Master zero silences every voice");
    audio.volumes(.65f,.7f,.8f,.85f,true);for(const auto& [id,v]:device->voices)require(v.gain==0,"Mute applies across every bus");
    audio.volumes(.65f,.7f,.8f,.85f,false);
    auto starts=audio.starts();f.paused=true;audio.update(f,.016);require(!device->active(),"Pause suspends device clock");
    f.racers[0].shots+=5;audio.update(f,.016);f.paused=false;audio.update(f,.016);
    require(!device->active(),"Returning from suspension does not autoplay");
    audio.activate();audio.update(f,.016);require(audio.starts()[Laser]==starts[Laser],"No stale shot burst on resume");
    f.racers[0].recoveries++;f.racers[0].impacts++;audio.update(f,.016);
    require(audio.starts()[HullHit]==starts[HullHit],"Recovery does not invent collision audio");
    for(int j=0;j<100;++j){for(auto& r:f.racers){++r.shots;++r.blocks;}audio.update(f,.016);}
    require(audio.system().voiceCount()<=32,"Sustained combat obeys voice budget");
    f.phase=3;audio.update(f,.016);audio.update(f,.016);
    require(audio.starts()[Finish]==1,"Finish cue plays once");
    for(const auto& [id,v]:device->voices)require(!v.options.loop,"Finished races stop engine and boost loops");
    f.epoch++;f.phase=0;audio.update(f,.016);require(audio.system().voiceCount()==1,"Garage transition clears race voices");
    f.racers[0].chassis=Bastion;f.epoch++;audio.update(f,.016);
    require(audio.system().voiceCount()==1&&audio.starts()[Bastion]>0,"Vehicle selection replaces its engine sound");
    audio.suspend();audio.update(f,.016);require(!device->active(),"Visibility suspension needs a new gesture");
}
void telemetry() {
    Application app;auto scene=std::make_unique<racing::StarterScene>(racing::TrackKind::DesertSolar);auto* game=scene.get();
    app.start(app.loadScene(std::move(scene)));app.advance(0);auto before=game->audioFrame();
    game->selectChassis(racing::Chassis::Vector);require(game->audioFrame().racers[0].chassis==Vector,"Selected chassis maps to matching sound");
    game->start();require(game->audioFrame().epoch!=before.epoch,"Restart provides a new audio epoch");
    double clock=0;for(int i=0;i<210;++i){game->input={1,0,0,0,1,1,1};clock+=1./60;app.advance(clock);}
    auto f=game->audioFrame();require(f.phase==2&&f.racers[0].shots>0&&f.racers[0].shieldUses>0&&f.racers[0].boost,"Audio reads real simulation events");
    require(f.racers[0].speed>0&&f.racers[0].throttle==1,"Driving audio uses real body velocity and input");
    game->recover();require(game->audioFrame().racers[0].recoveries==1,"Recovery reaches the audio snapshot");
}
int main(){try{controls();telemetry();std::cout<<"Racing audio activation, mix, spatial playback, RPM, combat, lifecycle and real telemetry passed\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
