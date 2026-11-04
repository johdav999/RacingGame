#include "starter_scene.hpp"
#include "engine/application.hpp"
#include <iostream>
#include <stdexcept>
using namespace racing;
using namespace racing::ai;
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void planner() {
    for(auto kind:{TrackKind::Kairox,TrackKind::DesertSolar,TrackKind::SiroccoSolar,TrackKind::CanyonRelay}){
        Circuit track(kind);Planner planner(track);bool brakingBeforeCorner=false;
        for(int i=0;i<Circuit::samples;++i){
            float easy=planner.speedLimit(i,Difficulty::Easy),normal=planner.speedLimit(i,Difficulty::Normal),hard=planner.speedLimit(i,Difficulty::Hard);
            require(easy<normal&&normal<hard,"Difficulty must order the whole track speed profile");
            auto t=tuning(Difficulty::Normal);float local=std::min(t.topSpeed,std::sqrt(t.lateralAcceleration/std::max(.001f,planner.curvature(i))));
            brakingBeforeCorner|=normal<local-.2f;
            std::array<Car,1> cars{{{track.point(i),track.tangent(i),track.tangent(i)*(normal+5)}}};State state;
            auto command=planner.update(0,cars,state,Difficulty::Normal,1.f/60);
            require(command.brake>.5f&&command.throttle==0,"Overspeed commands physical braking, not acceleration");
        }
        require(brakingBeforeCorner,"Braking envelope must slow before the corner apex");
    }
    Circuit straight;
    // Long straight-sided oval fixture gives deterministic traffic spacing.
    for(int i=0;i<Circuit::samples;++i){float angle=2*engine::pi*i/Circuit::samples;straight.points[i]={300*std::sin(angle),0,-300*std::cos(angle)};}
    Planner p(straight);auto make=[&](float ahead,float lane,float speed){auto f=straight.tangent(0),r=straight.right(0);return Car{straight.point(0)+f*ahead+r*lane,f,f*speed};};
    std::array<Car,4> cars{make(0,0,18),make(22,0,9),make(-4,-4.2f,24),make(-4,4.2f,24)};
    State blocked;auto command=p.update(0,cars,blocked,Difficulty::Normal,1.f/60);
    require(blocked.passing==-1&&std::abs(blocked.desiredLane)<.01,"No merge into either occupied side or a faster approaching car");
    cars[1]=make(10,0,5);command=p.update(0,cars,blocked,Difficulty::Normal,1.f/60);
    require(blocked.following&&command.brake>0,"Boxed-in driver brakes behind slower traffic");
    cars[1]=make(22,0,9);cars[2].active=cars[3].active=false;
    State pass;p.update(0,cars,pass,Difficulty::Normal,1.f/60);
    require(pass.passing==1&&std::abs(pass.desiredLane)>4,"Clear side commits a real passing line");
    require(std::abs(pass.lane)<.05,"Lane change is rate-limited, not an instant swerve");
    float chosen=pass.desiredLane;
    for(int j=0;j<40;++j)p.update(0,cars,pass,Difficulty::Normal,1.f/60);
    require(pass.desiredLane==chosen,"Passing commitment does not oscillate between sides");
    cars[2]=make(0,chosen,18);p.update(0,cars,pass,Difficulty::Normal,1.f/60);
    require(pass.passing==-1&&std::abs(pass.desiredLane)<.01,"New side traffic cancels unsafe merge without crossing it");
    cars[0]=make(0,0,0);cars[1]=make(7,0,0);cars[2].active=false;
    State stopped;p.update(0,cars,stopped,Difficulty::Normal,1.f/60);
    require(stopped.passing==1,"Stationary queue can safely pull around a stopped leader");
}
double race(TrackKind track,Difficulty difficulty){
    engine::Application app;auto owned=std::make_unique<StarterScene>(track);auto* scene=owned.get();
    app.start(app.loadScene(std::move(owned)));app.advance(0);
    require(scene->setDifficulty(int(difficulty)),"Garage accepts difficulty");
    require(!scene->setDifficulty(3),"Invalid difficulty rejected");scene->start();
    require(!scene->setDifficulty((int(difficulty)+1)%3),"Difficulty locked once event starts");
    double clock=0;int ticks=0;bool done=false;
    for(;ticks<27000&&!done;++ticks){clock+=1./60;app.advance(clock);scene->present(1./60,16.f/9);
        done=true;for(const auto& r:scene->standings())if(r.id&&!r.progress.finished)done=false;
    }
    double mean=0;int recoveries=0;
    for(const auto& r:scene->standings())if(r.id){
        std::cout<<"track "<<int(track)<<" difficulty "<<int(difficulty)<<" id "<<r.id<<" laps "<<r.progress.completedLaps<<" time "<<r.progress.elapsed<<" recoveries "<<r.progress.recoveries<<std::endl;
        require(r.progress.finished&&r.progress.completedLaps==3,"All AI must physically finish three ordered laps");
        mean+=r.progress.elapsed/5;recoveries+=r.progress.recoveries;
    }
    require(recoveries<=5,"Field must finish without repeated recovery loops");
    scene->selectChassis(Chassis::Vektor);require(scene->difficulty()==difficulty,"Garage retains difficulty");
    scene->setTimeTrial(true);require(!scene->setDifficulty(0),"Trial ignores rival difficulty changes");
    return mean;
}
int main(int argc,char** argv){try{planner();if(argc>2){race(static_cast<TrackKind>(std::stoi(argv[1])),static_cast<Difficulty>(std::stoi(argv[2])));}else if(argc>1){
    for(auto track:{TrackKind::Kairox,TrackKind::DesertSolar}){
        auto easy=race(track,Difficulty::Easy),normal=race(track,Difficulty::Normal),hard=race(track,Difficulty::Hard);
        std::cout<<"mean finishes "<<easy<<" "<<normal<<" "<<hard<<std::endl;
        require(easy>normal&&normal>hard,"Physical field race pace follows selected difficulty");
    }
}std::cout<<"Racing AI checks passed\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
