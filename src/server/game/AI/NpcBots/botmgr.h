#ifndef BOTMGR_H
#define BOTMGR_H

#include "botcommon.h"
#include "DBCEnums.h"

#include <functional>
#include <mutex>
#include <string_view>

class bot_ai;
class Battleground;
class Creature;
class GameObject;
class Group;
class Map;
class Player;
class Spell;
class SpellInfo;
class Unit;
class Vehicle;
class WorldLocation;
class WorldObject;
class WorldPacket;

class DPSTracker;

struct AreaTrigger;
struct CleanDamage;
struct GroupQueueInfo;
struct NpcBotMgrData;
struct Position;

enum BattlegroundTypeId : uint8;
enum CurrentSpellTypes : uint8;
enum DamageEffectType : uint8;

inline constexpr std::size_t TARGET_ICON_NAMES_CACHE_SIZE = 8u; // Group.h TARGETICONCOUNT

enum BotMgrDataFlags : uint32
{
    NPCBOT_MGR_FLAG_HIDE_BOTS                  = 0x00000001,
    NPCBOT_MGR_FLAG_DISABLE_COMBAT_POSITIONING = 0x00000002,

    NPCBOT_MGR_FLAG_MASK_ALL_ALLOWED           = (NPCBOT_MGR_FLAG_HIDE_BOTS | NPCBOT_MGR_FLAG_DISABLE_COMBAT_POSITIONING),
    NPCBOT_MGR_FLAG_MASK_ALL_DB_ALLOWED        = (NPCBOT_MGR_FLAG_DISABLE_COMBAT_POSITIONING)
};

enum BotAddResult
{
    BOT_ADD_DISABLED                    = 0x001,
    BOT_ADD_ALREADY_HAVE                = 0x002,
    BOT_ADD_MAX_EXCEED                  = 0x004,
    BOT_ADD_MAX_CLASS_EXCEED            = 0x008,
    BOT_ADD_CANT_AFFORD                 = 0x010,
    BOT_ADD_INSTANCE_LIMIT              = 0x020,
    BOT_ADD_BUSY                        = 0x040, // unused
    BOT_ADD_NOT_AVAILABLE               = 0x080,

    BOT_ADD_SUCCESS                     = 0x100,

    BOT_ADD_FATAL                       = (BOT_ADD_DISABLED | BOT_ADD_CANT_AFFORD | BOT_ADD_MAX_EXCEED | BOT_ADD_MAX_CLASS_EXCEED)
};

enum BotRemoveType
{
    BOT_REMOVE_LOGOUT                   = 0,
    BOT_REMOVE_DISMISS                  = 1,
    BOT_REMOVE_UNSUMMON                 = 2,
    BOT_REMOVE_UNBIND                   = 3,
    BOT_REMOVE_UNAFFORD                 = 4,
    BOT_REMOVE_BY_DEFAULT               = BOT_REMOVE_LOGOUT
};

enum BotOwnershipExpireMode
{
    BOT_OWNERSHIP_EXPIRE_OFFLINE        = 0,
    BOT_OWNERSHIP_EXPIRE_HIRE           = 1
};

enum BotAttackRange
{
    BOT_ATTACK_RANGE_SHORT              = 1,
    BOT_ATTACK_RANGE_LONG               = 2,
    BOT_ATTACK_RANGE_EXACT              = 3,

    BOT_ATTACK_RANGE_END                = BOT_ATTACK_RANGE_EXACT
};

enum BotAttackAngle
{
    BOT_ATTACK_ANGLE_NORMAL             = 1,
    BOT_ATTACK_ANGLE_AVOID_FRONTAL_AOE  = 2,

    BOT_ATTACK_ANGLE_END                = BOT_ATTACK_ANGLE_AVOID_FRONTAL_AOE
};

using BotMap = std::unordered_map<ObjectGuid /*guid*/, Creature* /*bot*/>;

class AC_GAME_API BotMgr
{
public:
    using delayed_teleport_callback_type = std::function<void(void)>;
    using delayed_teleport_mutex_type = std::mutex;
    using delayed_teleport_lock_type = std::unique_lock<delayed_teleport_mutex_type>;

    explicit BotMgr(Player* const master);
    ~BotMgr();
    BotMgr(BotMgr const&) = delete;
    BotMgr(BotMgr&&) = delete;
    BotMgr& operator=(BotMgr const&) = delete;
    BotMgr& operator=(BotMgr&&) = delete;

    Player* GetOwner() const { return _owner; }

    BotMap const* GetBotMap() const { return &_bots; }
    BotMap* GetBotMap() { return &_bots; }

    static void Initialize();

    //onEvent hooks
    static void OnBotWandererKilled(Creature const* bot, Player* looter);
    static void OnBotWandererKilled(GameObject* go);
    static void OnBotKilled(Creature const* bot, Unit* attacker = nullptr);
    static void OnBotSpellInterrupt(Unit const* caster, CurrentSpellTypes spellType);
    static void OnBotSpellGo(Unit const* caster, Spell const* spell, bool ok = true);
    static void OnBotOwnerSpellGo(Unit const* caster, Spell const* spell, bool ok = true);
    static void OnBotChannelFinish(Unit const* caster, Spell const* spell);
    static void OnVehicleSpellGo(Unit const* caster, Spell const* spell, bool ok = true);
    static void OnVehicleAttackedBy(Unit* attacker, Unit const* victim);
    static void OnBotDamageTaken(Unit* attacker, Unit* victim, uint32 damage, CleanDamage const* cleanDamage, DamageEffectType damagetype, SpellInfo const* spellInfo);
    static void OnBotDamageDealt(Unit* attacker, Unit* victim, uint32 damage, CleanDamage const* cleanDamage, DamageEffectType damagetype, SpellInfo const* spellInfo);
    static void OnBotDispelDealt(Unit* dispeller, Unit* dispelled, uint8 num);
    static void OnBotEnterVehicle(Creature const* passenger, Vehicle const* vehicle);
    static void OnBotExitVehicle(Creature const* passenger, Vehicle const* vehicle);
    static void OnBotOwnerEnterVehicle(Player const* passenger, Vehicle const* vehicle);
    static void OnBotOwnerExitVehicle(Player const* passenger, Vehicle const* vehicle);
    static void OnBotPartyEngage(Player const* owner);
    static void OnBotAttackStop(Creature const* bot, Unit const* target);
    //mod hooks
    static void ApplyBotEffectMods(Unit const* caster, SpellInfo const* spellInfo, uint8 effIndex, float& value);
    static void ApplyBotThreatMods(Unit const* attacker, SpellInfo const* spellInfo, float& threat);
    static void ApplyBotEffectValueMultiplierMods(Unit const* caster, SpellInfo const* spellInfo, SpellEffIndex effIndex, float& multiplier);
    static float GetBotDamageTakenMod(Creature const* bot, bool magic);
    static int32 GetBotStat(Creature const* bot, BotStatMods stat);
    static int32 GetBotStat(Creature const* bot, Stats stat);
    static float GetBotResilience(Creature const* botOrPet);

    void LoadData();

    void Update(uint32 diff);

    Creature* GetBot(ObjectGuid guid) const;
    Creature* GetBotByName(std::string_view name) const;
    std::vector<Creature*> GetAllBotsByClass(uint8 botclass) const;

    bool HaveBot() const { return !_bots.empty(); }
    uint8 GetNpcBotsCount() const;
    uint8 GetNpcBotsCountByRole(uint32 roles) const;
    uint8 GetNpcBotsCountByVehicleEntry(uint32 creEntry) const;
    uint8 GetNpcBotSlot(Creature const* bot) const;
    uint8 GetNpcBotSlotByRole(uint32 roles, Creature const* bot) const;
    uint32 GetAllNpcBotsClassMask() const;
    static bool LimitBots(Map const* map);
    static bool CanBotParryWhileCasting(Creature const* bot);
    static bool IsWanderingWorldBot(Creature const* bot);
    static bool IsBotContestedPvP(Creature const* bot);
    static void SetBotContestedPvP(Creature const* bot);
    bool RestrictBots(Creature const* bot, bool add) const;
    bool IsPartyInCombat(bool is_pvp) const;
    bool HasBotClass(uint8 botclass) const;
    bool HasBotWithSpec(uint8 spec, bool alive = true) const;
    bool HasBotPetType(uint32 petType) const;
    bool IsBeingResurrected(WorldObject const* corpse) const;

    static uint8 BotClassByClassName(std::string_view className);
    static uint8 GetBotPlayerClass(uint8 bot_class);
    static uint8 GetBotPlayerRace(uint8 bot_class, uint8 bot_race);
    static uint8 GetBotPlayerClass(Creature const* bot);
    static uint8 GetBotPlayerRace(Creature const* bot);
    static uint8 GetBotEquipmentClass(uint8 bot_class);
    static BotStatMods GetBotStatModByUnitStat(Stats stat);

    std::string GetTargetIconString(uint8 icon_idx) const;

    void OnTeleportFar(uint32 mapId, float x, float y, float z, float ori = 0.f);
    void OnOwnerSetGameMaster(bool on);
    void ReviveAllBots();
    void SendBotCommandState(uint32 state);
    void SendBotCommandStateRemove(uint32 state);
    void SendBotAwaitState(uint8 state);
    void RecallAllBots(bool teleport = false);
    void RecallBot(Creature* bot);
    void KillAllBots();
    void KillBot(Creature* bot) const;

    void CleanupsBeforeBotDelete(ObjectGuid guid, uint8 removetype = BOT_REMOVE_LOGOUT);
    static void CleanupsBeforeBotDelete(Creature* bot);
    void RemoveAllSummonedBots();
    void RemoveAllBots(uint8 removetype = BOT_REMOVE_LOGOUT);
    void RemoveBot(ObjectGuid guid, uint8 removetype = BOT_REMOVE_LOGOUT);
    void UnbindBot(ObjectGuid guid);
    [[nodiscard]] BotAddResult RebindBot(Creature* bot);
    [[nodiscard]] BotAddResult AddDungeonBot(Creature* bot);
    [[nodiscard]] BotAddResult AddBot(Creature* bot);
    bool AddBotToGroup(Creature* bot);
    void RemoveBotFromBGQueue(Creature const* bot);
    bool RemoveBotFromGroup(Creature* bot);
    bool RemoveAllBotsFromGroup();

    static uint8 GetBotFollowDistMax() { return 100; }
    uint8 GetBotFollowDist() const;
    void SetBotFollowDist(uint8 dist);

    uint8 GetBotExactAttackRange() const;
    uint8 GetBotAttackRangeMode() const;
    void SetBotAttackRangeMode(uint8 mode, uint8 exactRange = 0);

    uint8 GetBotAttackAngleMode() const;
    void SetBotAttackAngleMode(uint8 mode);

    bool GetBotAllowCombatPositioning() const;
    void SetBotAllowCombatPositioning(bool allow);

    bool GetBotsHidden() const;
    void SetBotsHidden(bool hidden);

    uint32 GetEngageDelayDPS() const;
    uint32 GetEngageDelayHeal() const;
    void SetEngageDelayDPS(uint32 delay);
    void SetEngageDelayHeal(uint32 delay);
    void PropagateEngageTimers() const;

    void SetBotsShouldUpdateStats();
    void UpdatePhaseForBots();
    void UpdatePvPForBots();

    static void BuildBotPartyMemberStatsPacket(ObjectGuid bot_guid, WorldPacket* data);
    static void BuildBotPartyMemberStatsChangedPacket(Creature const* bot, WorldPacket* data);
    //static uint32 GetBotGroupUpdateFlag(Creature const* bot);
    static void SetBotGroupUpdateFlag(Creature const* bot, uint32 flag);
    static uint64 GetBotAuraUpdateMaskForRaid(Creature const* bot);
    static void SetBotAuraUpdateMaskForRaid(Creature const* bot, uint8 slot);
    static void ResetBotAuraUpdateMaskForRaid(Creature const* bot);
    static uint64 GetBotPetAuraUpdateMaskForRaid(Creature const* botpet);
    static void SetBotPetAuraUpdateMaskForRaid(Creature const* botpet, uint8 slot);
    static void ResetBotPetAuraUpdateMaskForRaid(Creature const* botpet);

    void TrackDamage(Unit const* u, uint32 damage);
    uint32 GetDPSTaken(Unit const* u) const;
    int32 GetHPSTaken(Unit const* unit) const;

    static void ReviveBot(Creature* bot, WorldLocation* dest = nullptr) { _reviveBot(bot, dest); }

    //TELEPORT BETWEEN MAPS
    //CONFIRMEND UNSAFE (charmer,owner)
    static void TeleportBot(Creature* bot, Map* newMap, Position const* pos, bool quick = false, bool reset = false, bot_ai* detached_ai = nullptr);

    AoeSpotsVec const& GetAoeSpots() const { return _aoespots; }
    AoeSpotsVec& GetAoeSpots() { return _aoespots; }

    void UpdateTargetIconName(uint8 id, std::string_view name);
    void ResetTargetIconNames();

    static std::vector<Unit*> GetAllGroupMembers(Group const* group);
    static std::vector<Unit*> GetAllGroupMembers(Unit const* source);
    static void InviteBotToBG(ObjectGuid botguid, GroupQueueInfo* ginfo, Battleground* bg);

    static bool IsBotInAreaTriggerRadius(Creature const* bot, AreaTrigger const* trigger);

    static void AddDelayedTeleportCallback(delayed_teleport_callback_type&& callback);
    static void HandleDelayedTeleports();

private:
    static void _teleportBot(Creature* bot, Map* newMap, float x, float y, float z, float ori, bool quick, bool reset, bot_ai* detached_ai);
    static void _reviveBot(Creature* bot, WorldLocation* dest = nullptr);
    void _setBotExactAttackRange(uint8 exactRange);
    static delayed_teleport_mutex_type* _getTpLock();

    Player* const _owner;
    BotMap _bots;
    std::list<std::pair<ObjectGuid, BotRemoveType>> _delayedRemoveList;
    DPSTracker* const _dpstracker;
    NpcBotMgrData* _data;

    bool _quickrecall;
    bool _update_lock;

    AoeSpotsVec _aoespots;

    std::array<std::string_view, TARGET_ICON_NAMES_CACHE_SIZE> _targetIconNamesCache;
};

#endif
