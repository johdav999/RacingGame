#include "time_trial.hpp"
#include <iostream>
#include <stdexcept>
using namespace racing;
void check(bool b,const char* message){if(!b)throw std::runtime_error(message);}
TrialRun fixture() {
    TrialRun r;r.duration=3;r.splits={1,2,3};
    for(int i=0;i<=60;i++)r.frames.push_back({i*.05,{float(i),1,0},{0,i<30?3.1f:-3.1f,0}});
    return r;
}
int main(){try{
    auto r=fixture();check(r.valid(),"valid fixture");
    auto bytes=r.encode();auto copy=TrialRun::decode(bytes,0,0);
    check(copy&&copy->encode()==bytes,"round trip keeps time, sectors and poses");
    check(!TrialRun::decode(bytes,1,0)&&!TrialRun::decode(bytes,0,1),"records cannot cross track/chassis keys");
    check(!TrialRun::decode(bytes+" garbage",0,0)&&!TrialRun::decode("NATT2"+bytes.substr(5),0,0),"version and trailing bytes rejected");
    check(!TrialRun::decode(bytes.substr(0,bytes.size()-10),0,0),"truncated record rejected");
    check(std::abs(r.at(1.475).position.x-29.5f)<.001f&&std::abs(r.at(1.475).rotation.y-engine::pi)<.001f,"interpolation crosses yaw seam the short way");
    check(r.at(-1).time==0&&r.at(10).time==3,"replay endpoint clamp");
    r.frames[2].time=r.frames[1].time;check(!r.valid(),"duplicate timestamps rejected");
    r=fixture();r.frames[3].position.x=std::numeric_limits<float>::infinity();check(!r.valid(),"nonfinite poses rejected");
    r=fixture();r.splits[1]=r.splits[0];check(!r.valid(),"unordered splits rejected");
    r=fixture();r.frames.pop_back();check(!r.valid(),"missing finish pose rejected");
    r=fixture();r.frames.erase(r.frames.begin()+10,r.frames.begin()+20);check(!r.valid(),"missing samples rejected");
    for(auto track:{TrackKind::Kairox,TrackKind::DesertSolar}) {
        Circuit c(track);RaceProgress progress;TrialRecorder recorder;
        Vec3 previous=c.gate(0)+c.gateForward(0)*2;previous.y=1;
        recorder.begin(int(track),0,previous,{});
        // Route fixture exercises the ordered checkpoint and sector pipeline,
        // independent of physics (physical laps are tested in racing_trial).
        for(int i=1;i<=193;i++) {
            auto p=c.point(i);p.y=1;auto before=progress;
            progress.step(c,previous,p,.05);recorder.observe(before,progress,p,{});previous=p;
            if(progress.completedLaps)break;
        }
        check(recorder.clean&&recorder.run.valid(),"clean ordered lap records all sectors and finish");
        auto before=progress;progress.recover();recorder.observe(before,progress,previous,{});
        check(!recorder.clean,"recovery invalidates whole attempt");
        recorder.begin(int(track),0,previous,{});progress.reset();before=progress;progress.routeValid=false;
        recorder.observe(before,progress,previous,{});progress.routeValid=true;
        recorder.observe(before,progress,previous,{});check(!recorder.clean,"leaving circuit is sticky even after returning");
    }
    std::cout<<"Time trial codec, interpolation, sectors and invalidation passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
