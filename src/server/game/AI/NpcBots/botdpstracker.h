#ifndef BOT_DPSTRACKER_H
#define BOT_DPSTRACKER_H

#include "ObjectGuid.h"

#include <unordered_map>

class Unit;

enum DPSTrackerConstants : uint32
{
    DPS_UPDATE_TIMER        =  500, //recalculate dps every x ms
    MAX_DPS_TRACK_TIME      = 5000, //track damage taken for last x ms
    DPS_INACTIVE_TIMER      = 5000  //reset if combat not active for botparty for x ms
};

//maximum tracked damage taken periods of DPS_UPDATE_TIMER during MAX_DPS_TRACK_TIME
inline constexpr std::size_t MAX_DAMAGES = MAX_DPS_TRACK_TIME / DPS_UPDATE_TIMER;

class DPSTracker
{
public:
    void Update(uint32 diff);

    void TrackDamage(Unit const* victim, uint32 damage);
    uint32 GetDPSTaken(ObjectGuid guid) const;

private:
    void _Reset();
    void _Release();
    void _AccumulateDamage(ObjectGuid guid, uint32 damage);
    void _SetActive();

    using DamageTakenMap = std::unordered_map<ObjectGuid /*guid*/, std::array<uint32, MAX_DAMAGES> /*dmgarray*/>;
    using DPSTakenMap = std::unordered_map<ObjectGuid /*guid*/, uint32 /*dps*/>;
    DamageTakenMap _damages;
    DPSTakenMap _DPSes;

    uint32 _updateTimer{};
    uint32 _inactiveTimer{};
    uint32 _trackTimer{};
    bool _active{};
};

#endif
