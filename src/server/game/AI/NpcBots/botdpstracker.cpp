#include "botdpstracker.h"
#include "Unit.h"

#include <numeric>

/*
Name: bot_dps_tracker
%Complete: 100
Comment: dps taken tracker for NPCBot system by Trickerer (onlysuffering@gmail.com)
DPS trackers may collect data from different bot owners if in party but this overdoing has no significance whatsoever
*/

void DPSTracker::Update(uint32 diff)
{
    if (_active)
    {
        _inactiveTimer += diff;
        _updateTimer += diff;
        _trackTimer += diff;

        if (_inactiveTimer >= DPS_INACTIVE_TIMER)
        {
            _Reset();
        }
        else if (_updateTimer >= DPS_UPDATE_TIMER)
        {
            _updateTimer -= DPS_UPDATE_TIMER;
            _Release();
        }
    }
}

void DPSTracker::_Reset()
{
    if (_active)
    {
        _active = false;

        for (auto& [_, damage_array] : _damages)
            damage_array = {};
        for (auto& [_, dps] : _DPSes)
            dps = 0;

        _updateTimer = 0;
        _inactiveTimer = 0;
        _trackTimer = 0;
    }
}

void DPSTracker::_Release()
{
    for (auto& [guid, damage_array] : _damages)
    {
        uint32 total_damage = std::accumulate(damage_array.cbegin(), damage_array.cend(), 0u);

        _DPSes.insert_or_assign(guid, uint32(total_damage / (0.001f * std::max<uint32>(1 * IN_MILLISECONDS, std::min<uint32>(_trackTimer, MAX_DPS_TRACK_TIME)))));
        //BOT_LOG_ERROR("entities.player", "DPSTracker::Release(): guidlow = %u, time = %u, tick damage %u, total %u, dps = %u",
        //    itr->first, _trackTimer, dmgs[0], total_damage, _DPSes[itr->first]);

        //shift
        std::copy_backward(damage_array.begin(), std::prev(damage_array.end()), damage_array.end());
        damage_array[0] = 0;
    }
}

void DPSTracker::_AccumulateDamage(ObjectGuid guid, uint32 damage)
{
    decltype(_damages)::iterator itr = _damages.find(guid);

    if (itr == _damages.end())
    {
        DamageTakenMap::value_type::second_type dmgs{ damage };
        _damages.emplace(guid, std::move(dmgs));
        return;
    }

    itr->second[0] += damage;
}
//victim is bot owner, bot, party player or party bot; checked in Unit::DealDamage()
void DPSTracker::TrackDamage(Unit const* victim, uint32 damage)
{
    //BOT_LOG_ERROR("entities.player", "DPSTracker::OnDamage(): on %s, damage %u", victim->GetName().c_str(), damage);

    _SetActive();
    _AccumulateDamage(victim->GetGUID(), damage);
}

void DPSTracker::_SetActive()
{
    _inactiveTimer = 0;
    _active = true;
}

uint32 DPSTracker::GetDPSTaken(ObjectGuid guid) const
{
    decltype(_DPSes)::const_iterator itr = _DPSes.find(guid);
    //BOT_LOG_ERROR("entities.player", "DPSTracker::GetDPSTaken(): from %u, damage %u", guid, itr != _DPSes.end() ? itr->second : 0);
    return itr != _DPSes.end() ? itr->second : 0;
}
