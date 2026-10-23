#pragma once
#include <algorithm>

namespace racing {
// Simulation seconds. Shared by the player and every AI chassis.
struct CombatState {
    static constexpr float shotInterval=.8f, range=42, shieldHitCost=24,beamDuration=.14f;
    float interval=shotInterval,capacity=100,recharge=18;
    float charge=100,cooldown=0,disruption=0,immunity=0,flash=0,beamTime=0;
    bool shield=false,releaseRequired=false;
    int shots=0,hits=0,blocks=0,received=0,shieldUses=0;
    void tick(float dt,bool held,bool enabled) {
        cooldown=std::max(0.f,cooldown-dt);disruption=std::max(0.f,disruption-dt);
        immunity=std::max(0.f,immunity-dt);flash=std::max(0.f,flash-dt);beamTime=std::max(0.f,beamTime-dt);
        if(!held)releaseRequired=false;
        bool was=shield;
        shield=enabled&&held&&!releaseRequired&&(was?charge>0:charge>=25);
        if(shield&&!was)++shieldUses;
        charge=std::clamp(charge+dt*(shield?-32.f:recharge),0.f,capacity);
        if(charge<=0){shield=false;releaseRequired=true;}
        if(!enabled){shield=false;disruption=0;immunity=0;beamTime=0;}
    }
    bool fire() {
        if(cooldown>0)return false;
        cooldown=interval;beamTime=beamDuration;++shots;return true;
    }
    bool hit() {
        flash=.25f;
        if(shield) {
            ++blocks;charge=std::max(0.f,charge-shieldHitCost);
            if(charge==0){shield=false;releaseRequired=true;}
            return false;
        }
        if(immunity>0)return false;
        ++received;disruption=.65f;immunity=1.65f;return true;
    }
};
}
