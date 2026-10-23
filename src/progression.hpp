#pragma once
#include "championship.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <optional>
#include <string>
#include <string_view>

namespace racing {
// Game policy only: the host owns durable storage, never prices or race rewards.
struct VehicleUpgrades {
    std::array<int,3> ranks{}; // drive, pulse cannon, energy shield
    float driveMultiplier() const { return 1.f+.1f*ranks[0]; }
    float speedBonus() const { return 2.f*ranks[0]; }
    float shotInterval() const { return .8f-.1f*ranks[1]; }
    float shieldCapacity() const { return 100.f+25.f*ranks[2]; }
    float shieldRecharge() const { return 18.f+3.f*ranks[2]; }
};
struct Progression {
    static constexpr int maxCredits=999999999,maxRaces=1000000;
    static constexpr std::size_t maxSaveBytes=16384;
    int credits=0,completedRaces=0,selected=0,chassis=0;
    std::array<VehicleUpgrades,2> vehicles{};
    Championship championship;
    static constexpr int originalPaint=0x1000000;
    std::array<int,4> paint{originalPaint,originalPaint,originalPaint,originalPaint};
    static int payout(int position) {
        constexpr int prizes[]={1200,900,700,500,400,300};
        return position>=1&&position<=6?prizes[position-1]:0;
    }
    int price(int vehicle,int system) const {
        if(vehicle<0||vehicle>1||system<0||system>2)return 0;
        const int rank=vehicles[vehicle].ranks[system];
        constexpr int prices[3][3]={{400,700,1100},{300,550,900},{300,550,900}};
        return rank<3?prices[system][rank]:0;
    }
    bool buy(int vehicle,int system) {
        const int cost=price(vehicle,system);
        if(!cost||credits<cost)return false;
        credits-=cost;++vehicles[vehicle].ranks[system];return true;
    }
    int award(int position) {
        int amount=std::min(payout(position),maxCredits-credits);
        if(!payout(position))return 0;
        credits+=amount;completedRaces=std::min(completedRaces+1,maxRaces);return amount;
    }
    std::string encode() const {
        std::string save="NA5 "+std::to_string(credits)+" "+std::to_string(completedRaces)+" "+std::to_string(selected);
        for(const auto& v:vehicles)for(int rank:v.ranks)save+=" "+std::to_string(rank);
        save+=" "+std::to_string(chassis);
        save+=" | "+championship.encode()+" /";
        for(int color:paint)save+=" "+std::to_string(color);
        return save;
    }
    static std::optional<Progression> decode(std::string_view save) {
        if(save.starts_with("NA5 ")) {
            if(save.size()>maxSaveBytes)return {};
            return decode("NA4 "+std::string(save.substr(4)));
        }
        if(save.starts_with("NA4 ")) {
            if(save.size()>maxSaveBytes)return {};
            auto split=save.find(" / ");if(split==std::string_view::npos)return {};
            auto base=decode("NA3 "+std::string(save.substr(4,split-4)));if(!base)return {};
            auto colors=save.substr(split+3);
            for(int i=0;i<4;++i) {
                auto end=colors.find(' ');auto token=colors.substr(0,end);int value=0;
                auto [ptr,error]=std::from_chars(token.data(),token.data()+token.size(),value);
                if(token.empty()||token.front()<'0'||token.front()>'9'||error!=std::errc{}||ptr!=token.data()+token.size()||value<0||value>originalPaint)return {};
                base->paint[i]=value;
                if(i==3){if(end!=std::string_view::npos)return {};}
                else {if(end==std::string_view::npos)return {};colors.remove_prefix(end+1);}
            }
            return base;
        }
        if(save.starts_with("NA3 ")) {
            if(save.size()>maxSaveBytes)return {};
            auto split=save.find(" | ");if(split==std::string_view::npos)return {};
            auto base=decode("NA2 "+std::string(save.substr(4,split-4)));
            auto league=Championship::decode(save.substr(split+3));
            if(!base||!league)return {};base->championship=*league;return base;
        }
        bool legacy=save.starts_with("NA1 ");
        if(save.size()>128||(!legacy&&!save.starts_with("NA2 ")))return {};
        const std::size_t count=legacy?9:10;
        save.remove_prefix(4);std::array<int,10> fields{};
        for(std::size_t i=0;i<count;++i) {
            auto end=save.find(' ');auto token=save.substr(0,end);
            if(token.empty()||token.front()<'0'||token.front()>'9')return {};
            auto [ptr,error]=std::from_chars(token.data(),token.data()+token.size(),fields[i]);
            if(error!=std::errc{}||ptr!=token.data()+token.size())return {};
            if(i+1==count){if(end!=std::string_view::npos)return {};}
            else {if(end==std::string_view::npos)return {};save.remove_prefix(end+1);}
        }
        if(fields[0]>maxCredits||fields[1]>maxRaces||fields[2]>1)return {};
        Progression p;p.credits=fields[0];p.completedRaces=fields[1];p.selected=fields[2];
        p.chassis=legacy?p.selected:fields[9];
        if(p.chassis>3||((p.chassis==0||p.chassis==2)?0:1)!=p.selected)return {};
        for(int i=0;i<6;++i){if(fields[i+3]>3)return {};p.vehicles[i/3].ranks[i%3]=fields[i+3];}
        return p;
    }
};
}
