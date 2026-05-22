#ifndef BOTCONFIG_H
#define BOTCONFIG_H

#include "botcommon.h"

enum SharedOwnerOptions : uint32
{
    SHARED_OWNER_ENABLE                 = 1,
    SHARED_OWNER_EQUIPMENT              = 2,
    SHARED_OWNER_ADD_OWNERS             = 3,
    SHARED_OWNER_REMOVE_OWNERS          = 4,

    MAX_SHARED_OWNER_OPTIONS
};
enum SharedOwnerOptionMask : uint32
{
    SHARED_OWNER_OPTION_MASK_ENABLE         = (1<<(SHARED_OWNER_ENABLE-1)),
    SHARED_OWNER_OPTION_MASK_EQUIPMENT      = (1<<(SHARED_OWNER_EQUIPMENT-1)),
    SHARED_OWNER_OPTION_MASK_ADD_OWNERS     = (1<<(SHARED_OWNER_ADD_OWNERS-1)),
    SHARED_OWNER_OPTION_MASK_REMOVE_OWNERS  = (1<<(SHARED_OWNER_REMOVE_OWNERS-1)),

    SHARED_OWNER_OPTION_MASK_MANAGE_OWNERS  = SHARED_OWNER_OPTION_MASK_ADD_OWNERS | SHARED_OWNER_OPTION_MASK_REMOVE_OWNERS,
    SHARED_OWNER_OPTION_MASK_ALL            = (1<<(MAX_SHARED_OWNER_OPTIONS-1)) - 1
};

class Map;

template<typename U>
using BotBrackets = std::array<U, BRACKETS_COUNT>;
using LvlBrackets = BotBrackets<uint8>;
using PctBrackets = BotBrackets<uint32>;
using ItemLvlBrackets = BotBrackets<uint32>;

class AC_GAME_API BotCfg
{
public:
    static void ReloadConfig();

    static bool IsNpcBotModEnabled();
    static bool IsNpcBotLogEnabled();
    static bool IsNpcBotDungeonFinderEnabled();
    static bool IsNpcBotDungeonFinderBotGenerationEnabled();
    static bool LimitNpcBotsInDungeons();
    static bool LimitNpcBotsInRaids();
    static bool IsNpcBotsPremadeEnabled();
    static bool DisplayEquipment();
    static bool ShowEquippedCloak();
    static bool ShowEquippedHelm();
    static bool SendEquipListItems();
    static bool IsGearBankEnabled();
    static bool IsTransmogEnabled();
    static bool MixArmorClasses();
    static bool MixWeaponClasses();
    static bool MixWeaponInventoryTypes();
    static bool TransmogUseEquipmentSlots();
    static bool IsClassEnabled(uint8 m_class);
    static bool IsWanderingClassEnabled(uint8 m_class);
    static bool EnableWanderingUntargetNpcQuestgiver();
    static bool EnableWanderingUntargetNpcFlightmaster();
    static bool HideBotSpawns();
    static bool IsEnrageOnDimissEnabled();
    static bool IsBotStatsLimitsEnabled();
    static bool IsPvPEnabled();
    static bool IsFoodInterruptedByMovement();
    static bool FilterRaces();
    static bool IsBotGenerationEnabledBGs();
    static bool IsBotLevelCappedByConfigBG();
    static bool IsBotLevelCappedByConfigBGFirstPlayer();
    static bool IsBotGenerationEnabledWorldMapId(uint32 mapId);
    static bool IsBotHKEnabled();
    static bool IsBotHKMessageEnabled();
    static bool IsBotHKAchievementsEnabled();
    static bool IsSharedOwnerOptionEnabled(SharedOwnerOptionMask options);
    static uint8 GetMaxClassBots();
    static uint8 GetMaxAccountBots();
    static uint8 GetMaxSharedOwners();
    static uint32 GetGearBankCapacity();
    static uint32 GetGearBankEquipmentSetsCount();
    static uint8 GetHealTargetIconFlags();
    static uint8 GetTankTargetIconFlags();
    static uint8 GetOffTankTargetIconFlags();
    static uint8 GetDPSTargetIconFlags();
    static uint8 GetRangedDPSTargetIconFlags();
    static uint8 GetNoDPSTargetIconFlags();
    static uint32 GetBaseUpdateDelay();
    static uint32 GetOwnershipExpireTime();
    static uint8 GetOwnershipExpireMode();
    static uint32 GetDesiredWanderingBotsCount();
    static uint32 GetBGTargetTeamPlayersCount(BattlegroundTypeId bgTypeId);
    static float GetBotHKHonorRate();
    static float GetBotStatLimitDodge();
    static float GetBotStatLimitParry();
    static float GetBotStatLimitBlock();
    static float GetBotStatLimitCrit();
    static float GetBotDamageModPhysical();
    static float GetBotDamageModSpell();
    static float GetBotHealingMod();
    static float GetBotHPMod();
    static float GetBotWandererDamageMod();
    static float GetBotWandererHealingMod();
    static float GetBotWandererHPMod();
    static float GetBotWandererSpeedMod();
    static float GetBotWandererXPGainMod();
    static PctBrackets GetBotWandererLevelBrackets();
    static uint32 GetBotWandererMaxItemLevel(uint8 level);
    static uint32 GetBotWandererKillRewardMoney();
    static uint32 GetBotWandererKillRewardItemMaxCount();
    static uint32 GetBotWandererKillRewardItemMaxQuality();
    static uint32 GetBotDungeonMaxItemLevel(uint8 level, uint16 map_id, Difficulty map_difficulty);
    static float GetBotDamageModByClass(uint8 botclass);
    static float GetBotDamageModByLevel(uint8 botlevel);
    static float GetBotHealingModByLevel(uint8 botlevel);
    static float GetBotHPModByLevel(uint8 botlevel);
    static float GetBotMPModByLevel(uint8 botlevel);

    static uint8 GetFollowDistDefault();
    static uint32 GetEngageDelayDPSDefault();
    static uint32 GetEngageDelayHealDefault();

    static uint8 GetMaxNpcBots(uint8 level);
    static bool IsNpcBotXpReductionEnabled();
    static bool IsNpcBotXpReductionGroupOnly();
    static uint8 GetNpcBotXpReductionExtraAmount();
    static uint8 GetNpcBotXpReductionExtraStartingNumber();
    static bool IsNpcBotHonorReductionEnabled();
    static bool IsNpcBotHonorReductionGroupOnly();
    static bool GetNpcBotMoneyShareEnabled();
    static bool GetNpcBotMoneyShareGroupOnly();
    static uint8 GetNpcBotMountLevel60();
    static uint8 GetNpcBotMountLevel100();
    static int32 GetBotInfoPacketsLimit();

    static uint32 GetNpcBotCostRent(uint8 level, uint8 botclass);
    static uint32 GetNpcBotCostHire(uint8 level, uint8 botclass);
    static std::string GetNpcBotCostStr(uint8 level, uint8 botclass);

    static bool IsMapAllowedForBots(Map const* map);
private:
    static uint32 _normalizedCostForLevel(uint32 cost_base, uint8 bot_class, uint8 level);
};

void AddNpcBotScripts();

#endif
