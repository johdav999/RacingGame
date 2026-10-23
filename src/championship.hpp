#pragma once
#include <algorithm>
#include <array>
#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>

namespace racing {
// Portable game policy. Racer IDs remain stable across disposable track scenes.
struct LeagueSeason {
    static constexpr int racers=6, leagues=5, maxRevision=1000000;
    static constexpr std::array<int,6> pointsTable{15,12,10,8,6,4};
    static constexpr std::array<const char*,5> names{
        "Street League","District League","Metro League","Elite League","Master Championship"};
    using Result=std::array<int,racers>; // position per racer; zero is DNF
    int league=-1,trophies=0,revision=0,completed=0,finalPosition=0,finalAttempts=0;
    std::array<Result,6> history{};
    enum class Status { NotEntered, Season, Final, SeasonLost, FinalLost, Won, MasterLost };
    struct Row {int id=0,position=0,points=0,wins=0,gap=0;};
    int unlocked() const {
        int value=0;while(value<4&&(trophies&(1<<value)))++value;return value;
    }
    int rounds() const {return league==4?6:4;}
    static int points(int position) {return position>=1&&position<=6?pointsTable[position-1]:0;}
    static bool validResult(const Result& result) {
        int used=0,finished=0;
        for(int p:result){if(p<0||p>6||(p&&(used&(1<<p))))return false;if(p){used|=1<<p;++finished;}}
        return used==((1<<(finished+1))-2); // finishers occupy contiguous positions
    }
    std::array<Row,6> standings() const {
        std::array<Row,6> rows{};for(int id=0;id<6;++id){auto& row=rows[id];row.id=id;
            for(int r=0;r<completed;++r){row.points+=points(history[r][id]);row.wins+=history[r][id]==1;}}
        std::sort(rows.begin(),rows.end(),[&](const Row& a,const Row& b){
            if(a.points!=b.points)return a.points>b.points;
            for(int p=1;p<=6;++p){int ac=0,bc=0;for(int r=0;r<completed;++r){ac+=history[r][a.id]==p;bc+=history[r][b.id]==p;}
                if(ac!=bc)return ac>bc;}
            for(int r=completed-1;r>=0;--r){int ap=history[r][a.id]?history[r][a.id]:7,bp=history[r][b.id]?history[r][b.id]:7;
                if(ap!=bp)return ap<bp;}
            return a.id<b.id;
        });
        for(int i=0;i<6;++i){rows[i].position=i+1;rows[i].gap=rows[0].points-rows[i].points;}return rows;
    }
    int playerPosition() const {for(auto r:standings())if(r.id==0)return r.position;return 6;}
    Status status() const {
        if(league<0)return Status::NotEntered;
        if(completed<rounds())return Status::Season;
        if(league==4)return playerPosition()==1?Status::Won:Status::MasterLost;
        if(playerPosition()>3)return Status::SeasonLost;
        if(finalPosition==1)return Status::Won;
        return finalAttempts?Status::FinalLost:Status::Final;
    }
    bool canRace() const {auto s=status();return s==Status::Season||s==Status::Final||s==Status::FinalLost;}
    bool isFinal() const {return league>=0&&league<4&&completed==4&&playerPosition()<=3;}
    // Alternating existing circuits; later leagues start in the desert.
    int track() const {return league<0?0:((completed+(league%2))%2);}
    int rankCap() const {return std::clamp(league,0,3);}
    int difficulty() const {return league==0?0:league<3?1:2;}
    bool enter(int selected) {
        if(selected<0||selected>unlocked()||revision>=maxRevision)return false;
        league=selected;completed=finalPosition=finalAttempts=0;history={};++revision;return true;
    }
    // A captured revision makes each accepted race result idempotent, including
    // final retries and races returning from an older scene.
    bool record(int expectedRevision,const Result& result) {
        if(expectedRevision!=revision||revision>=maxRevision||!canRace()||!validResult(result))return false;
        if(isFinal()){finalPosition=result[0];++finalAttempts;}
        else history[completed++]=result;
        ++revision;
        if(status()==Status::Won)trophies|=1<<league;
        return true;
    }
    std::string encode() const {
        std::string s="LC1";
        for(int v:{league+1,trophies,revision,completed,finalPosition,finalAttempts})s+=" "+std::to_string(v);
        for(auto row:history)for(int v:row)s+=" "+std::to_string(v);return s;
    }
    static std::optional<LeagueSeason> decode(std::string_view text) {
        if(!text.starts_with("LC1 ")||text.size()>1024)return {};
        text.remove_prefix(4);std::array<int,42> values{};
        for(int i=0;i<42;++i){auto end=text.find(' ');auto token=text.substr(0,end);
            if(token.empty()||token.front()<'0'||token.front()>'9')return {};
            auto [p,ec]=std::from_chars(token.data(),token.data()+token.size(),values[i]);
            if(ec!=std::errc{}||p!=token.data()+token.size())return {};
            if(i==41){if(end!=std::string_view::npos)return {};}
            else {if(end==std::string_view::npos)return {};text.remove_prefix(end+1);}}
        LeagueSeason c;c.league=values[0]-1;c.trophies=values[1];c.revision=values[2];c.completed=values[3];
        c.finalPosition=values[4];c.finalAttempts=values[5];
        if(values[0]>5||c.trophies>31||(c.trophies&(c.trophies+1))||c.revision>maxRevision||c.completed>c.rounds()||
           c.finalPosition>6||c.finalAttempts>maxRevision||c.league>c.unlocked())return {};
        for(int r=0;r<6;++r){for(int id=0;id<6;++id)c.history[r][id]=values[6+r*6+id];
            if(r<c.completed){if(!validResult(c.history[r]))return {};}
            else if(c.history[r]!=Result{})return {};}
        if(c.league<0){if(c.trophies||c.revision||c.completed||c.finalPosition||c.finalAttempts)return {};}
        else {
            if(c.revision<1+c.completed+c.finalAttempts)return {};
            if(c.finalPosition&&!c.finalAttempts)return {};
            if((c.finalPosition||c.finalAttempts)&&!c.isFinal())return {};
            if(c.status()==Status::Won&&!(c.trophies&(1<<c.league)))return {};
        }
        return c;
    }
    std::string json(bool racing=false,bool raceFinal=false) const {
        std::string s="{\"league\":"+std::to_string(league)+",\"unlocked\":"+std::to_string(unlocked())+
            ",\"trophies\":"+std::to_string(trophies)+",\"revision\":"+std::to_string(revision)+
            ",\"completed\":"+std::to_string(completed)+",\"rounds\":"+std::to_string(rounds())+
            ",\"finalPosition\":"+std::to_string(finalPosition)+",\"finalAttempts\":"+std::to_string(finalAttempts)+
            ",\"status\":"+std::to_string(static_cast<int>(status()))+",\"track\":"+std::to_string(track())+
            ",\"rankCap\":"+std::to_string(rankCap())+",\"difficulty\":"+std::to_string(difficulty())+
            ",\"final\":"+(isFinal()?"true":"false")+",\"canRace\":"+(canRace()?"true":"false")+
            ",\"racing\":"+(racing?"true":"false")+",\"raceFinal\":"+(raceFinal?"true":"false")+",\"standings\":[";
        bool first=true;for(auto r:standings()){
            if(!first)s+=",";first=false;
            s+="{\"id\":"+std::to_string(r.id)+",\"position\":"+std::to_string(r.position)+",\"points\":"+std::to_string(r.points)+
                ",\"wins\":"+std::to_string(r.wins)+",\"gap\":"+std::to_string(r.gap)+",\"results\":[";
            for(int round=0;round<completed;++round){if(round)s+=",";s+=std::to_string(history[round][r.id]);}s+="]}";}
        return s+"]}";
    }
};

// One active season plus persistent per-league latest/best player records.
// Stored snapshots contain value data only; no scene, host or runtime handles.
struct Championship : LeagueSeason {
    static constexpr std::size_t maxBytes=12288;
    std::array<std::optional<LeagueSeason>,leagues> latest{},best{};
    static auto merit(const LeagueSeason& s) {
        int points=0;for(auto row:s.standings())if(row.id==0)points=row.points;
        return std::tuple{int(s.status()==Status::Won),int(s.completed==s.rounds()),s.completed,
            -s.playerPosition(),points,-(s.finalPosition?s.finalPosition:7)};
    }
    void remember() {
        if(league<0||!completed)return;
        const auto snapshot=static_cast<const LeagueSeason&>(*this);
        latest[league]=snapshot;
        if(!best[league]||merit(snapshot)>merit(*best[league]))best[league]=snapshot;
    }
    bool enter(int selected) {
        if(selected<0||selected>unlocked()||revision>=maxRevision)return false;
        remember();return LeagueSeason::enter(selected);
    }
    bool record(int expectedRevision,const Result& result) {
        if(!LeagueSeason::record(expectedRevision,result))return false;
        if(completed==rounds())remember();
        return true;
    }
    std::string encode() const {
        std::string text="LC2 "+LeagueSeason::encode();
        for(int i=0;i<leagues;++i)for(const auto* slot:{&latest[i],&best[i]})
            text+=" ~ "+(*slot?(*slot)->encode():"-");
        return text;
    }
    static std::optional<Championship> decode(std::string_view text) {
        Championship c;
        if(text.starts_with("LC1 ")) {
            auto season=LeagueSeason::decode(text);if(!season)return {};
            static_cast<LeagueSeason&>(c)=*season;
            if(c.completed==c.rounds())c.remember();
            return c;
        }
        if(!text.starts_with("LC2 ")||text.size()>maxBytes)return {};
        text.remove_prefix(4);
        for(int field=0;field<=leagues*2;++field) {
            auto end=text.find(" ~ ");auto value=text.substr(0,end);
            if(field==0){auto season=LeagueSeason::decode(value);if(!season)return {};static_cast<LeagueSeason&>(c)=*season;}
            else if(value!="-") {
                auto season=LeagueSeason::decode(value);int index=(field-1)/2;
                if(!season||season->league!=index||!season->completed||season->revision>c.revision||
                   (season->trophies&~c.trophies))return {};
                (field%2?c.latest[index]:c.best[index])=*season;
            }
            if(field==leagues*2){if(end!=std::string_view::npos)return {};}
            else {if(end==std::string_view::npos)return {};text.remove_prefix(end+3);}
        }
        for(int i=0;i<leagues;++i){
            if(bool(c.latest[i])!=bool(c.best[i]))return {};
            if(c.latest[i]&&(c.best[i]->revision>c.latest[i]->revision||merit(*c.latest[i])>merit(*c.best[i])))return {};
        }
        return c;
    }
    std::string json(bool racing=false,bool raceFinal=false) const {
        auto text=LeagueSeason::json(racing,raceFinal);text.pop_back();text+=",\"records\":[";
        for(int i=0;i<leagues;++i){if(i)text+=",";
            text+="{\"league\":"+std::to_string(i)+",\"latest\":"+(latest[i]?latest[i]->json():"null")+
                ",\"best\":"+(best[i]?best[i]->json():"null")+"}";
        }
        return text+"]}";
    }
};
}
