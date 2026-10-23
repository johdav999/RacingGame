#pragma once
#include "race_rules.hpp"
#include <iomanip>
#include <locale>
#include <optional>
#include <sstream>
#include <string_view>
#include <vector>

namespace racing {
// Version 1: stock chassis, standing start at gate zero +2m, one lap,
// sectors end at gates 4, 8 and 0. Change the format when these rules change.
struct GhostFrame { double time=0; Vec3 position{},rotation{}; };
struct TrialRun {
    static constexpr double maxSeconds=600;
    static constexpr std::size_t maxFrames=12002,maxBytes=1500000;
    int track=0,chassis=0;
    double duration=0;
    std::array<double,3> splits{}; // Cumulative times, so deltas compare the same gate.
    std::vector<GhostFrame> frames;
    GhostFrame at(double time) const {
        if(frames.empty())return {};
        auto hi=std::lower_bound(frames.begin(),frames.end(),time,[](const auto& f,double t){return f.time<t;});
        if(hi==frames.begin())return *hi;
        if(hi==frames.end())return frames.back();
        const auto& a=*(hi-1);const auto& b=*hi;
        const float u=static_cast<float>((time-a.time)/(b.time-a.time));
        auto angle=[u](float a,float b){return a+std::remainder(b-a,2*engine::pi)*u;};
        return {time,a.position+(b.position-a.position)*u,
            {angle(a.rotation.x,b.rotation.x),angle(a.rotation.y,b.rotation.y),angle(a.rotation.z,b.rotation.z)}};
    }
    bool valid() const {
        if(track<0||track>1||chassis<0||chassis>3||!std::isfinite(duration)||duration<=0||duration>maxSeconds||
            frames.size()<2||frames.size()>maxFrames||frames.front().time!=0||std::abs(frames.back().time-duration)>.00001)return false;
        double last=0;
        for(double s:splits){if(!std::isfinite(s)||s<=last||s>duration)return false;last=s;}
        if(std::abs(splits[2]-duration)>.00001)return false;
        last=-1;
        for(const auto& f:frames) {
            if(!std::isfinite(f.time)||f.time<=last||f.time>duration||(last>=0&&f.time-last>.1))return false;
            for(float v:{f.position.x,f.position.y,f.position.z,f.rotation.x,f.rotation.y,f.rotation.z})
                if(!std::isfinite(v)||std::abs(v)>10000)return false;
            last=f.time;
        }
        return true;
    }
    std::string encode() const {
        std::ostringstream s;s.imbue(std::locale::classic());s<<std::setprecision(17);
        s<<"NATT1 "<<track<<' '<<chassis<<' '<<duration;
        for(double split:splits)s<<' '<<split;
        s<<' '<<frames.size();
        for(const auto& f:frames)s<<' '<<std::setprecision(17)<<f.time<<' '<<std::setprecision(9)<<f.position.x<<' '<<f.position.y<<' '<<f.position.z
            <<' '<<f.rotation.x<<' '<<f.rotation.y<<' '<<f.rotation.z;
        return s.str();
    }
    static std::optional<TrialRun> decode(std::string_view bytes,int track,int chassis) {
        if(bytes.size()>maxBytes)return {};
        std::istringstream s{std::string(bytes)};s.imbue(std::locale::classic());
        TrialRun r;std::string magic;std::size_t count=0;
        if(!(s>>magic>>r.track>>r.chassis>>r.duration>>r.splits[0]>>r.splits[1]>>r.splits[2]>>count)||
            magic!="NATT1"||r.track!=track||r.chassis!=chassis||count<2||count>maxFrames)return {};
        r.frames.resize(count);
        for(auto& f:r.frames)if(!(s>>f.time>>f.position.x>>f.position.y>>f.position.z>>f.rotation.x>>f.rotation.y>>f.rotation.z))return {};
        s>>std::ws;if(!s.eof()||!r.valid())return {};
        return r;
    }
};
struct TrialRecorder {
    TrialRun run;
    bool clean=true;
    void begin(int track,int chassis,Vec3 position,Vec3 rotation) {
        run={};run.track=track;run.chassis=chassis;clean=true;
        run.frames.push_back({0,position,rotation});
    }
    void observe(const RaceProgress& before,const RaceProgress& after,Vec3 position,Vec3 rotation) {
        clean=clean&&after.routeValid&&after.recoveries==0&&after.elapsed<=TrialRun::maxSeconds;
        if(before.nextGate!=after.nextGate) {
            if(before.nextGate==4)run.splits[0]=after.elapsed;
            if(before.nextGate==8)run.splits[1]=after.elapsed;
            if(before.nextGate==0)run.splits[2]=after.elapsed;
        }
        const bool finish=after.completedLaps>0;
        if(clean&&!run.frames.empty()&&(after.elapsed-run.frames.back().time>=.05-1e-7||finish)&&after.elapsed>run.frames.back().time) {
            if(run.frames.size()>=TrialRun::maxFrames)clean=false;
            else run.frames.push_back({after.elapsed,position,rotation});
        }
        if(finish)run.duration=after.elapsed;
    }
};
}
