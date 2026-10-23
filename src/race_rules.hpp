#pragma once
#include "engine/math.hpp"
#include <array>
#include <limits>

namespace racing {
using engine::Vec3;
inline float magnitude(Vec3 v) { return std::sqrt(engine::dot(v,v)); }
enum class TrackKind { Kairox, DesertSolar };
// Shared source for asphalt, barriers, checkpoints and minimap. Metres; +Y up.
struct Circuit {
    static constexpr int samples=192, gates=12;
    static constexpr float halfWidth=10;
    std::array<Vec3,samples> points{};
    TrackKind kind;
    explicit Circuit(TrackKind selected=TrackKind::Kairox):kind(selected) {
        if(kind==TrackKind::DesertSolar) {
            // Closed Catmull-Rom route, resampled by distance so checkpoints,
            // AI look-ahead and road tessellation share uniform metric spacing.
            constexpr std::array<Vec3,10> anchors{{{0,0,-160},{95,0,-160},{150,0,-100},
                {130,0,0},{145,0,95},{80,0,155},{-45,0,145},{-120,0,80},
                {-110,0,-40},{-80,0,-145}}};
            std::array<Vec3,1001> curve{};std::array<float,1001> length{};
            for(int j=0;j<=1000;++j) {
                int i=(j/100)%10;float u=float(j%100)/100;
                auto a=anchors[(i+9)%10],b=anchors[i],c=anchors[(i+1)%10],d=anchors[(i+2)%10];
                curve[j]=(b*2+(c-a)*u+(a*2-b*5+c*4-d)*(u*u)+(a*-1+b*3-c*3+d)*(u*u*u))*.5f;
                if(j)length[j]=length[j-1]+magnitude(curve[j]-curve[j-1]);
            }
            int segment=1;
            for(int i=0;i<samples;++i) {
                float distance=length.back()*i/samples;
                while(length[segment]<distance)++segment;
                float u=(distance-length[segment-1])/(length[segment]-length[segment-1]);
                points[i]=curve[segment-1]+(curve[segment]-curve[segment-1])*u;
            }
            return;
        }
        for(int i=0;i<samples;++i) {
            float t=2*engine::pi*i/samples;
            points[i]={72*std::sin(t)+12*std::sin(2*t),0,-100*std::cos(t)};
        }
    }
    Vec3 point(int i) const { return points[(i%samples+samples)%samples]; }
    Vec3 tangent(int i) const { return engine::normalized(point(i+1)-point(i-1)); }
    Vec3 right(int i) const { auto t=tangent(i);return {t.z,0,-t.x}; }
    Vec3 gate(int i) const { return point(i*samples/gates); }
    Vec3 gateForward(int i) const { return tangent(i*samples/gates); }
    struct Nearest { float distance=std::numeric_limits<float>::max(); int segment=0; Vec3 position{}; };
    Nearest nearest(Vec3 p) const {
        p.y=0;Nearest best;
        for(int i=0;i<samples;++i) {
            auto a=point(i),d=point(i+1)-a;
            auto q=a+d*std::clamp(engine::dot(p-a,d)/engine::dot(d,d),0.f,1.f);
            float distance=magnitude(p-q);
            if(distance<best.distance)best={distance,i,q};
        }
        return best;
    }
};
struct RaceProgress {
    int nextGate=1, completedLaps=0, recoveries=0;
    double elapsed=0,lapTime=0,bestLap=0,lastLap=0,penalty=0;
    bool finished=false,routeValid=true;
    void reset() { *this={}; }
    void recover() { ++recoveries;penalty+=3;elapsed+=3;lapTime+=3;routeValid=true; }
    int lastGate() const { return (nextGate+Circuit::gates-1)%Circuit::gates; }
    // Only interpolate within the last validated sector. Proximity to a later
    // part of the circuit must never award a position after a shortcut.
    float distanceScore(const Circuit& track,Vec3 position) const {
        if(finished)return 3.f*Circuit::gates;
        int start=lastGate()*Circuit::samples/Circuit::gates;
        float best=std::numeric_limits<float>::max(),along=0,total=0,selected=0;
        for(int i=0;i<Circuit::samples/Circuit::gates;++i) {
            auto a=track.point(start+i),d=track.point(start+i+1)-a;position.y=0;
            float t=std::clamp(engine::dot(position-a,d)/engine::dot(d,d),0.f,1.f);
            float distance=magnitude(position-(a+d*t)),length=magnitude(d);
            if(distance<best){best=distance;selected=along+t*length;}
            along+=length;total+=length;
        }
        return completedLaps*Circuit::gates+lastGate()+selected/total;
    }
    void step(const Circuit& track,Vec3 previous,Vec3 position,double dt) {
        if(finished)return;
        elapsed+=dt;lapTime+=dt;
        if(magnitude(position-previous)>12 || track.nearest(position).distance>Circuit::halfWidth+2)
            routeValid=false;
        if(!routeValid || position.y>3 || position.y<-.5f)return;
        auto center=track.gate(nextGate),forward=track.gateForward(nextGate);
        float before=engine::dot(previous-center,forward),after=engine::dot(position-center,forward);
        if(before>=0 || after<0)return;
        auto crossing=previous+(position-previous)*(-before/(after-before));crossing.y=0;
        if(magnitude(crossing-center)>Circuit::halfWidth+1)return;
        if(nextGate==0) {
            ++completedLaps;lastLap=lapTime;
            if(bestLap==0 || lapTime<bestLap)bestLap=lapTime;
            lapTime=0;finished=completedLaps==3;
        }
        nextGate=(nextGate+1)%Circuit::gates;
    }
};
}
