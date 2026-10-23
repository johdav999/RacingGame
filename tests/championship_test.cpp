#include "starter_scene.hpp"
#include "engine/application.hpp"
#include <iostream>
#include <stdexcept>
using namespace racing;
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
const Championship::Result win{1,2,3,4,5,6},last{6,1,2,3,4,5};
void rules(){
    Championship c;check(c.unlocked()==0&&!c.canRace(),"New profile only unlocks Street");
    check(!c.enter(-1)&&!c.enter(1)&&!c.enter(5),"Invalid/locked leagues reject entry");
    check(c.enter(0),"Street entry");
    auto bytes=c.encode();check(Championship::decode(bytes)->encode()==bytes,"Initial season roundtrip");
    check(!c.record(c.revision,{1,1,2,3,4,5})&&!c.record(c.revision,{1,2,3,4,5,7}),"Invalid classification rejects");
    check(c.encode()==bytes,"Invalid result preserves state");
    for(int l=0;l<5;l++){
        if(l)check(c.enter(l),"Promotion permits next league");
        check(c.rankCap()==std::min(l,3),"Per-league equipment cap");
        for(int r=0;r<(l==4?6:4);r++){
            check(c.track()==(r+l%2)%2,"Scheduled track sequence");int token=c.revision;
            check(!c.record(token-1,win),"Stale result rejected");
            check(c.record(token,win),"Round accepted");
            auto accepted=c.encode();check(!c.record(token,win),"Duplicate result rejected");
            check(c.encode()==accepted,"Duplicate preserves points");
            check(Championship::decode(accepted)->encode()==accepted,"Every intermediate state reloads");
        }
        if(l<4){
            check(c.status()==Championship::Status::Final&&c.completed==4,"Top three qualify");
            int points=c.standings()[0].points;
            check(c.record(c.revision,last)&&c.status()==Championship::Status::FinalLost,"Final loss retains qualification");
            check(c.unlocked()==l,"Final loss cannot promote");
            check(c.standings()[0].points==points,"Final does not change season points");
            auto reload=Championship::decode(c.encode());check(reload&&reload->finalAttempts==1,"Lost final save");c=*reload;
            check(c.record(c.revision,win),"Retry promotion final");
        }
        check(c.status()==Championship::Status::Won,"League won");
        check(c.unlocked()==std::min(l+1,4),"Promotion advances one level");
        check((c.trophies&(1<<l))!=0,"Title retained");
        check(!c.record(c.revision,win),"Completed league cannot award again");
    }
    check(c.trophies==31,"Full ladder to Master title");
    check(c.enter(0)&&c.trophies==31&&c.unlocked()==4,"Replay preserves every title");
    for(int r=0;r<4;r++)check(c.record(c.revision,last),"Losing season round");
    check(c.status()==Championship::Status::SeasonLost&&!c.canRace(),"Outside top three requires new season");
    check(c.enter(4),"Replay Master");for(int r=0;r<6;r++)check(c.record(c.revision,last),"Master losing round");
    check(c.status()==Championship::Status::MasterLost&&c.trophies==31,"Loss retains earned title");
    Championship tie;tie.enter(0);
    tie.record(tie.revision,{1,2,3,4,5,6});tie.record(tie.revision,{2,1,3,4,5,6});
    check(tie.standings()[0].id==1,"Latest result resolves equal points and finish counts");
    Championship dnf;dnf.enter(0);check(dnf.record(dnf.revision,{0,1,2,3,4,5}),"DNF classification");
    check(dnf.playerPosition()==6&&dnf.standings()[5].points==0,"DNF earns zero");
    check(!Championship::validResult({0,2,3,4,5,6}),"Skipped finishing position rejects");
    for(int p=1;p<=6;p++)check(Championship::points(p)==Championship::pointsTable[p-1],"All points positions");
    check(Championship::points(0)==0&&Championship::points(7)==0,"Invalid positions earn nothing");
}
void saves(){
    for(auto old:{"NA1 4321 9 1 3 2 1 0 1 2","NA2 4321 9 1 3 2 1 0 1 2 3"}){
        auto p=Progression::decode(old);check(bool(p),"Legacy save migration");
        check(p->credits==4321&&p->completedRaces==9&&p->vehicles[0].ranks[0]==3,"Money/upgrades migrate");
        check(p->championship.league==-1,"Legacy starts without league progress");
        auto migrated=p->encode();check(migrated.starts_with("NA5 "),"Current save version");
        check(Progression::decode(migrated)->encode()==migrated,"Full migrated save roundtrip");
    }
    Progression p;p.credits=123;p.championship.enter(0);
    p.championship.record(p.championship.revision,win);p.award(1);
    auto snapshot=p.encode();check(Progression::decode(snapshot)->encode()==snapshot,"Wallet and points save together");
    check(!Progression::decode(snapshot+" garbage"),"Trailing data rejects whole save");
    check(!Progression::decode("NA3 1 2 | LC9 0"),"Unknown championship version rejects");
    auto bad=p.championship;bad.history[0][1]=1;check(!Championship::decode(bad.encode()),"Duplicate positions reject load");
    bad=p.championship;bad.trophies=2;check(!Championship::decode(bad.encode()),"Skipped league unlock rejects load");
    bad=p.championship;bad.history[5]=win;check(!Championship::decode(bad.encode()),"Unplayed history rejects load");
    bad=p.championship;bad.finalAttempts=1;check(!Championship::decode(bad.encode()),"Premature final rejects load");
    bad=p.championship;bad.revision=0;check(!Championship::decode(bad.encode()),"Inconsistent revision rejects load");
    check(!Progression::decode(std::string(Progression::maxSaveBytes+1,'1')),"Oversized saves reject");
}
void records(){
    Championship c;check(c.enter(0),"Records season starts");
    for(int i=0;i<4;i++)check(c.record(c.revision,win),"Records points round");
    check(c.latest[0]&&c.best[0]&&c.latest[0]->completed==4,"Completed points standings archive automatically");
    check(c.record(c.revision,win),"Records promotion final");
    auto champion=c.best[0]->encode();
    check(c.best[0]->status()==Championship::Status::Won,"Won final stored in permanent best");
    check(c.enter(1)&&c.latest[0]->status()==Championship::Status::Won,"Next league preserves Street result");
    check(c.record(c.revision,last),"Partial District round");
    check(c.enter(0)&&c.latest[1]&&c.latest[1]->completed==1,"Switching saves partial run");
    for(int i=0;i<4;i++)c.record(c.revision,last);
    check(c.latest[0]->status()==Championship::Status::SeasonLost,"Latest lost season stays reviewable");
    check(c.best[0]->encode()==champion,"Worse replay cannot replace best championship");
    const auto saved=c.encode();auto loaded=Championship::decode(saved);
    check(loaded&&loaded->encode()==saved,"All league records survive codec roundtrip");
    check(loaded->latest[1]->history[0]==last,"Per-racer partial round results retained");
    auto rejected=c.encode();check(!c.enter(4)&&c.encode()==rejected,"Locked entry cannot alter stored results");
    auto bad=c;bad.latest[1]->league=0;check(!Championship::decode(bad.encode()),"Wrong-league archive rejects atomically");
    bad=c;bad.best[0].reset();check(!Championship::decode(bad.encode()),"Unpaired best/latest archive rejects");
    bad=c;bad.latest[1]->revision=c.revision+1;check(!Championship::decode(bad.encode()),"Future archive revision rejects");
    auto old=static_cast<const LeagueSeason&>(c).encode();
    auto migrated=Championship::decode(old);check(migrated&&migrated->latest[0],"LC1 completed season seeds player records");
    const auto wallet="4321 9 1 3 2 1 0 1 2 3";
    auto na3=Progression::decode(std::string("NA3 ")+wallet+" | "+old);
    check(na3&&na3->championship.latest[0]&&na3->credits==4321,"NA3 career and results migrate");
    auto na4=Progression::decode(std::string("NA4 ")+wallet+" | "+old+" / 123 456 16777216 789");
    check(na4&&na4->paint==std::array<int,4>{123,456,16777216,789},"NA4 paint survives archive migration");
    na4->championship=c;auto complete=na4->encode();auto restored=Progression::decode(complete);
    check(restored&&restored->encode()==complete&&restored->paint==na4->paint,"NA5 stores wallet, paint and league records together");
    check(complete.size()<Progression::maxSaveBytes,"Bounded career envelope accommodates archive");
    check(!Championship::decode(saved+" ~ -"),"Extra archive slots reject");
    std::cout<<"Persistent league latest/best records, partial runs, invalid archives and NA3/NA4 migration passed\n";
}
void scene(){
    engine::Application app;auto owned=std::make_unique<StarterScene>();auto* g=owned.get();
    app.start(app.loadScene(std::move(owned)));app.advance(0);
    check(g->restoreProgression("NA2 20000 0 0 3 3 3 3 3 3 0"),"Upgraded profile fixture");
    check(!g->enterLeague(4)&&g->enterLeague(0),"Scene enforces unlocks");
    check(g->startLeagueRace()&&g->leagueRace(),"Scene starts scheduled league round");
    check(g->difficulty()==ai::Difficulty::Easy,"League controls difficulty");
    check(!g->setDifficulty(2)&&!g->enterLeague(0)&&!g->purchase(0),"Running league blocks configuration changes");
    check(g->audioFrame().racers.size()>=6,"Normal six-racer event");
    g->start();check(g->leagueRace()&&g->phase==Phase::Countdown,"Restart preserves scheduled event");
    check(g->progression().championship.completed==0,"Restart does not score");
    g->selectChassis(Chassis::Skimmer);check(!g->leagueRace(),"Abandon returns to garage");
    check(g->progression().championship.completed==0&&g->progression().credits==20000,"Abandon retains round and money");
    check(g->startLeagueRace(),"Resume uncompleted round");
    g->selectChassis(Chassis::Vektor);g->start();check(!g->leagueRace(),"Quick race remains independent");
    g->selectChassis(Chassis::Vektor);
    // Controlled completed-round fixture exercises scheduled cross-scene loading.
    Progression p=g->progression();p.championship.record(p.championship.revision,win);p.award(1);
    check(g->restoreProgression(p.encode()),"Saved completed-round fixture");
    check(!g->startLeagueRace(),"Wrong scene cannot start next scheduled round");
    check(!g->restoreProgression("NA3 corrupt"),"Invalid restore rejects");
    check(g->progression().encode()==p.encode(),"Failed restore preserves complete career");
    engine::Application next;auto desert=std::make_unique<StarterScene>(TrackKind::DesertSolar);auto* d=desert.get();
    next.start(next.loadScene(std::move(desert)));next.advance(0);
    check(d->restoreProgression(p.encode())&&d->startLeagueRace(),"New track scene restores season and starts next round");
    check(d->progression().championship.completed==1,"Round history survives scene replacement");
}
void physical(){
    for(int scenario=0;scenario<5;++scenario){
        const auto track=(scenario==1||scenario==3)?TrackKind::DesertSolar:TrackKind::Kairox;
        const int league=(scenario==2||scenario==3)?4:0;const bool final=scenario==4;
        engine::Application app;auto owned=std::make_unique<StarterScene>(track);auto* g=owned.get();
        app.start(app.loadScene(std::move(owned)));app.advance(0);
        Progression p;p.chassis=1;p.selected=1;p.credits=1000;p.vehicles[1].ranks={3,3,3};p.championship.trophies=league==4?15:0;p.championship.enter(league);
        int preceding=final?4:track==TrackKind::DesertSolar?1:0;
        for(int r=0;r<preceding;++r){p.championship.record(p.championship.revision,win);p.award(1);}
        check(g->restoreProgression(p.encode())&&g->startLeagueRace(),"Physical scheduled race starts");
        double clock=0;bool pending=false;
        for(int tick=0;tick<30000&&!g->classificationFinal();++tick){
            auto pos=g->player().transform.position;auto nearest=g->track.nearest(pos);auto d=g->track.point(nearest.segment+6)-pos;
            float angle=std::remainder(std::atan2(d.x,d.z)-g->heading(),2*engine::pi);
            g->input={};g->input.steer=std::clamp(-angle*2,-1.f,1.f);
            g->input.throttle=g->speed()<65?1.f:.1f;g->input.brake=g->speed()>76?.5f:0;g->input.fire=1;
            if(g->awaitingResults()){
                pending=true;check(g->progression().encode()==p.encode(),"No points or payout before final classification");
                check(!g->enterLeague(0),"Pending result cannot replace season");
            }
            clock+=1./60;app.advance(clock);g->present(1./60,16.f/9);
        }
        check(g->classificationFinal()&&g->phase==Phase::Finished,"Physical field finishes league event");
        const auto& after=g->progression();check(after.championship.completed==p.championship.completed+(final?0:1),"Exactly one scheduled event committed");
        int pos=0;for(const auto& row:g->standings()){
            if(row.id==0)pos=row.position;
            if(!final)check(after.championship.history[p.championship.completed][row.id]==row.position,"Recorded positions match physical finishing classification");}
        if(final){check(after.championship.finalAttempts==1&&after.championship.finalPosition==pos,"Physical final records one attempt");
            check(after.championship.unlocked()==(pos==1?1:0),"Physical final promotes only its winner");}
        check(after.credits==p.credits+Progression::payout(pos)&&after.completedRaces==p.completedRaces+1,"Points and credits commit together");
        check(g->standings()[0].combat.capacity==(league==4?175:100),"League equipment applies to the physical field");
        auto saved=after.encode();for(int i=0;i<120;i++){clock+=1./60;app.advance(clock);}
        check(g->progression().encode()==saved,"Finished ticks cannot repeat points or payout");
        g->selectChassis(Chassis::Skimmer);check(g->progression().vehicles[1].ranks[0]==3,"Equipment cap preserves purchases");
        std::cout<<"Physical championship league "<<league<<" final="<<final<<" track "<<int(track)<<" pending="<<pending<<" player position="<<pos<<'\n';
    }
}
void recordFixtures(){
    Progression p;p.credits=9000;p.chassis=1;p.selected=1;p.paint={123,456,16777216,789};
    p.championship.enter(0);for(int i=0;i<5;i++){p.championship.record(p.championship.revision,win);p.award(1);}
    p.championship.enter(1);p.championship.record(p.championship.revision,last);p.award(6);
    p.championship.enter(0);for(int i=0;i<4;i++){p.championship.record(p.championship.revision,last);p.award(6);}
    p.championship.enter(1);p.championship.record(p.championship.revision,win);p.award(1);
    std::cout<<p.encode()<<'\n';
}
void fixtures(){
    std::cout<<"{";bool first=true;
    auto emit=[&](const char* name,const Progression& p){if(!first)std::cout<<',';first=false;std::cout<<'"'<<name<<"\":\""<<p.encode()<<'"';};
    Progression p;p.credits=12000;p.vehicles[0].ranks={3,3,3};p.vehicles[1].ranks={3,3,3};p.championship.enter(0);
    p.championship.record(p.championship.revision,win);p.award(1);emit("nextRound",p);
    for(int i=1;i<4;i++){p.championship.record(p.championship.revision,win);p.award(1);}emit("qualified",p);
    p.championship.record(p.championship.revision,last);p.award(6);emit("finalLost",p);
    p.championship.record(p.championship.revision,win);p.award(1);emit("promoted",p);
    for(int l=1;l<4;l++){p.championship.enter(l);for(int i=0;i<5;i++){p.championship.record(p.championship.revision,win);p.award(1);}}
    p.championship.enter(4);for(int i=0;i<5;i++){p.championship.record(p.championship.revision,win);p.award(1);}emit("masterLastRound",p);
    p.championship.record(p.championship.revision,last);p.award(6);emit("masterWon",p);
    p.championship.enter(0);for(int i=0;i<4;i++){p.championship.record(p.championship.revision,last);p.award(6);}emit("failedSeason",p);
    std::cout<<"}\n";
}
int main(int argc,char** argv){try{if(argc>1&&std::string_view(argv[1])=="records"){recordFixtures();return 0;}if(argc>1&&std::string_view(argv[1])=="records-tests"){rules();saves();records();scene();return 0;}if(argc>1){fixtures();return 0;}rules();saves();records();scene();physical();std::cout<<"Championship: ladder, scoring, ties, retries, migration, guards and scene travel passed\n";}
    catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
