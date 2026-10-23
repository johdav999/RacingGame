#pragma once
#include "race_rules.hpp"
#include <span>

namespace racing::ai {
enum class Difficulty { Easy, Normal, Hard };
struct Tuning {float topSpeed,lateralAcceleration,braking,headway,laneRate;};
inline Tuning tuning(Difficulty d) {
    switch(d){
    case Difficulty::Easy:return {17.5f,5.2f,3.8f,1.25f,1.8f};
    case Difficulty::Hard:return {25.f,8.2f,5.5f,.8f,2.8f};
    default:return {21.f,6.6f,4.6f,1.f,2.3f};
    }
}
struct Car {Vec3 position{},forward{0,0,1},velocity{};bool active=true;};
struct State {
    bool initialized=false,following=false;
    float lane=0,desiredLane=0,hold=0,targetSpeed=0,brake=0,steer=0;
    int passing=-1;
};
struct Command {float throttle=0,steer=0,brake=0;};
// Precompute metric curvature and braking envelopes once per track/settings.
// Runtime planning only reads six poses and emits ordinary driving commands.
class Planner {
public:
    explicit Planner(const Circuit& track):track_(track){
        for(int i=0;i<Circuit::samples;++i){
            length_[i]=magnitude(track.point(i+1)-track.point(i));
            distance_[i+1]=distance_[i]+length_[i];
            auto a=track.point(i)-track.point(i-2),b=track.point(i+2)-track.point(i);
            curvature_[i]=std::acos(std::clamp(engine::dot(engine::normalized(a),engine::normalized(b)),-1.f,1.f)) /
                std::max(.1f,(magnitude(a)+magnitude(b))*.5f);
        }
        for(int d=0;d<3;++d){
            auto t=tuning(static_cast<Difficulty>(d));auto& speeds=speeds_[d];
            for(int i=0;i<Circuit::samples;++i)speeds[i]=std::min(t.topSpeed,std::sqrt(t.lateralAcceleration/std::max(.001f,curvature_[i])));
            // Two backwards laps propagate the seam's braking constraint.
            for(int j=2*Circuit::samples-1;j>=0;--j){int i=j%Circuit::samples;
                speeds[i]=std::min(speeds[i],std::sqrt(speeds[(i+1)%Circuit::samples]*speeds[(i+1)%Circuit::samples]+2*t.braking*length_[i]));}
        }
    }
    float speedLimit(int segment,Difficulty d) const {return speeds_[static_cast<int>(d)][(segment+Circuit::samples)%Circuit::samples];}
    float curvature(int segment) const {return curvature_[(segment+Circuit::samples)%Circuit::samples];}
    Command update(int id,std::span<const Car> cars,State& state,Difficulty difficulty,float dt) const {
        const auto& car=cars[id];auto nearest=track_.nearest(car.position);
        const auto t=tuning(difficulty);const auto direction=track_.tangent(nearest.segment),right=track_.right(nearest.segment);
        const float speed=magnitude(car.velocity),offset=engine::dot(car.position-nearest.position,right);
        if(!state.initialized){state={};state.initialized=true;state.lane=state.desiredLane=std::clamp(offset,-5.f,5.f);}
        dt=std::clamp(dt,0.f,.1f);state.hold=std::max(0.f,state.hold-dt);state.following=false;
        float along=distance_[nearest.segment]+magnitude(nearest.position-track_.point(nearest.segment));
        struct Traffic {float gap,lane,speed,lateral;int id;};std::array<Traffic,6> traffic{};int count=0;
        for(int other=0;other<int(cars.size());++other)if(other!=id&&cars[other].active){
            auto n=track_.nearest(cars[other].position);
            float s=distance_[n.segment]+magnitude(n.position-track_.point(n.segment));
            float gap=std::remainder(s-along,distance_.back());
            if(std::abs(gap)>60)continue;
            traffic[count++]={gap,engine::dot(cars[other].position-n.position,track_.right(n.segment)),
                engine::dot(cars[other].velocity,track_.tangent(n.segment)),engine::dot(cars[other].velocity,track_.right(n.segment)),other};
        }
        const Traffic* lead=nullptr;
        for(int i=0;i<count;++i){const auto& o=traffic[i];
            if(o.gap>0&&std::abs(o.lane-offset)<3.4f&&(!lead||o.gap<lead->gap))lead=&o;}
        // A lane change sweeps the whole corridor, including faster cars behind.
        auto clear=[&](float lane){
            for(int i=0;i<count;++i){const auto& o=traffic[i];
                for(float seconds:{0.f,.6f,1.2f}){
                    float gap=o.gap+(o.speed-speed)*seconds,lateral=o.lane+o.lateral*seconds;
                    float ownLane=offset+std::clamp(lane-offset,-t.laneRate*seconds,t.laneRate*seconds);
                    if(std::abs(gap)<5.5f&&std::abs(lateral-ownLane)<3.3f)return false;
                }
                if(o.gap>0&&o.gap<32&&std::abs(o.lane-lane)<3.3f)return false;
            }return true;
        };
        bool tight=false;float scan=0;
        for(int j=0;j<16&&scan<30;++j){int i=(nearest.segment+j)%Circuit::samples;tight|=curvature_[i]>.038f;scan+=length_[i];}
        if(state.passing>=0){
            bool alongside=false;
            for(int i=0;i<count;++i)if(traffic[i].id==state.passing&&traffic[i].gap>-11)alongside=true;
            if(!alongside&&state.hold==0)state.passing=-1;
        }
        if(lead&&lead->gap<35&&lead->speed+1<speedLimit(nearest.segment,difficulty)&&!tight&&state.hold==0&&state.passing<0){
            float first=lead->lane>=0?-4.2f:4.2f,second=-first;
            for(float lane:{first,second})if(clear(lane)){state.desiredLane=lane;state.passing=lead->id;state.hold=2;break;}
        }else if(!lead&&state.passing<0&&state.hold==0){
            float home=id==0?0:(id%2?-3.2f:3.2f);
            if(clear(home))state.desiredLane=home;
        }
        // Abort an unsafe merge without swerving to the opposite side.
        if(std::abs(state.desiredLane-offset)>.6f&&!clear(state.desiredLane)){
            state.desiredLane=std::clamp(offset,-5.f,5.f);state.passing=-1;state.hold=.5f;
        }
        state.lane+=std::clamp(state.desiredLane-state.lane,-t.laneRate*dt,t.laneRate*dt);
        float look=std::clamp(10.f+speed*.65f,12.f,27.f),travel=0;int target=nearest.segment;
        while(travel<look){travel+=length_[target];target=(target+1)%Circuit::samples;}
        auto destination=track_.point(target)+track_.right(target)*state.lane;
        auto delta=destination-car.position;
        float angle=std::remainder(std::atan2(delta.x,delta.z)-std::atan2(car.forward.x,car.forward.z),2*engine::pi);
        state.steer=std::clamp(-angle*2,-1.f,1.f);
        state.targetSpeed=speedLimit(nearest.segment,difficulty);
        // Inside lanes have less radius; keep the shared profile conservative.
        if(tight)state.targetSpeed*=.9f;
        if(std::abs(angle)>.65f)state.targetSpeed=std::min(state.targetSpeed,10.f);
        if(lead&&lead->gap<std::max(12.f,speed*t.headway+6)){
            state.targetSpeed=std::min(state.targetSpeed,std::max(0.f,lead->speed+(lead->gap-7)*.45f));state.following=true;
        }
        float error=state.targetSpeed-speed;
        state.brake=error<-.8f?std::clamp(-error/3,.55f,1.f):0;
        return {state.brake>0?0.f:std::clamp(error*.65f,0.f,1.f),state.steer,state.brake};
    }
private:
    const Circuit& track_;
    std::array<float,Circuit::samples> length_{},curvature_{};
    std::array<float,Circuit::samples+1> distance_{};
    std::array<std::array<float,Circuit::samples>,3> speeds_{};
};
}
