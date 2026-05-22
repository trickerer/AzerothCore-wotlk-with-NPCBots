#include "Battleground.h"
#include "BattlegroundMgr.h"
#include "bot_ai.h"
#include "bot_Events.h"
#include "botconfig.h"
#include "botdatamgr.h"
#include "botdpstracker.h"
#include "botlog.h"
#include "botmgr.h"
#include "botspell.h"
#include "bottext.h"
#include "bpet_ai.h"
#include "Chat.h"
#include "CombatPackets.h"
#include "Config.h"
#include "GroupMgr.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Group.h"
#include "InstanceScript.h"
#include "Language.h"
#include "Log.h"
#include "Map.h"
#include "MapMgr.h"
#include "MotionMaster.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "Tokenize.h"
#include "Transport.h"
#include "Vehicle.h"
#include "World.h"
#include "GitRevision.h"
/*
Npc Bot Manager by Trickerer (onlysuffering@gmail.com)
Player NpcBots management
TODO: Move creature hooks here
*/

#ifdef _MSC_VER
# pragma warning(push, 4)
#endif

using namespace std::string_view_literals;

static std::list<BotMgr::delayed_teleport_callback_type> delayed_bot_teleports;

BotMgr::BotMgr(Player* const master) : _owner(master), _dpstracker(new DPSTracker())
{
    _quickrecall = false;
    _update_lock = false;
    _data = nullptr;
}
BotMgr::~BotMgr()
{
    if (_data)
        _data->flags &= NPCBOT_MGR_FLAG_MASK_ALL_DB_ALLOWED;

    delete _dpstracker;
}

void BotMgr::LoadData()
{
    ASSERT(!_data, "Trying to load player %u data a second time", _owner->GetGUID().GetCounter());
    _data = BotDataMgr::SelectOrCreateNpcBotMgrData(_owner->GetGUID());
}

void BotMgr::Initialize()
{
    BotCfg::ReloadConfig();
    BotLogger::Log(NPCBOT_LOG_SYSTEM_START, uint32(0), std::string_view{ GitRevision::GetFileVersionStr() }.substr(0, MAX_BOT_LOG_PARAM_LENGTH));

    BotDataMgr::LoadNpcBots();
    BotDataMgr::LoadWanderMap();
    BotDataMgr::GenerateWanderingBots();
    BotDataMgr::CreateGeneratedBotsSortedGear();
    BotDataMgr::LoadNpcBotGroupData();
    BotDataMgr::LoadNpcBotGearStorage();
    BotDataMgr::LoadNpcBotGearSets();
    BotDataMgr::LoadNpcBotMgrData();
    BotDataMgr::DeleteOldLogs();
}

uint8 BotMgr::GetNpcBotsCount() const
{
    //if (!inWorldOnly)
        return (uint8)_bots.size();

    //CRITICAL SECTION
    //inWorldOnly is only for one-shot cases (opcodes, etc.)
    //maybe convert to (bot && bot->isInWorld()) ?
    //uint8 count = 0;
    //for (BotMap::const_iterator itr = _bots.begin(); itr != _bots.end(); ++itr)
    //    if (ObjectAccessor::GetObjectInWorld(itr->first, (Creature*)nullptr))
    //        ++count;
    //return count;
}

uint8 BotMgr::GetNpcBotsCountByRole(uint32 roles) const
{
    return std::ranges::count_if(_bots, [=](BotMap::value_type const& kv) { return kv.second && (roles & kv.second->GetBotRoles()); });
}

uint8 BotMgr::GetNpcBotsCountByVehicleEntry(uint32 creEntry) const
{
    return std::ranges::count_if(_bots, [=](BotMap::value_type const& kv) { return kv.second && kv.second->GetVehicle() && kv.second->GetVehicleBase()->GetEntry() == creEntry; });
}

uint8 BotMgr::GetNpcBotSlot(Creature const* bot) const
{
    uint8 count = 1;
    for (auto const& [_, mbot] : _bots)
    {
        if (mbot == bot)
            break;
        ++count;
    }
    return count;
}

uint8 BotMgr::GetNpcBotSlotByRole(uint32 roles, Creature const* bot) const
{
    uint8 count = 1;
    for (auto const& [_, mbot] : _bots)
    {
        if (roles & mbot->GetBotRoles())
        {
            if (mbot == bot)
                break;
            if (!(roles == BOT_ROLE_DPS && (mbot->GetBotRoles() & BOT_ROLE_TANK)))
                ++count;
        }
    }
    return count;
}

uint32 BotMgr::GetAllNpcBotsClassMask() const
{
    uint32 classMask = 0;
    for (auto const& [_, mbot] : _bots)
        classMask |= (1u << (BotMgr::GetBotEquipmentClass(mbot->GetBotClass()) - 1));
    return classMask;
}

bool BotMgr::LimitBots(Map const* map)
{
    if (map->IsBattlegroundOrArena())
        return true;

    if (BotCfg::LimitNpcBotsInDungeons() && map->IsNonRaidDungeon())
        return true;
    if (BotCfg::LimitNpcBotsInRaids() && map->IsRaid())
        return true;

    return false;
}

bool BotMgr::IsBotContestedPvP(Creature const* bot)
{
    return bot->GetBotAI()->IsContestedPvP();
}

void BotMgr::SetBotContestedPvP(Creature const* bot)
{
    bot->GetBotAI()->SetContestedPvP();
}

bool BotMgr::CanBotParryWhileCasting(Creature const* bot)
{
    switch (bot->GetBotClass())
    {
        case BOT_CLASS_SEA_WITCH:
            return true;
        default:
            return false;
    }
}

bool BotMgr::IsWanderingWorldBot(Creature const* bot)
{
    return bot->IsWandererBot() && (!bot->FindMap() || !bot->GetMap()->GetEntry() || bot->GetMap()->GetEntry()->IsWorldMap());
}

void BotMgr::Update(uint32 diff)
{
    _dpstracker->Update(diff);

    if (!HaveBot())
        return;

    //ObjectGuid guid;
    bool partyCombat = IsPartyInCombat(false);
    bool restrictBots = RestrictBots(_bots.begin()->second, false);

    if (partyCombat)
        bot_ai::CalculateAoeSpots(_owner, _aoespots);

    _update_lock = true;

    for (auto const& [_, bot] : _bots)
    {
        bot_ai* ai = bot->GetBotAI();

        if (ai->IAmFree())
            continue;

        if (!bot->IsInWorld() || (bot->IsSummon() && !bot->IsInMap(_owner)))
        {
            ai->CommonTimers(diff);
            continue;
        }

        if (partyCombat == false || _owner->InBattleground())
            ai->UpdateReviveTimer(diff);

        //bot->IsAIEnabled = true;

        if (ai->GetReviveTimer() <= diff)
        {
            if (bot->IsInMap(_owner) && !bot->IsAlive() && !ai->IsDuringTeleport() && _owner->IsAlive() && !_owner->IsInCombat() &&
                !_owner->IsBeingTeleported() && !_owner->GetMap()->IsBattleArena() && !_owner->IsInFlight() &&
                !_owner->HasUnitFlag2(UNIT_FLAG2_FEIGN_DEATH) && !_owner->HasInvisibilityAura() && !_owner->HasStealthAura())
            {
                _reviveBot(bot);
                continue;
            }

            ai->SetReviveTimer(urand(1000, 5000));
        }

        if (_owner->IsAlive() && (bot->IsAlive() || restrictBots) && !bot->IsSummon() && !ai->IsTempBot() && !ai->IsDuringTeleport() &&
            (restrictBots || bot->GetMap() != _owner->GetMap() ||
            (!bot->GetBotAI()->HasBotCommandState(BOT_COMMAND_STAY) && _owner->GetDistance(bot) > SIZE_OF_GRIDS)))
        {
            //_owner->m_Controlled.erase(bot);
            TeleportBot(bot, _owner->GetMap(), _owner, _quickrecall);
            continue;
        }

        ai->canUpdate = true;
        bot->Update(diff);
        ai->canUpdate = false;
    }

    _update_lock = false;

    while (!_delayedRemoveList.empty())
    {
        decltype(_delayedRemoveList)::iterator itr = _delayedRemoveList.begin();
        RemoveBot(itr->first, itr->second);
    }

    if (_quickrecall)
    {
        _quickrecall = false;
        _data->RemoveFlag(NPCBOT_MGR_FLAG_HIDE_BOTS);
    }
}

bool BotMgr::RestrictBots(Creature const* bot, bool add) const
{
    if (!_owner->FindMap())
        return true;

    if (_owner->IsInFlight())
        return true;

    if (_data->HasFlag(NPCBOT_MGR_FLAG_HIDE_BOTS))
        return true;

    Map const* currMap = _owner->GetMap();

    if (!BotCfg::IsMapAllowedForBots(currMap))
        return true;

    if (LimitBots(currMap))
    {
        Group const* gr = _owner->GetGroup();

        //if bot is not in instance group - deny (only if trying to teleport to instance)
        if (add)
        {
            if (!gr || !gr->IsMember(bot->GetGUID()))
                return true;

            //teleporting raid member bot to non-rain dungeon: prioritize owner sub-group members
            if (gr->isRaidGroup() && currMap->IsNonRaidDungeon())
            {
                uint32 max_members = currMap->ToInstanceMap()->GetMaxPlayers();
                if (gr->GetMembersCount() > max_members)
                {
                    uint8 owner_subgroup = gr->GetMemberGroup(_owner->GetGUID());
                    if (owner_subgroup != gr->GetMemberGroup(bot->GetGUID()))
                    {
                        const std::vector<Unit*> members = GetAllGroupMembers(gr);
                        uint32 sub_members = 0;
                        uint32 sub_members_inside = 0;
                        for (auto const& mslot : gr->GetMemberSlots())
                        {
                            if (mslot.group == owner_subgroup)
                            {
                                decltype(members)::const_iterator it = std::ranges::find_if(members, [&](Unit const* unit) { return mslot.guid == unit->GetGUID(); });
                                if (it != members.cend() && (*it)->IsInMap(_owner))
                                    ++sub_members_inside;
                                if (++sub_members >= max_members)
                                    break;
                            }
                        }
                        if (sub_members >= max_members || sub_members_inside < sub_members)
                            return true;
                    }
                }
            }
        }

        uint32 max_players = 0;
        if (currMap->IsDungeon())
            max_players = currMap->ToInstanceMap()->GetMaxPlayers();
        else if (currMap->IsBattlegroundOrArena())
            max_players = _owner->GetBattleground()->GetMaxPlayersPerTeam();

        if (max_players)
        {
            uint32 curPlayers;
            if (gr && currMap->IsBattlegroundOrArena())
            {
                curPlayers = std::ranges::count_if(GetAllGroupMembers(gr), [this](Unit const* u) {
                    return u->IsInWorld() && u->IsInMap(_owner) && !(u->IsNPCBot() && u->ToCreature()->IsTempBot());
                });
            }
            else
                curPlayers = currMap->GetPlayersCountExceptGMs();
            if (curPlayers + uint32(add) > max_players)
                return true;
        }
    }

    return false;
}

bool BotMgr::IsPartyInCombat(bool is_pvp) const
{
    if (_owner->IsInCombat() && (!is_pvp || _owner->GetCombatTimer() > 0))
        return true;
    return std::ranges::any_of(_bots, [=](BotMap::value_type const& kv) { return kv.second->IsInCombat() && (!is_pvp || kv.second->GetCombatManager().HasPvPCombat()); });
}

bool BotMgr::HasBotClass(uint8 botclass) const
{
    return std::ranges::any_of(_bots, [=](BotMap::value_type const& kv) { return kv.second->GetBotClass() == botclass; });
}

bool BotMgr::HasBotWithSpec(uint8 spec, bool alive) const
{
    return std::ranges::any_of(_bots, [=](BotMap::value_type const& kv) { return kv.second->GetBotAI()->GetSpec() == spec && (!alive || kv.second->IsAlive()); });
}

bool BotMgr::HasBotPetType(uint32 petType) const
{
    return std::ranges::any_of(_bots, [=](BotMap::value_type const& kv) { return kv.second->GetBotsPet() && kv.second->GetBotAI()->GetAIMiscValue(BOTAI_MISC_PET_TYPE) == petType; });
}

bool BotMgr::IsBeingResurrected(WorldObject const* corpse) const
{
    std::vector<Unit const*> casters;
    if (_owner->IsNonMeleeSpellCast(false, true, true))
        casters.push_back(_owner);
    for (auto const& [_, bot] : _bots)
    {
        if (bot->IsNonMeleeSpellCast(false, true, true))
            casters.push_back(bot);
    }

    if (Group const* group = _owner->GetGroup())
    {
        for (GroupReference const* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player const* player = itr->GetSource();
            if (!player || player == _owner || player->FindMap() != corpse->GetMap())
                continue;

            if (player->IsNonMeleeSpellCast(false, true, true))
                casters.push_back(player);

            if (player->HaveBot())
            {
                for (auto const& [_, bot] : *player->GetBotMgr()->GetBotMap())
                {
                    if (bot->IsNonMeleeSpellCast(false, true, true))
                        casters.push_back(bot);
                }
            }
        }
    }

    for (Unit const* caster : casters)
    {
        if (Spell const* spell = caster->GetCurrentSpell(CURRENT_GENERIC_SPELL))
        {
            if (corpse->GetGUID() == (corpse->ToCorpse() ? spell->m_targets.GetCorpseTargetGUID() : spell->m_targets.GetUnitTargetGUID()))
                return true;
        }
    }

    return false;
}

void BotMgr::_reviveBot(Creature* bot, WorldLocation* dest)
{
    if (bot->IsAlive() || !bot->IsInWorld())
        return;

    if (!bot->GetBotAI()->IAmFree())
    {
        if (!dest)
            bot->CastSpell(bot, COSMETIC_RESURRECTION, false);

        if (!dest)
            dest = bot->GetBotOwner();

        bot->NearTeleportTo(dest->GetPositionX(), dest->GetPositionY(), dest->GetPositionZ(), dest->GetOrientation());
        //some weird pos manipulation
        if (dest != bot)
            bot->Relocate(dest);
    }

    bot->SetDisplayId(bot->GetNativeDisplayId(), bot->GetCreatureTemplate()->GetFirstValidModel()->DisplayScale);
    bot->ReplaceAllNpcFlags(NPCFlags(bot->GetCreatureTemplate()->npcflag));
    bot->ClearUnitState(uint32(UNIT_STATE_ALL_STATE & ~(UNIT_STATE_IGNORE_PATHFINDING | UNIT_STATE_NO_ENVIRONMENT_UPD)));
    bot->ReplaceAllUnitFlags(UnitFlags(0));
    bot->SetLootRecipient(nullptr);
    bot->SetPvP(bot->GetBotOwner()->IsPvP());
    bot->Motion_Initialize();
    bot->setDeathState(DeathState::Alive);
    //bot->GetBotAI()->Reset();
    bot->RefreshSwimmingFlag();
    bot->GetBotAI()->SetShouldUpdateStats();

    uint8 restore_factor = (bot->IsWandererBot() || (!bot->GetBotAI()->IAmFree() && bot->GetBotOwner()->InBattleground())) ? 1 : 4;
    bot->SetHealth(bot->GetMaxHealth() / restore_factor); //25% of max health
    if (bot->GetMaxPower(POWER_MANA) > 1)
        bot->SetPower(POWER_MANA, bot->GetMaxPower(POWER_MANA) / restore_factor); //25% of max mana

    if (IsWanderingWorldBot(bot))
        bot->ResetPlayerDamageReq();

    if (!bot->GetBotAI()->IAmFree() && !bot->GetBotAI()->HasBotCommandState(BOT_COMMAND_MASK_UNMOVING))
        bot->GetBotAI()->SetBotCommandState(BOT_COMMAND_FOLLOW, true);
}

Creature* BotMgr::GetBot(ObjectGuid guid) const
{
    decltype(_bots)::const_iterator itr = _bots.find(guid);
    return itr != _bots.end() ? itr->second : nullptr;
}

Creature* BotMgr::GetBotByName(std::string_view name) const
{
    std::wstring wname;
    if (Utf8toWStr(name, wname))
    {
        wstrToLower(wname);
        for (auto const& [_, bot] : _bots)
        {
            if (!bot)
                continue;

            std::string_view basename = bot->GetName();
            if (CreatureLocale const* creatureInfo = sObjectMgr->GetCreatureLocale(bot->GetEntry()))
            {
                uint32 loc = _owner->GetSession()->GetSessionDbLocaleIndex();
                if (creatureInfo->Name.size() > loc && !creatureInfo->Name[loc].empty())
                    basename = creatureInfo->Name[loc];
            }

            std::wstring wbname;
            if (!Utf8toWStr(basename, wbname))
                continue;

            wstrToLower(wbname);
            if (wbname == wname)
                return bot;
        }
    }

    return nullptr;
}

std::vector<Creature*> BotMgr::GetAllBotsByClass(uint8 botclass) const
{
    std::vector<Creature*> foundBots;
    foundBots.reserve(_bots.size());
    for (auto const& [_, bot] : _bots)
    {
        if (!bot|| !bot->IsInWorld() || !bot->IsAlive())
            continue;

        if (bot->GetBotClass() == botclass)
            foundBots.push_back(bot);
    }

    return foundBots;
}

void BotMgr::OnOwnerSetGameMaster(bool on)
{
    for (auto const& [_, bot] : _bots)
    {
        if (!bot)
            continue;

        bot->SetFaction(_owner->GetFaction());
        //bot->getHostileRefManager().setOnlineOfflineState(!on);
        bot->SetByteValue(UNIT_FIELD_BYTES_2, 1, _owner->GetByteValue(UNIT_FIELD_BYTES_2, 1)); //pvp state

        if (on && bot->IsInWorld())
            bot->CombatStop(true);

        if (Unit* pet = bot->GetBotsPet())
        {
            pet->SetFaction(_owner->GetFaction());
            //pet->getHostileRefManager().setOnlineOfflineState(!on);
            pet->SetByteValue(UNIT_FIELD_BYTES_2, 1, _owner->GetByteValue(UNIT_FIELD_BYTES_2, 1)); //pvp state

            if (on)
                pet->CombatStop(true);
        }
    }
}

void BotMgr::OnTeleportFar(uint32 mapId, float x, float y, float z, float ori)
{
    Map* newMap = sMapMgr->CreateBaseMap(mapId);
    Position pos{ x, y, z, ori };

    for (auto const& [_, bot] : _bots)
    {
        if (bot->IsTempBot())
            continue;
        else if (bot->IsSummon())
        {
            bot->GetBotAI()->canUpdate = false;
            continue;
        }

        //_owner->m_Controlled.erase(bot);
        TeleportBot(bot, newMap, &pos);
    }
}

void BotMgr::_teleportBot(Creature* bot, Map* newMap, float x, float y, float z, float ori, bool quick, bool reset, bot_ai* detached_ai)
{
    bot_ai* botai = detached_ai ? detached_ai : bot->GetBotAI();
    ASSERT(botai);
    botai->AbortTeleport();
    botai->SetIsDuringTeleport(true);
    botai->KillEvents(true);
    bot->m_Events.KillAllEvents(false);

    BotLogger::Log(NPCBOT_LOG_TELEPORT_START, bot, bot->IsInGrid(), bot->IsWandererBot(), botai->CanAppearInWorld(), newMap->GetId(), bool(reset));

    BotMgr::AddDelayedTeleportCallback([bot, botai, newMap, x, y, z, ori, quick, reset]() {
        if (bot->GetVehicle())
            bot->ExitVehicle();

        if (bot->GetTransport())
        {
            bot->ClearUnitState(UNIT_STATE_IGNORE_PATHFINDING);
            bot->GetTransport()->RemovePassenger(bot, true);
        }

        Map* mymap = bot->FindMap();
        if (mymap)
        {
            bot->BotStopMovement();

            if (mymap != newMap)
            {
                bot->RemoveAurasByType(SPELL_AURA_MOD_STUN);
                bot->RemoveAurasByType(SPELL_AURA_MOD_FEAR);
                bot->RemoveAurasByType(SPELL_AURA_MOD_CONFUSE);
                bot->RemoveAurasByType(SPELL_AURA_MOD_ROOT);
                bot->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TELEPORTED);
            }

            bot->InterruptNonMeleeSpells(true);

            if (bot->IsInWorld())
            {
                botai->UnsummonAll(!botai->IAmFree() || botai->IsWanderer());

                if (Battleground* bg = bot->GetBotBG())
                    bg->EventBotDroppedFlag(bot);

                bot->CastSpell(bot, COSMETIC_TELEPORT_EFFECT, true);

                if (!bot->IsFreeBot())
                    if (InstanceScript* iscr = bot->GetBotOwner()->GetInstanceScript())
                        iscr->OnNPCBotLeave(bot);

                bot->RemoveFromWorld();
            }

            bot->RemoveAllGameObjects();
            bot->m_Events.KillAllEvents(false);
            bot->CombatStop();
            bot->ClearComboPoints();
            bot->ClearComboPointHolders();

            if (bot->IsInGrid())
                mymap->RemoveFromMap(bot, false);
        }

        if (botai->IAmFree())
        {
            bot->Relocate(x, y, z, ori);
            if (bot->FindMap())
                bot->ResetMap();
            bot->SetMap(newMap);
            if (!bot->IsWandererBot() && !botai->CanAppearInWorld())
            {
                botai->AbortTeleport();
                TeleportFinishEvent* delayedTeleportEvent = new TeleportFinishEvent(botai, reset);
                botai->GetEvents()->AddEvent(delayedTeleportEvent, botai->GetEvents()->CalculateTime(urand(5000, 8000)));
                botai->SetTeleportFinishEvent(delayedTeleportEvent);
                return;
            }

            BotLogger::Log(NPCBOT_LOG_TELEPORT_FINISH, bot, bot->IsInGrid(), bot->IsWandererBot(), botai->CanAppearInWorld(), newMap->GetId(), bool(reset));

            newMap->AddToMap(bot);
            if (reset)
                botai->Reset();
            botai->SetIsDuringTeleport(false);
            botai->ResetContestedPvP();

            if (newMap->IsBattleground())
            {
                Battleground* bg = botai->GetBG();
                if (!bg)
                {
                    BotDataMgr::DespawnWandererBot(bot->GetEntry());
                    return;
                }

                if (newMap != mymap)
                {
                    //we teleport from base non-instanced map which normally doesn't exist
                    if (mymap)
                        ASSERT(mymap->GetPlayersCountExceptGMs() == 0);

                    bg->AddBot(bot);
                }

                if (!bot->IsAlive())
                {
                    ObjectGuid shGuid = ObjectGuid::Empty;
                    float mindist = 0.0f;
                    for (ObjectGuid bgCreGuid : bg->BgCreatures)
                    {
                        if (Creature const* bgCre = newMap->GetCreature(bgCreGuid))
                        {
                            if (bgCre->IsSpiritService())
                            {
                                float dist = bot->GetExactDist2d(bgCre);
                                if (shGuid == ObjectGuid::Empty || dist < mindist)
                                {
                                    mindist = dist;
                                    shGuid = bgCreGuid;
                                }
                            }
                        }
                    }
                    if (shGuid)
                        bg->AddPlayerToResurrectQueue(shGuid, bot->GetGUID());
                    else
                    {
                        BOT_LOG_ERROR("npcbots", "TeleportBot: Bot {} '{}' can't find SpiritHealer in bg {}!",
                            bot->GetEntry(), bot->GetName().c_str(), bg->GetName().c_str());
                    }
                }
            }

            botai->canUpdate = true;

            return;
        }

        //update group member online state
        if (Group* gr = bot->GetBotOwner()->GetGroup())
            if (gr->IsMember(bot->GetGUID()))
                gr->SendUpdate();

        botai->AbortTeleport();
        TeleportFinishEvent* finishEvent = new TeleportFinishEvent(botai, reset);
        uint64 delay = quick ? urand(500, 1500) : urand(5000, 8000);
        botai->GetEvents()->AddEvent(finishEvent, botai->GetEvents()->CalculateTime(delay));
        botai->SetTeleportFinishEvent(finishEvent);
    });
}

void BotMgr::TeleportBot(Creature* bot, Map* newMap, Position const* pos, bool quick, bool reset, bot_ai* detached_ai)
{
    _teleportBot(bot, newMap, pos->GetPositionX(), pos->GetPositionY(), pos->GetPositionZ(), pos->GetOrientation(), quick, reset, detached_ai);
}

void BotMgr::CleanupsBeforeBotDelete(ObjectGuid guid, uint8 removetype)
{
    decltype(_bots)::const_iterator itr = _bots.find(guid);
    ASSERT(itr != _bots.end(), "Trying to remove bot which does not belong to this botmgr(b)!!");
    //ASSERT(_owner->IsInWorld(), "Trying to remove bot while not in world(b)!!");

    Creature* bot = itr->second;

    ASSERT(bot->GetCreator() && bot->GetCreator()->GetGUID() == _owner->GetGUID());

    if (!bot->IsTempBot())
        RemoveBotFromBGQueue(bot);

    if (removetype != BOT_REMOVE_LOGOUT || bot->GetBotAI()->HasSharedOwner(_owner->GetGUID().GetCounter()))
        RemoveBotFromGroup(bot);

    CleanupsBeforeBotDelete(bot);
}

void BotMgr::CleanupsBeforeBotDelete(Creature* bot)
{
    //don't allow removing bots while they are teleporting
    if (!bot->IsInWorld())
        bot->GetBotAI()->AbortTeleport();

    if (bot->GetVehicle())
        bot->ExitVehicle();

    //remove any summons
    bot->GetBotAI()->UnsummonAll(false);
    bot->AttackStop();
    bot->CombatStopWithPets(true);

    //bot->SetOwnerGUID(ObjectGuid::Empty);
    //_owner->m_Controlled.erase(bot);
    bot->SetControlledByPlayer(false);
    //bot->RemoveUnitFlag(UNIT_FLAG_PLAYER_CONTROLLED);
    //bot->RemoveUnitFlag(UNIT_FLAG_PVP_ATTACKABLE);
    bot->SetByteValue(UNIT_FIELD_BYTES_2, 1, 0);
    bot->SetCreator(nullptr);
    //bot->SetCreatorGUID(ObjectGuid::Empty);

    Map* map = bot->FindMap();
    if (!map || map->IsDungeon() || bot->IsTempBot())
        bot->RemoveFromWorld();
}

void BotMgr::RemoveAllSummonedBots()
{
    if (!_bots.empty())
    {
        GuidVector summoned_bots;
        summoned_bots.reserve(_bots.size());
        for (auto const& [guid, bot] : _bots)
            if (bot->IsSummon() && !bot->IsTempBot())
                summoned_bots.push_back(guid);
        for (ObjectGuid guid : summoned_bots)
            RemoveBot(guid, BOT_REMOVE_UNSUMMON);
    }
}

void BotMgr::RemoveAllBots(uint8 removetype)
{
    while (!_bots.empty())
        RemoveBot(_bots.begin()->second->GetGUID(), removetype);
}
//Bot is being abandoned by player
void BotMgr::RemoveBot(ObjectGuid guid, uint8 removetype)
{
    decltype(_bots)::const_iterator itr = _bots.find(guid);
    ASSERT(itr != _bots.end(), "Trying to remove bot which does not belong to this botmgr(a)!!");
    //ASSERT(_owner->IsInWorld(), "Trying to remove bot while not in world(a)!!");

    Creature* bot = itr->second;

    if (_update_lock)
    {
        _delayedRemoveList.emplace_back(guid, BotRemoveType(removetype));
        return;
    }
    else if (!_delayedRemoveList.empty())
        std::erase_if(_delayedRemoveList, [=](decltype(_delayedRemoveList)::value_type const& p) { return p.first == guid; });

    if (bot->IsSummon() && !bot->GetBotAI()->IsTempBot())
    {
        RemoveBotFromBGQueue(bot);
        RemoveBotFromGroup(bot);
        bot->SetCreator(nullptr);
        if (Unit* bpet = bot->GetBotsPet())
            bpet->SetCreator(nullptr);
        bot->GetBotAI()->ResetBotAI(BOTAI_RESET_LOGOUT | BOTAI_RESET_DISMISS);
        BotDataMgr::DespawnDungeonBot(bot->GetEntry());
        _bots.erase(itr);
        return;
    }

    CleanupsBeforeBotDelete(guid, removetype);

    if (_owner->GetSession()->PlayerLogout() && bot->IsInGrid() && bot->FindMap() && bot->FindMap()->GetEntry()->Instanceable())
        bot->FindMap()->RemoveFromMap(bot, false);

    ////remove control bar
    //if (GetNpcBotsCount() <= 1 && !_owner->GetPetGUID() && _owner->m_Controlled.empty())
    //    _owner->SendRemoveControlBar();

    _bots.erase(itr);

    if (bot->GetBotAI()->IsTempBot())
        return;

    BotAIResetType resetType;
    switch (removetype)
    {
        case BOT_REMOVE_DISMISS: case BOT_REMOVE_UNAFFORD: resetType = bot->GetBotAI()->IsSharedBot() ? BOTAI_RESET_UNBIND : BOTAI_RESET_DISMISS; break;
        case BOT_REMOVE_UNBIND:                            resetType = BOTAI_RESET_UNBIND;                                                        break;
        default:                                           resetType = BOTAI_RESET_LOGOUT;                                                        break;
    }
    bot->GetBotAI()->ResetBotAI(resetType);

    bot->SetFaction(bot->GetCreatureTemplate()->faction);
    bot->SetLevel(bot->GetCreatureTemplate()->minlevel);

    if (resetType == BOTAI_RESET_DISMISS)
    {
        BotDataMgr::ResetNpcBotTransmogData(bot->GetEntry(), false);
        uint32 newOwner = 0;
        BotDataMgr::UpdateNpcBotData(bot->GetEntry(), NPCBOT_UPDATE_OWNER, &newOwner);
        NpcBotData::SharedOwnersContainer sharedOwners{};
        BotDataMgr::UpdateNpcBotData(bot->GetEntry(), NPCBOT_UPDATE_SHARED_OWNERS, &sharedOwners);
    }
}

void BotMgr::UnbindBot(ObjectGuid guid)
{
    Creature const* bot = GetBot(guid);
    ASSERT(bot);

    RemoveBot(guid, BOT_REMOVE_UNBIND);
    bot->GetBotAI()->SetBotCommandState(BOT_COMMAND_UNBIND);
}
BotAddResult BotMgr::RebindBot(Creature* bot)
{
    BotAddResult res = AddBot(bot);
    if (res == BOT_ADD_SUCCESS)
        bot->GetBotAI()->RemoveBotCommandState(BOT_COMMAND_UNBIND);
    return res;
}

BotAddResult BotMgr::AddDungeonBot(Creature* bot)
{
    BotAddResult add_res = AddBot(bot);
    if (add_res != BOT_ADD_SUCCESS)
        return add_res;

    uint32 lfg_roles = BotDataMgr::BotToLFGRoles(bot->GetBotAI()->GetBotRoles());
    _owner->GetGroup()->SetLfgRoles(bot->GetGUID(), lfg_roles);

    return BOT_ADD_SUCCESS;
}

BotAddResult BotMgr::AddBot(Creature* bot)
{
    ASSERT(bot->IsNPCBot());
    ASSERT(bot->GetBotAI() != nullptr);

    bool owned = bot->IsSummon() || bot->GetBotAI()->HasOwner(_owner->GetGUID().GetCounter());
    uint8 owned_count = BotDataMgr::GetOwnedBotsCount(_owner->GetGUID(), 0, true);
    uint8 class_count = BotDataMgr::GetOwnedBotsCount(_owner->GetGUID(), bot->GetClassMask(), true);

    if (!BotCfg::IsNpcBotModEnabled())
    {
        ChatHandler ch(_owner->GetSession());
        ch.SendSysMessage(bot_ai::LocalizedNpcText(GetOwner(), BOT_TEXT_BOTADDFAIL_DISABLED));
        return BOT_ADD_DISABLED;
    }
    if (GetBot(bot->GetGUID()))
        return BOT_ADD_ALREADY_HAVE; //Silent error, intended
    if (!bot->GetBotAI()->IAmFree())
    {
        ChatHandler ch(_owner->GetSession());
        ch.PSendSysMessage(bot_ai::LocalizedNpcText(GetOwner(), BOT_TEXT_BOTADDFAIL_OWNED).c_str(), bot->GetName(), bot->GetBotOwner()->GetName());
        return BOT_ADD_NOT_AVAILABLE;
    }
    if (!owned && owned_count >= BotCfg::GetMaxNpcBots(_owner->GetLevel()))
    {
        ChatHandler ch(_owner->GetSession());
        ch.PSendSysMessage(bot_ai::LocalizedNpcText(GetOwner(), BOT_TEXT_HIREFAIL_MAXBOTS).c_str(), BotCfg::GetMaxNpcBots(_owner->GetLevel()));
        return BOT_ADD_MAX_EXCEED;
    }
    if (!owned && BotCfg::GetMaxClassBots() && class_count >= BotCfg::GetMaxClassBots())
    {
        ChatHandler ch(_owner->GetSession());
        ch.PSendSysMessage(bot_ai::LocalizedNpcText(GetOwner(), BOT_TEXT_HIREFAIL_MAXCLASSBOTS).c_str(), class_count, BotCfg::GetMaxClassBots());
        return BOT_ADD_MAX_CLASS_EXCEED;
    }
    //Map* curMap = _owner->GetMap();
    //if (!temporary && LimitBots(curMap))
    //{
    //    InstanceMap* map = curMap->ToInstanceMap();
    //    uint32 count = map->GetPlayersCountExceptGMs();
    //    if (count >= map->GetMaxPlayers())
    //    {
    //        ChatHandler ch(_owner->GetSession());
    //        ch.PSendSysMessage("Instance players limit exceed ({} of {})", count, map->GetMaxPlayers());
    //        return BOT_ADD_INSTANCE_LIMIT;
    //    }
    //}
    if (!owned)
    {
        uint32 cost = BotCfg::GetNpcBotCostHire(_owner->GetLevel(), bot->GetBotClass());
        if (!_owner->HasEnoughMoney(cost))
        {
            ChatHandler ch(_owner->GetSession());
            std::ostringstream mss;
            mss << bot_ai::LocalizedNpcText(GetOwner(), BOT_TEXT_HIREFAIL_COST) << " ("
                << BotCfg::GetNpcBotCostStr(_owner->GetLevel(), bot->GetBotClass()) << ")!";
            ch.SendSysMessage(mss.view());
            return BOT_ADD_CANT_AFFORD;
        }

        _owner->ModifyMoney(-(int32(cost)));
    }

    bot->GetBotAI()->canUpdate = false;

    if (!bot->IsAlive())
        _reviveBot(bot);

    bot->GetBotAI()->UnsummonAll(false);

    _bots[bot->GetGUID()] = bot;

    ASSERT(!bot->GetCreator());
    //ASSERT(!bot->GetOwnerGUID());
    //bot->SetOwnerGUID(_owner->GetGUID());
    bot->SetCreator(_owner); //needed in case of FFAPVP
    //bot->SetCreatorGUID(_owner->GetGUID());
    //_owner->m_Controlled.insert(bot);
    bot->SetControlledByPlayer(true);
    //bot->SetUnitFlag(UNIT_FLAG_PLAYER_CONTROLLED);
    bot->SetByteValue(UNIT_FIELD_BYTES_2, 1, _owner->GetByteValue(UNIT_FIELD_BYTES_2, 1));
    bot->SetFaction(_owner->GetFaction());
    bot->SetPhaseMask(_owner->GetPhaseMask(), true);

    bot->GetBotAI()->SetBotOwner(_owner);

    bot->GetBotAI()->Reset();

    bot->LowerPlayerDamageReq(bot->GetMaxHealth(), false);

    if (!bot->IsInWorld())
        TeleportBot(bot, _owner->GetMap(), _owner);

    if (!bot->GetBotAI()->IsTempBot())
    {
        if (!bot->IsSummon())
        {
            uint32 newOwner = _owner->GetGUID().GetCounter();
            if (!bot->GetBotAI()->HasSharedOwner(newOwner))
                BotDataMgr::UpdateNpcBotData(bot->GetEntry(), NPCBOT_UPDATE_OWNER, &newOwner);
        }

        bot->GetBotAI()->SetBotCommandState(BOT_COMMAND_FOLLOW, true);
        if (bot->GetBotAI()->HasRole(BOT_ROLE_PARTY))
            AddBotToGroup(bot);
    }

    return BOT_ADD_SUCCESS;
}

bool BotMgr::AddBotToGroup(Creature* bot)
{
    ASSERT(GetBot(bot->GetGUID()));

    Group* gr = _owner->GetGroup();
    if (gr)
    {
        if (gr->IsMember(bot->GetGUID()))
            return true;

        if (gr->IsFull())
        {
            if (!gr->isRaidGroup()) //non-raid group is full
                gr->ConvertToRaid();
            else
                return false;
        }
    }
    else
    {
        gr = new Group;
        if (!gr->Create(_owner))
        {
            delete gr;
            return false;
        }
        sGroupMgr->AddGroup(gr);
    }

    if (gr->AddMember(bot))
    {
        if (!bot->GetBotAI()->HasRole(BOT_ROLE_PARTY))
            bot->GetBotAI()->ToggleRole(BOT_ROLE_PARTY, true);

        return true;
    }

    return false;
}

void BotMgr::RemoveBotFromBGQueue(Creature const* bot)
{
    for (auto i : NPCBots::index_array<uint32, PLAYER_MAX_BATTLEGROUND_QUEUES>)
    {
        if (BattlegroundQueueTypeId bgQueueTypeId = _owner->GetBattlegroundQueueTypeId(i))
            sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeId).RemovePlayer(bot->GetGUID(), true);
    }
}

bool BotMgr::RemoveBotFromGroup(Creature* bot)
{
    ASSERT(GetBot(bot->GetGUID()));

    Group* gr = _owner->GetGroup();
    if (!gr || !gr->IsMember(bot->GetGUID()))
        return false;

    RemoveBotFromBGQueue(bot);

    if (bot->GetBotAI()->HasRole(BOT_ROLE_PARTY) && !_owner->GetSession()->PlayerLogout())
        bot->GetBotAI()->ToggleRole(BOT_ROLE_PARTY, true);

    //debug
    //if (gr->RemoveMember(bot->GetGUID()))
    //    BOT_LOG_ERROR("entities.player", "RemoveBotFromGroup(): bot %s removed from group", bot->GetName().c_str());
    //else
    //    BOT_LOG_ERROR("entities.player", "RemoveBotFromGroup(): RemoveMember() returned FALSE on bot %s", bot->GetName().c_str());

    gr->RemoveMember(bot->GetGUID());

    //if removed from group while in instance / bg then remove from world immediately
    if (bot->IsInWorld() && !bot->IsSummon() && RestrictBots(bot, true))
        TeleportBot(bot, bot->GetMap(), bot);

    return true;
}

bool BotMgr::RemoveAllBotsFromGroup()
{
    for (auto const& [_, bot] : _bots)
        RemoveBotFromGroup(bot);

    return true;
}

uint8 BotMgr::BotClassByClassName(std::string_view className)
{
    static const std::map<std::string_view, uint8> BotClassNamesMap = {
        { "warrior"sv, BOT_CLASS_WARRIOR },
        { "paladin"sv, BOT_CLASS_PALADIN },
        { "hunter"sv, BOT_CLASS_HUNTER },
        { "rogue"sv, BOT_CLASS_ROGUE },
        { "priest"sv, BOT_CLASS_PRIEST },
        { "deathknight"sv, BOT_CLASS_DEATH_KNIGHT },
        { "death_knight"sv, BOT_CLASS_DEATH_KNIGHT },
        { "shaman"sv, BOT_CLASS_SHAMAN },
        { "mage"sv, BOT_CLASS_MAGE },
        { "warlock"sv, BOT_CLASS_WARLOCK },
        { "druid"sv, BOT_CLASS_DRUID },
        { "blademaster"sv, BOT_CLASS_BM },
        { "blade_master"sv, BOT_CLASS_BM },
        { "sphynx"sv, BOT_CLASS_SPHYNX },
        { "obsidiandestroyer"sv, BOT_CLASS_SPHYNX },
        { "obsidian_destroyer"sv, BOT_CLASS_SPHYNX },
        { "destroyer"sv, BOT_CLASS_SPHYNX },
        { "archmage"sv, BOT_CLASS_ARCHMAGE },
        { "dreadlord"sv, BOT_CLASS_DREADLORD },
        { "spellbreaker"sv, BOT_CLASS_SPELLBREAKER },
        { "spell_breaker"sv, BOT_CLASS_SPELLBREAKER },
        { "darkranger"sv, BOT_CLASS_DARK_RANGER },
        { "dark_ranger"sv, BOT_CLASS_DARK_RANGER },
        { "necromancer"sv, BOT_CLASS_NECROMANCER },
        { "necro"sv, BOT_CLASS_NECROMANCER },
        { "seawitch"sv, BOT_CLASS_SEA_WITCH },
        { "sea_witch"sv, BOT_CLASS_SEA_WITCH },
        { "cryptlord"sv, BOT_CLASS_CRYPT_LORD},
        { "crypt_lord"sv, BOT_CLASS_CRYPT_LORD }
    };

    //std::transform(className.begin(), className.end(), className.begin(), std::tolower);
    decltype(BotClassNamesMap)::const_iterator ci = BotClassNamesMap.find(className);
    return ci != BotClassNamesMap.cend() ? ci->second : static_cast<uint8>(BOT_CLASS_NONE);
}

uint8 BotMgr::GetBotPlayerClass(uint8 bot_class)
{
    if (bot_class >= BOT_CLASS_EX_START)
    {
        switch (bot_class)
        {
            case BOT_CLASS_BM:
                return BOT_CLASS_WARRIOR;
            case BOT_CLASS_SPHYNX:
                return BOT_CLASS_WARLOCK;
            case BOT_CLASS_ARCHMAGE:
                return BOT_CLASS_MAGE;
            case BOT_CLASS_DREADLORD:
                return BOT_CLASS_WARLOCK;
            case BOT_CLASS_SPELLBREAKER:
                return BOT_CLASS_PALADIN;
            case BOT_CLASS_DARK_RANGER:
                return BOT_CLASS_HUNTER;
            case BOT_CLASS_NECROMANCER:
                return BOT_CLASS_WARLOCK;
            case BOT_CLASS_SEA_WITCH:
                return BOT_CLASS_MAGE;
            case BOT_CLASS_CRYPT_LORD:
                return BOT_CLASS_WARRIOR;
            default:
                BOT_LOG_ERROR("npcbots", "GetPlayerClass: unknown Ex bot class {}!", bot_class);
                return BOT_CLASS_PALADIN;
        }
    }

    return bot_class;
}

uint8 BotMgr::GetBotPlayerRace(uint8 bot_class, uint8 bot_race)
{
    if (bot_class >= BOT_CLASS_EX_START)
    {
        switch (bot_class)
        {
            case BOT_CLASS_BM:
                return RACE_ORC;
            case BOT_CLASS_SPHYNX:
                return RACE_UNDEAD_PLAYER;
            case BOT_CLASS_ARCHMAGE:
                return RACE_HUMAN;
            case BOT_CLASS_DREADLORD:
                return RACE_UNDEAD_PLAYER;
            case BOT_CLASS_SPELLBREAKER:
                return RACE_BLOODELF;
            case BOT_CLASS_DARK_RANGER:
                return RACE_BLOODELF;
            case BOT_CLASS_NECROMANCER:
                return RACE_HUMAN;
            case BOT_CLASS_SEA_WITCH:
                return RACE_TROLL;
            case BOT_CLASS_CRYPT_LORD:
                return RACE_UNDEAD_PLAYER;
            default:
                BOT_LOG_ERROR("npcbots", "GetBotPlayerRace: unknown Ex bot class {}!", bot_class);
                return RACE_HUMAN;
        }
    }

    return bot_race;
}

uint8 BotMgr::GetBotPlayerClass(Creature const* bot)
{
    return GetBotPlayerClass(bot->GetBotAI()->GetBotClass());
}

uint8 BotMgr::GetBotPlayerRace(Creature const* bot)
{
    return GetBotPlayerRace(bot->GetBotAI()->GetBotClass(), bot->GetRace());
}

uint8 BotMgr::GetBotEquipmentClass(uint8 bot_class)
{
    if (bot_class >= BOT_CLASS_EX_START)
    {
        switch (bot_class)
        {
            case BOT_CLASS_BM:
                return BOT_CLASS_WARRIOR;
            case BOT_CLASS_SPHYNX:
                return BOT_CLASS_PALADIN;
            case BOT_CLASS_ARCHMAGE:
                return BOT_CLASS_MAGE;
            case BOT_CLASS_DREADLORD:
                return BOT_CLASS_PALADIN;
            case BOT_CLASS_SPELLBREAKER:
                return BOT_CLASS_PALADIN;
            case BOT_CLASS_DARK_RANGER:
                return BOT_CLASS_HUNTER;
            case BOT_CLASS_NECROMANCER:
                return BOT_CLASS_PALADIN;
            case BOT_CLASS_SEA_WITCH:
                return BOT_CLASS_MAGE;
            case BOT_CLASS_CRYPT_LORD:
                return BOT_CLASS_WARRIOR;
            default:
                BOT_LOG_ERROR("npcbots", "GetBotEquipmentClass: unknown Ex bot class {}!", bot_class);
                return BOT_CLASS_PALADIN;
        }
    }

    return BotMgr::GetBotPlayerClass(bot_class);
}

BotStatMods BotMgr::GetBotStatModByUnitStat(Stats stat)
{
    BotStatMods bot_stat;
    switch (stat)
    {
        case STAT_STRENGTH:  bot_stat = BotStatMods::BOT_STAT_MOD_STRENGTH;  break;
        case STAT_AGILITY:   bot_stat = BotStatMods::BOT_STAT_MOD_AGILITY;   break;
        case STAT_STAMINA:   bot_stat = BotStatMods::BOT_STAT_MOD_STAMINA;   break;
        case STAT_INTELLECT: bot_stat = BotStatMods::BOT_STAT_MOD_INTELLECT; break;
        case STAT_SPIRIT:    bot_stat = BotStatMods::BOT_STAT_MOD_SPIRIT;    break;
        default: //should not happen
            bot_stat = BOT_STAT_MOD_HEALTH;
            break;
    }
    return bot_stat;
}

std::string BotMgr::GetTargetIconString(uint8 icon_idx) const
{
    std::ostringstream ss;
    ss << "|TInterface\\TargetingFrame\\UI-RaidTargetingIcon_" << uint32(icon_idx + 1) << ":12|t";
    if (size_t(icon_idx) < TARGET_ICON_NAMES_CACHE_SIZE)
        ss << _targetIconNamesCache[icon_idx];

    return ss.str();
}
void BotMgr::UpdateTargetIconName(uint8 id, std::string_view name)
{
    if (id >= TARGET_ICON_NAMES_CACHE_SIZE)
        return;

    _targetIconNamesCache[id] = name;
}
void BotMgr::ResetTargetIconNames()
{
    _targetIconNamesCache = {};
}

void BotMgr::ReviveAllBots()
{
    for (auto const& [_, bot] : _bots)
        _reviveBot(bot);
}

void BotMgr::SendBotCommandState(uint32 state)
{
    for (auto const& [_, bot] : _bots)
        bot->GetBotAI()->SetBotCommandState(state, true);
}

void BotMgr::SendBotCommandStateRemove(uint32 state)
{
    for (auto const& [_, bot] : _bots)
        bot->GetBotAI()->RemoveBotCommandState(state);
}

void BotMgr::SendBotAwaitState(uint8 state)
{
    for (auto const& [_, bot] : _bots)
        bot->GetBotAI()->SetBotAwaitState(state);
}

void BotMgr::RecallAllBots(bool teleport)
{
    if (teleport)
    {
        _data->SetFlag(NPCBOT_MGR_FLAG_HIDE_BOTS);
        _quickrecall = true;
    }
    else
    {
        for (auto const& [_, bot] : _bots)
            if (bot->IsInWorld() && bot->IsAlive() && !bot_ai::CCed(bot, true))
                bot->GetMotionMaster()->MovePoint(_owner->GetMapId(), *_owner, FORCED_MOVEMENT_NONE, 0.0f, false);
    }
}

void BotMgr::RecallBot(Creature* bot)
{
    ASSERT(GetBot(bot->GetGUID()));

    if (bot->IsInWorld() && bot->IsAlive() && !bot_ai::CCed(bot, true))
        bot->GetMotionMaster()->MovePoint(_owner->GetMapId(), *_owner, FORCED_MOVEMENT_NONE, 0.0f, false);
}

void BotMgr::KillAllBots()
{
    for (auto const& [_, bot] : _bots)
        KillBot(bot);
}

void BotMgr::KillBot(Creature* bot) const
{
    ASSERT(GetBot(bot->GetGUID()));

    if (bot->IsInWorld() && bot->IsAlive())
    {
        bot->setDeathState(DeathState::JustDied);
        bot->GetBotAI()->JustDied(bot);
        //bot->Kill(bot);
    }
}

void BotMgr::SetBotsShouldUpdateStats()
{
    for (auto const& [_, bot] : _bots)
        bot->GetBotAI()->SetShouldUpdateStats();
}

void BotMgr::UpdatePhaseForBots()
{
    for (auto const& [_, bot] : _bots)
    {
        bot->SetPhaseMask(_owner->GetPhaseMask(), bot->IsInWorld());
        if (bot->GetBotsPet())
            bot->GetBotsPet()->SetPhaseMask(_owner->GetPhaseMask(), bot->GetBotsPet()->IsInWorld());
    }
}

void BotMgr::UpdatePvPForBots()
{
    for (auto const& [_, bot] : _bots)
    {
        bot->SetByteValue(UNIT_FIELD_BYTES_2, 1, _owner->GetByteValue(UNIT_FIELD_BYTES_2, 1));
        if (bot->GetBotsPet())
            bot->GetBotsPet()->SetByteValue(UNIT_FIELD_BYTES_2, 1, _owner->GetByteValue(UNIT_FIELD_BYTES_2, 1));
    }
}

void BotMgr::BuildBotPartyMemberStatsPacket(ObjectGuid bot_guid, WorldPacket* data)
{
    Creature const* bot = BotDataMgr::FindBot(bot_guid.GetEntry());
    if (!bot)
    {
        *data << uint8(0);
        *data << bot_guid.WriteAsPacked();
        *data << uint32(GROUP_UPDATE_FLAG_STATUS);
        *data << uint16(MEMBER_STATUS_OFFLINE);
        return;
    }

    Creature const* pet = nullptr; //bot->GetBotAI()->GetBotsPet();
    Powers powerType = bot->GetPowerType();

    *data << uint8(0);                                       // only for SMSG_PARTY_MEMBER_STATS_FULL, probably arena/bg related
    *data << bot->GetPackGUID();

    uint32 updateFlags = GROUP_UPDATE_FLAG_STATUS | GROUP_UPDATE_FLAG_CUR_HP | GROUP_UPDATE_FLAG_MAX_HP
                      | GROUP_UPDATE_FLAG_CUR_POWER | GROUP_UPDATE_FLAG_MAX_POWER | GROUP_UPDATE_FLAG_LEVEL
                      | GROUP_UPDATE_FLAG_ZONE | GROUP_UPDATE_FLAG_POSITION | GROUP_UPDATE_FLAG_AURAS
                      | GROUP_UPDATE_FLAG_PET_NAME | GROUP_UPDATE_FLAG_PET_MODEL_ID | GROUP_UPDATE_FLAG_PET_AURAS;

    if (powerType != POWER_MANA)
        updateFlags |= GROUP_UPDATE_FLAG_POWER_TYPE;

    if (pet)
        updateFlags |= GROUP_UPDATE_FLAG_PET_GUID | GROUP_UPDATE_FLAG_PET_CUR_HP | GROUP_UPDATE_FLAG_PET_MAX_HP
                    | GROUP_UPDATE_FLAG_PET_POWER_TYPE | GROUP_UPDATE_FLAG_PET_CUR_POWER | GROUP_UPDATE_FLAG_PET_MAX_POWER;

    if (bot->GetVehicle())
        updateFlags |= GROUP_UPDATE_FLAG_VEHICLE_SEAT;

    uint16 playerStatus = MEMBER_STATUS_ONLINE;
    if (bot->IsPvP())
        playerStatus |= MEMBER_STATUS_PVP;

    if (!bot->IsAlive())
        playerStatus |= MEMBER_STATUS_DEAD;

    if (bot->IsFFAPvP())
        playerStatus |= MEMBER_STATUS_PVP_FFA;

    *data << uint32(updateFlags);
    *data << uint16(playerStatus);                           // GROUP_UPDATE_FLAG_STATUS
    *data << uint32(bot->GetHealth());                    // GROUP_UPDATE_FLAG_CUR_HP
    *data << uint32(bot->GetMaxHealth());                 // GROUP_UPDATE_FLAG_MAX_HP
    if (updateFlags & GROUP_UPDATE_FLAG_POWER_TYPE)
        *data << uint8(powerType);

    *data << uint16(bot->GetPower(powerType));            // GROUP_UPDATE_FLAG_CUR_POWER
    *data << uint16(bot->GetMaxPower(powerType));         // GROUP_UPDATE_FLAG_MAX_POWER
    *data << uint16(bot->GetLevel());                     // GROUP_UPDATE_FLAG_LEVEL
    *data << uint16(bot->GetZoneId());                    // GROUP_UPDATE_FLAG_ZONE
    *data << uint16(bot->GetPositionX());                 // GROUP_UPDATE_FLAG_POSITION
    *data << uint16(bot->GetPositionY());                 // GROUP_UPDATE_FLAG_POSITION

    uint64 auraMask = 0;
    size_t maskPos = data->wpos();
    *data << uint64(auraMask);                               // placeholder
    for (auto i : NPCBots::index_array<uint8, MAX_AURAS_GROUP_UPDATE>)
    {
        if (AuraApplication const* aurApp = const_cast<Creature*>(bot)->GetVisibleAura(i))
        {
            auraMask |= uint64(1) << i;
            *data << uint32(aurApp->GetBase()->GetId());
            *data << uint8(aurApp->GetFlags());
        }
    }

    data->put<uint64>(maskPos, auraMask);                    // GROUP_UPDATE_FLAG_AURAS

    if (updateFlags & GROUP_UPDATE_FLAG_PET_GUID)
        *data << pet->GetGUID();

    *data << std::string(pet ? pet->GetName() : "");         // GROUP_UPDATE_FLAG_PET_NAME
    *data << uint16(pet ? pet->GetDisplayId() : 0);          // GROUP_UPDATE_FLAG_PET_MODEL_ID

    if (updateFlags & GROUP_UPDATE_FLAG_PET_CUR_HP)
        *data << uint32(pet->GetHealth());

    if (updateFlags & GROUP_UPDATE_FLAG_PET_MAX_HP)
        *data << uint32(pet->GetMaxHealth());

    if (updateFlags & GROUP_UPDATE_FLAG_PET_POWER_TYPE)
        *data << (uint8)pet->GetPowerType();

    if (updateFlags & GROUP_UPDATE_FLAG_PET_CUR_POWER)
        *data << uint16(pet->GetPower(pet->GetPowerType()));

    if (updateFlags & GROUP_UPDATE_FLAG_PET_MAX_POWER)
        *data << uint16(pet->GetMaxPower(pet->GetPowerType()));

    uint64 petAuraMask = 0;
    maskPos = data->wpos();
    *data << uint64(petAuraMask);                            // placeholder
    if (pet)
    {
        for (auto i : NPCBots::index_array<uint8, MAX_AURAS_GROUP_UPDATE>)
        {
            if (AuraApplication const* aurApp = const_cast<Creature*>(pet)->GetVisibleAura(i))
            {
                petAuraMask |= uint64(1) << i;
                *data << uint32(aurApp->GetBase()->GetId());
                *data << uint8(aurApp->GetFlags());
            }
        }
    }

    data->put<uint64>(maskPos, petAuraMask);                 // GROUP_UPDATE_FLAG_PET_AURAS

    if (updateFlags & GROUP_UPDATE_FLAG_VEHICLE_SEAT)
        *data << uint32(bot->GetVehicle()->GetVehicleInfo()->m_seatID[bot->m_movementInfo.transport.seat]);
}

void BotMgr::BuildBotPartyMemberStatsChangedPacket(Creature const* bot, WorldPacket* data)
{
    uint32 mask = bot->GetBotAI()->GetGroupUpdateFlag();

    if (mask == GROUP_UPDATE_FLAG_NONE)
        return;

    if (mask & GROUP_UPDATE_FLAG_POWER_TYPE)                // if update power type, update current/max power also
        mask |= (GROUP_UPDATE_FLAG_CUR_POWER | GROUP_UPDATE_FLAG_MAX_POWER);

    if (mask & GROUP_UPDATE_FLAG_PET_POWER_TYPE)            // same for pets
        mask |= (GROUP_UPDATE_FLAG_PET_CUR_POWER | GROUP_UPDATE_FLAG_PET_MAX_POWER);

    uint32 byteCount = 0;
    uint8 flags_count = GROUP_UPDATE_FLAGS_COUNT;
    for (uint8 i = 1; i < flags_count; ++i)
        if (mask & (1u << i))
            byteCount += GroupUpdateLength[i];

    data->Initialize(SMSG_PARTY_MEMBER_STATS, size_t(8 + 4 + byteCount));
    *data << bot->GetPackGUID();
    *data << uint32(mask);

    if (mask & GROUP_UPDATE_FLAG_STATUS)
    {
        uint16 playerStatus = MEMBER_STATUS_ONLINE;
        if (bot->IsPvP())
            playerStatus |= MEMBER_STATUS_PVP;

        if (!bot->IsAlive())
            playerStatus |= MEMBER_STATUS_DEAD;

        if (bot->IsFFAPvP())
            playerStatus |= MEMBER_STATUS_PVP_FFA;

        *data << uint16(playerStatus);
    }

    if (mask & GROUP_UPDATE_FLAG_CUR_HP)
        *data << uint32(bot->GetHealth());

    if (mask & GROUP_UPDATE_FLAG_MAX_HP)
        *data << uint32(bot->GetMaxHealth());

    Powers powerType = bot->GetPowerType();
    if (mask & GROUP_UPDATE_FLAG_POWER_TYPE)
        *data << uint8(powerType);

    if (mask & GROUP_UPDATE_FLAG_CUR_POWER)
        *data << uint16(bot->GetPower(powerType));

    if (mask & GROUP_UPDATE_FLAG_MAX_POWER)
        *data << uint16(bot->GetMaxPower(powerType));

    if (mask & GROUP_UPDATE_FLAG_LEVEL)
        *data << uint16(bot->GetLevel());

    if (mask & GROUP_UPDATE_FLAG_ZONE)
        *data << uint16(bot->GetZoneId());

    if (mask & GROUP_UPDATE_FLAG_POSITION)
    {
        *data << uint16(bot->GetPositionX());
        *data << uint16(bot->GetPositionY());
    }

    if (mask & GROUP_UPDATE_FLAG_AURAS)
    {
        uint64 auramask = GetBotAuraUpdateMaskForRaid(bot);
        *data << uint64(auramask);
        for (auto i : NPCBots::index_array<uint8, MAX_AURAS_GROUP_UPDATE>)
        {
            if (auramask & (uint64(1) << i))
            {
                AuraApplication const* aurApp = const_cast<Creature*>(bot)->GetVisibleAura(i);
                *data << uint32(aurApp ? aurApp->GetBase()->GetId() : 0);
                *data << uint8(1);
            }
        }
    }

    Creature const* pet = nullptr; //bot->GetBotAI()->GetBotsPet();
    if (mask & GROUP_UPDATE_FLAG_PET_GUID)
    {
        if (pet)
            *data << pet->GetGUID();
        else
            *data << (uint64) 0;
    }

    if (mask & GROUP_UPDATE_FLAG_PET_NAME)
    {
        if (pet)
            *data << pet->GetName();
        else
            *data << uint8(0);
    }

    if (mask & GROUP_UPDATE_FLAG_PET_MODEL_ID)
    {
        if (pet)
            *data << uint16(pet->GetDisplayId());
        else
            *data << uint16(0);
    }

    if (mask & GROUP_UPDATE_FLAG_PET_CUR_HP)
    {
        if (pet)
            *data << uint32(pet->GetHealth());
        else
            *data << uint32(0);
    }

    if (mask & GROUP_UPDATE_FLAG_PET_MAX_HP)
    {
        if (pet)
            *data << uint32(pet->GetMaxHealth());
        else
            *data << uint32(0);
    }

    if (mask & GROUP_UPDATE_FLAG_PET_POWER_TYPE)
    {
        if (pet)
            *data << uint8(pet->GetPowerType());
        else
            *data << uint8(0);
    }

    if (mask & GROUP_UPDATE_FLAG_PET_CUR_POWER)
    {
        if (pet)
            *data << uint16(pet->GetPower(pet->GetPowerType()));
        else
            *data << uint16(0);
    }

    if (mask & GROUP_UPDATE_FLAG_PET_MAX_POWER)
    {
        if (pet)
            *data << uint16(pet->GetMaxPower(pet->GetPowerType()));
        else
            *data << uint16(0);
    }

    if (mask & GROUP_UPDATE_FLAG_PET_AURAS)
    {
        if (pet)
        {
            uint64 auramask = GetBotPetAuraUpdateMaskForRaid(pet);
            *data << uint64(auramask);
            for (auto i : NPCBots::index_array<uint8, MAX_AURAS_GROUP_UPDATE>)
            {
                if (auramask & (uint64(1) << i))
                {
                    AuraApplication const* aurApp = const_cast<Creature*>(pet)->GetVisibleAura(i);
                    *data << uint32(aurApp ? aurApp->GetBase()->GetId() : 0);
                    *data << uint8(aurApp ? aurApp->GetFlags() : 0);
                }
            }
        }
        else
            *data << uint64(0);
    }

    if (mask & GROUP_UPDATE_FLAG_VEHICLE_SEAT)
    {
        if (Vehicle* veh = bot->GetVehicle())
            *data << uint32(veh->GetVehicleInfo()->m_seatID[bot->m_movementInfo.transport.seat]);
        else
            *data << uint32(0);
    }
}

//uint32 BotMgr::GetBotGroupUpdateFlag(Creature const* bot)
//{
//    bot->GetBotAI()->GetGroupUpdateFlags
//}
void BotMgr::SetBotGroupUpdateFlag(Creature const* bot, uint32 flag)
{
    bot->GetBotAI()->SetGroupUpdateFlag(flag);
}
uint64 BotMgr::GetBotAuraUpdateMaskForRaid(Creature const* bot)
{
    return bot->GetBotAI()->GetAuraUpdateMaskForRaid();
}
void BotMgr::SetBotAuraUpdateMaskForRaid(Creature const* bot, uint8 slot)
{
    bot->GetBotAI()->SetAuraUpdateMaskForRaid(slot);
}
void BotMgr::ResetBotAuraUpdateMaskForRaid(Creature const* bot)
{
    bot->GetBotAI()->ResetAuraUpdateMaskForRaid();
}
uint64 BotMgr::GetBotPetAuraUpdateMaskForRaid(Creature const* botpet)
{
    return botpet->GetBotPetAI()->GetAuraUpdateMaskForRaid();
}
void BotMgr::SetBotPetAuraUpdateMaskForRaid(Creature const* botpet, uint8 slot)
{
    botpet->GetBotPetAI()->SetAuraUpdateMaskForRaid(slot);
}
void BotMgr::ResetBotPetAuraUpdateMaskForRaid(Creature const* botpet)
{
    botpet->GetBotPetAI()->ResetAuraUpdateMaskForRaid();
}

uint8 BotMgr::GetBotFollowDist() const
{
    return _data->dist_follow;
}
void BotMgr::SetBotFollowDist(uint8 dist)
{
    _data->dist_follow = dist;
}

void BotMgr::_setBotExactAttackRange(uint8 exactRange)
{
    _data->dist_attack = exactRange;
}

uint8 BotMgr::GetBotExactAttackRange() const
{
    return _data->dist_attack;
}
uint8 BotMgr::GetBotAttackRangeMode() const
{
    return _data->attack_range_mode;
}
void BotMgr::SetBotAttackRangeMode(uint8 mode, uint8 exactRange)
{
    _data->attack_range_mode = mode; _setBotExactAttackRange(exactRange);
}

uint8 BotMgr::GetBotAttackAngleMode() const
{
    return _data->attack_angle_mode;
}
void BotMgr::SetBotAttackAngleMode(uint8 mode)
{
    _data->attack_angle_mode = mode;
}

bool BotMgr::GetBotAllowCombatPositioning() const
{
    return !_data->HasFlag(NPCBOT_MGR_FLAG_DISABLE_COMBAT_POSITIONING);
}
void BotMgr::SetBotAllowCombatPositioning(bool allow)
{
    allow ? _data->RemoveFlag(NPCBOT_MGR_FLAG_DISABLE_COMBAT_POSITIONING) : _data->SetFlag(NPCBOT_MGR_FLAG_DISABLE_COMBAT_POSITIONING);
}

bool BotMgr::GetBotsHidden() const
{
    return _data->HasFlag(NPCBOT_MGR_FLAG_HIDE_BOTS);
}
void BotMgr::SetBotsHidden(bool hidden)
{
    hidden ? _data->SetFlag(NPCBOT_MGR_FLAG_HIDE_BOTS) : _data->RemoveFlag(NPCBOT_MGR_FLAG_HIDE_BOTS);
}

uint32 BotMgr::GetEngageDelayDPS() const
{
    return _data->engage_delay_dps;
}
uint32 BotMgr::GetEngageDelayHeal() const { return _data->engage_delay_heal;
}
void BotMgr::SetEngageDelayDPS(uint32 delay)
{
    _data->engage_delay_dps = delay;
}
void BotMgr::SetEngageDelayHeal(uint32 delay)
{
    _data->engage_delay_heal = delay;
}

void BotMgr::PropagateEngageTimers() const
{
    uint32 delay_dps = GetEngageDelayDPS();
    uint32 delay_heal = GetEngageDelayHeal();

    if (!delay_dps && !delay_heal)
        return;

    for (auto const& [_, bot] : _bots)
    {
        if (bot->GetBotAI()->IsTank())
            continue;

        bool is_heal = bot->GetBotAI()->HasRole(BOT_ROLE_HEAL);
        bool is_dps= bot->GetBotAI()->HasRole(BOT_ROLE_DPS);
        uint32 delay = (is_heal && is_dps) ? std::max<uint32>(delay_dps, delay_heal) : is_heal ? delay_heal : is_dps ? delay_dps : 0;

        bot->GetBotAI()->ResetEngageTimer(delay);
    }
}

void BotMgr::TrackDamage(Unit const* u, uint32 damage)
{
    _dpstracker->TrackDamage(u, damage);
}

uint32 BotMgr::GetDPSTaken(Unit const* u) const
{
    return _dpstracker->GetDPSTaken(u->GetGUID());
}

int32 BotMgr::GetHPSTaken(Unit const* unit) const
{
    if (!HaveBot())
        return 0;

    std::list<Unit*> unitList;
    Group const* gr = _owner->GetGroup();
    if (!gr)
    {
        if (_owner->HasUnitState(UNIT_STATE_CASTING))
            unitList.push_back(_owner);
        for (auto const& [_, bot] : _bots)
            if (bot->GetTarget() == unit->GetGUID() && bot->HasUnitState(UNIT_STATE_CASTING))
                unitList.push_back(bot);
    }
    else
    {
        bool Bots = false;
        for (GroupReference const* itr = gr->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* player = itr->GetSource();
            if (player == nullptr) continue;
            if (_owner->GetMap() != player->FindMap()) continue;
            if (!Bots)
                Bots = true;
            if (player->HasUnitState(UNIT_STATE_CASTING))
                unitList.push_back(player);
        }
        if (Bots)
        {
            for (GroupReference const* gitr = gr->GetFirstMember(); gitr != nullptr; gitr = gitr->next())
            {
                if (gitr->GetSource() == nullptr) continue;
                if (_owner->GetMap() != gitr->GetSource()->FindMap()) continue;

                if (gitr->GetSource()->HaveBot())
                {
                    for (auto const& [_, bot] : *gitr->GetSource()->GetBotMgr()->GetBotMap())
                        if (bot->GetTarget() == unit->GetGUID() && bot->HasUnitState(UNIT_STATE_CASTING))
                            unitList.push_back(bot);
                }
            }
        }
    }

    int32 amount = 0;
    for (Unit* u : unitList)
    {
        for (uint8 i = CURRENT_FIRST_NON_MELEE_SPELL; i != CURRENT_AUTOREPEAT_SPELL; ++i)
        {
            Spell const* spell = u->GetCurrentSpell(CurrentSpellTypes(i));
            if (!spell)
                continue;

            ObjectGuid targetGuid = spell->m_targets.GetObjectTargetGUID();
            if (!targetGuid || !targetGuid.IsUnit())
                continue;

            if (targetGuid != unit->GetGUID())
            {
                if (!gr || !gr->IsMember(unit->GetGUID()))
                    continue;
            }

            SpellInfo const* spellInfo = spell->GetSpellInfo();

            for (auto j : NPCBots::index_array<uint8, MAX_SPELL_EFFECTS>)
            {
                if (spellInfo->Effects[j].Effect != SPELL_EFFECT_HEAL)
                    continue;

                if (targetGuid != unit->GetGUID())
                {
                    if (spellInfo->Effects[j].TargetA.GetSelectionCategory() != TARGET_SELECT_CATEGORY_AREA)
                        continue;

                    //Targets t = spellInfo->Effects[j].TargetA.GetTarget();
                    //non-existing case
                    //if (t == TARGET_UNIT_CASTER_AREA_PARTY && !gr->SameSubGroup(u->GetGUID(), unit->GetGUID()))
                    //    continue;
                    Targets t = spellInfo->Effects[j].TargetB.GetTarget();
                    if (t == TARGET_UNIT_LASTTARGET_AREA_PARTY &&
                        !(GetBot(unit->GetGUID()) && GetBot(targetGuid)) &&
                        !gr->SameSubGroup(unit->GetGUID(), targetGuid))
                        continue;
                }

                int32 healing = u->SpellHealingBonusDone(const_cast<Unit*>(unit), spellInfo, spellInfo->Effects[0].CalcValue(u), HEAL, 0);
                healing = const_cast<Unit*>(unit)->SpellHealingBonusTaken(u, spellInfo, healing, HEAL);

                if (i == CURRENT_CHANNELED_SPELL)
                    amount += int32(healing / (spellInfo->Effects[j].Amplitude * 0.001f));
                else
                    amount += int32(healing / (std::max<int32>(spell->GetTimer(), 1000) * 0.001f));

                //BOT_LOG_ERROR("entities.player", "BotMgr:pendingHeals: found %s's %s on %s in %u (%i, total %i)",
                //    u->GetName().c_str(), spellInfo->SpellName[0], target->GetName().c_str(), pheal->delay, healing, pheal->amount);
            }

            break;
        }
    }

    //HoTs
    for (AuraEffect const* aeff : unit->GetAuraEffectsByType(SPELL_AURA_PERIODIC_HEAL))
        amount += int32(aeff->GetAmount() / (aeff->GetAmplitude() * 0.001f));

    //if (amount != 0)
    //    BOT_LOG_ERROR("entities.player", "BotMgr:GetHPSTaken(): %s got %i)", unit->GetName().c_str(), amount);

    return amount;
}

void BotMgr::OnBotWandererKilled(Creature const* bot, Player* looter)
{
    bot->GetBotAI()->SpawnKillReward(looter);
}

void BotMgr::OnBotWandererKilled(GameObject* go)
{
    if (go->GetEntry() == GO_BOT_MONEY_BAG && go->GetSpellId() > go->GetEntry())
    {
        uint32 bot_id = go->GetSpellId() - GO_BOT_MONEY_BAG;
        if (Creature const* bot = BotDataMgr::FindBot(bot_id))
            bot->GetBotAI()->FillKillReward(go);
    }
}

void BotMgr::OnBotKilled(Creature const* bot, Unit* attacker/* = nullptr*/)
{
    bot->GetBotAI()->OnDeath(attacker);
}

void BotMgr::OnBotSpellInterrupt(Unit const* caster, CurrentSpellTypes spellType)
{
    if (spellType == CURRENT_AUTOREPEAT_SPELL)
    {
        WorldPacket data(SMSG_CANCEL_AUTO_REPEAT, caster->GetPackGUID().size());
        data << caster->GetPackGUID();
        caster->SendMessageToSet(&data, true);
    }
}

void BotMgr::OnBotSpellGo(Unit const* caster, Spell const* spell, bool ok)
{
    if (caster->ToCreature()->GetBotAI())
        caster->ToCreature()->GetBotAI()->OnBotSpellGo(spell, ok);
    else if (caster->ToCreature()->GetBotPetAI())
        caster->ToCreature()->GetBotPetAI()->OnBotPetSpellGo(spell, ok);
}

void BotMgr::OnBotOwnerSpellGo(Unit const* caster, Spell const* spell, bool ok)
{
    for (auto const& [_, bot] : *caster->ToPlayer()->GetBotMgr()->GetBotMap())
    {
        if (!bot || !bot->IsInWorld() || !bot->IsAlive())
            continue;

        bot->GetBotAI()->OnBotOwnerSpellGo(spell, ok);
        //if (Creature const* botpet = bot->GetBotsPet())
        //    botpet->GetBotAI()->OnBotPetOwnerSpellGo(spell, ok);
    }
}

void BotMgr::OnBotChannelFinish(Unit const* caster, Spell const* spell)
{
    if (caster->ToCreature()->GetBotAI())
        caster->ToCreature()->GetBotAI()->OnBotChannelFinish(spell);
    //else if (caster->ToCreature()->GetBotPetAI())
    //    caster->ToCreature()->GetBotPetAI()->OnBotPetChannelFinish(spell);
}

void BotMgr::OnVehicleSpellGo(Unit const* caster, Spell const* spell, bool ok)
{
    if (caster->GetCharmerGUID().IsPlayer())
    {
        Unit const* owner = caster->GetCharmer();
        if (owner && owner->ToPlayer()->HaveBot())
        {
            for (auto const& [_, bot] : *owner->ToPlayer()->GetBotMgr()->GetBotMap())
            {
                if (bot)
                {
                    bot->GetBotAI()->OnBotOwnerSpellGo(spell, ok);
                    //if (Creature const* botpet = bot->GetBotsPet())
                    //    botpet->GetBotAI()->OnBotPetOwnerSpellGo(spell, ok);
                }
            }
        }
    }
    else if (caster->GetCharmerGUID().IsCreature())
    {
        Unit const* bot = caster->GetCharmer();
        if (bot->ToCreature()->GetBotAI())
            bot->ToCreature()->GetBotAI()->OnBotSpellGo(spell, ok);
    }
}

void BotMgr::OnVehicleAttackedBy(Unit* attacker, Unit const* victim)
{
    Unit const* owner = victim->GetCharmer();
    if (victim->GetCharmerGUID().IsPlayer())
        owner = victim->GetCharmer();
    else if (victim->GetCharmerGUID().IsCreature())
        if (Unit const* bot = victim->GetCharmer())
            owner = bot->ToCreature()->GetBotOwner();

    if (owner && owner->IsPlayer() && owner->ToPlayer()->HaveBot())
    {
        for (auto const& [_, bot] : *owner->ToPlayer()->GetBotMgr()->GetBotMap())
            if (bot)
                bot->GetBotAI()->OnOwnerVehicleDamagedBy(attacker);
    }
}

void BotMgr::OnBotDamageTaken(Unit* attacker, Unit* victim, uint32 damage, CleanDamage const* cleanDamage, DamageEffectType damagetype, SpellInfo const* spellInfo)
{
    victim->ToCreature()->GetBotAI()->OnBotDamageTaken(attacker, damage, cleanDamage , damagetype, spellInfo);
}

void BotMgr::OnBotDamageDealt(Unit* attacker, Unit* victim, uint32 damage, CleanDamage const* cleanDamage, DamageEffectType damagetype, SpellInfo const* spellInfo)
{
    attacker->ToCreature()->GetBotAI()->OnBotDamageDealt(victim, damage, cleanDamage, damagetype, spellInfo);
}

void BotMgr::OnBotDispelDealt(Unit* dispeller, Unit* dispelled, uint8 num)
{
    dispeller->ToCreature()->GetBotAI()->OnBotDispelDealt(dispelled, num);
}

void BotMgr::OnBotEnterVehicle(Creature const* passenger, Vehicle const* vehicle)
{
    passenger->GetBotAI()->OnBotEnterVehicle(vehicle);
}

void BotMgr::OnBotExitVehicle(Creature const* passenger, Vehicle const* vehicle)
{
    passenger->GetBotAI()->OnBotExitVehicle(vehicle);
}

void BotMgr::OnBotOwnerEnterVehicle(Player const* passenger, Vehicle const* vehicle)
{
    for (auto const& [_, bot] : *passenger->GetBotMgr()->GetBotMap())
        if (bot && bot->IsInWorld() && bot->IsAlive())
            bot->GetBotAI()->OnBotOwnerEnterVehicle(vehicle);
}

void BotMgr::OnBotOwnerExitVehicle(Player const* passenger, Vehicle const* vehicle)
{
    for (auto const& [_, bot] : *passenger->GetBotMgr()->GetBotMap())
        if (bot && bot->IsInWorld() && bot->IsAlive())
            bot->GetBotAI()->OnBotOwnerExitVehicle(vehicle);
}

void BotMgr::OnBotPartyEngage(Player const* owner)
{
    Group const* gr = owner->GetGroup();
    if (gr)
    {
        std::vector<Player const*> affectedPlayers;
        for (GroupReference const* itr = gr->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player const* player = itr->GetSource();
            if (!player || owner->GetMap() != player->FindMap() ||
                player->GetDistance(owner) > World::GetMaxVisibleDistanceOnContinents() ||
                !player->HaveBot())
                continue;

            if (player->GetBotMgr()->IsPartyInCombat(false))
                return;

            affectedPlayers.push_back(player);
        }
        for (Player const* p : affectedPlayers)
            p->GetBotMgr()->PropagateEngageTimers();
    }
    else
        owner->GetBotMgr()->PropagateEngageTimers();
}

void BotMgr::OnBotAttackStop(Creature const* bot, Unit const* target)
{
    if (bot->IsNPCBot())
        bot->GetBotAI()->OnAttackStop(target);
    else if (bot->IsNPCBotPet())
        bot->GetBotPetAI()->OnAttackStop(target);
}

void BotMgr::ApplyBotEffectMods(Unit const* caster, SpellInfo const* spellInfo, uint8 effIndex, float& value)
{
    caster->ToCreature()->GetBotAI()->ApplyBotEffectMods(spellInfo, effIndex, value);
}

void BotMgr::ApplyBotThreatMods(Unit const* attacker, SpellInfo const* spellInfo, float& threat)
{
    attacker->ToCreature()->GetBotAI()->ApplyBotThreatMods(spellInfo, threat);
}

void BotMgr::ApplyBotEffectValueMultiplierMods(Unit const* caster, SpellInfo const* spellInfo, SpellEffIndex effIndex, float& multiplier)
{
    caster->ToCreature()->GetBotAI()->ApplyBotEffectValueMultiplierMods(spellInfo, effIndex, multiplier);
}

float BotMgr::GetBotDamageTakenMod(Creature const* bot, bool magic)
{
    return bot->GetBotAI()->GetBotDamageTakenMod(magic);
}

int32 BotMgr::GetBotStat(Creature const* bot, BotStatMods stat)
{
    return bot->GetBotAI()->GetTotalBotStat(stat);
}

int32 BotMgr::GetBotStat(Creature const* bot, Stats stat)
{
    return GetBotStat(bot, GetBotStatModByUnitStat(stat));
}

float BotMgr::GetBotResilience(Creature const* botOrPet)
{
    if (botOrPet->IsNPCBot())
        return botOrPet->GetBotAI()->GetBotResilience();

    return botOrPet->GetBotPetAI()->GetPetsOwner()->GetBotAI()->GetBotResilience();
}

std::vector<Unit*> BotMgr::GetAllGroupMembers(Group const* group)
{
    std::vector<Unit*> group_members;
    if (group)
    {
        group_members.reserve(group->GetMembersCount());
        for (GroupReference const* ref = group->GetFirstMember(); ref != nullptr; ref = ref->next())
        {
            if (Player* pl = ref->GetSource())
                group_members.push_back(pl);
        }
        for (GroupBotReference const* ref = group->GetFirstBotMember(); ref != nullptr; ref = ref->next())
        {
            if (Creature* cr = ref->GetSource())
                group_members.push_back(cr);
        }
    }

    return group_members;
}
std::vector<Unit*> BotMgr::GetAllGroupMembers(Unit const* source)
{
    Group const* group = (source->IsNPCBot() && source->ToCreature()->GetBotAI()) ? source->ToCreature()->GetBotAI()->GetGroup() :
        source->IsPlayer() ? source->ToPlayer()->GetGroup() : nullptr;
    return GetAllGroupMembers(group);
}

void BotMgr::InviteBotToBG(ObjectGuid botguid, GroupQueueInfo* ginfo, Battleground* bg)
{
    Creature const* bot = BotDataMgr::FindBot(botguid.GetEntry());
    ASSERT(bot);

    bg->IncreaseInvitedCount(ginfo->teamId);
    //BOT_LOG_INFO("npcbots", "Battleground: invited NPCBot %u to BG instance %u bgtype %u '%s'",
    //    botguid.GetEntry(), bg->GetInstanceID(), bg->GetTypeID(), bg->GetName().c_str());
}

bool BotMgr::IsBotInAreaTriggerRadius(Creature const* bot, AreaTrigger const* trigger)
{
    if (!trigger || !bot->IsInWorld() || bot->GetMap()->GetId() != trigger->map)
        return false;

    if (trigger->radius > 0.f)
    {
        // if we have radius check it
        float dist = bot->GetDistance(trigger->x, trigger->y, trigger->z);
        if (dist > trigger->radius)
            return false;
    }
    else
    {
        Position center(trigger->x, trigger->y, trigger->z, trigger->orientation);
        if (!bot->IsWithinBox(center, trigger->length / 2.f, trigger->width / 2.f, trigger->height / 2.f))
            return false;
    }

    return true;
}

BotMgr::delayed_teleport_mutex_type* BotMgr::_getTpLock()
{
    static BotMgr::delayed_teleport_mutex_type _lock;
    return &_lock;
}
void BotMgr::AddDelayedTeleportCallback(delayed_teleport_callback_type&& callback)
{
    delayed_teleport_lock_type lock(*_getTpLock());
    delayed_bot_teleports.push_back(std::forward<delayed_teleport_callback_type>(callback));
}
void BotMgr::HandleDelayedTeleports()
{
    for (auto& func : delayed_bot_teleports)
        func();
    delayed_bot_teleports.clear();
}

#ifdef _MSC_VER
# pragma warning(pop)
#endif
