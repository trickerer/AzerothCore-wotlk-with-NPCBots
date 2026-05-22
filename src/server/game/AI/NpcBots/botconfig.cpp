#include "botcommon.h"
#include "botconfig.h"
#include "botdatamgr.h"
#include "botlog.h"
#include "Config.h"
#include "DBCStores.h"
#include "Group.h"
#include "Log.h"
#include "MapMgr.h"
#include "ScriptMgr.h"
#include "Tokenize.h"
#include "World.h"

/*
Npc Bot Config by Trickerer (onlysuffering@gmail.com)
*/

#ifdef _MSC_VER
# pragma warning(push, 4)
#endif

#ifdef AC_COMPILER
# define GetBoolDefault GetOption<bool>
# define GetIntDefault GetOption<int32>
# define GetFloatDefault GetOption<float>
# define GetStringDefault GetOption<std::string>
#endif

static uint8 _basefollowdist;
static uint8 _maxClassNpcBots;
static uint8 _maxAccountNpcBots;
static uint8 _maxSharedOwners;
static uint8 _xpReductionExtraAmount;
static uint8 _xpReductionExtraStartingNumber;
static uint8 _mountLevel60;
static uint8 _mountLevel100;
static uint8 _healTargetIconFlags;
static uint8 _tankingTargetIconFlags;
static uint8 _offTankingTargetIconFlags;
static uint8 _dpsTargetIconFlags;
static uint8 _rangedDpsTargetIconFlags;
static uint8 _noDpsTargetIconFlags;
static uint8 _npcBotOwnerExpireMode;
static int32 _botInfoPacketsLimit;
static uint32 _gearBankCapacity;
static uint32 _gearBankEquipmentSetsCount;
static uint32 _npcBotsCostHire;
static uint32 _npcBotsCostRent;
static uint32 _npcBotUpdateDelayBase;
static uint32 _npcBotEngageDelayDPS_default;
static uint32 _npcBotEngageDelayHeal_default;
static uint32 _npcBotOwnerExpireTime;
static uint32 _desiredWanderingBotsCount;
static uint32 _killrewardWandererMoneyBase;
static uint32 _killrewardWandererItemCount;
static uint32 _killrewardWandererItemQuality;
static uint32 _targetBGPlayersPerTeamCount_AV;
static uint32 _targetBGPlayersPerTeamCount_WS;
static uint32 _targetBGPlayersPerTeamCount_AB;
static uint32 _targetBGPlayersPerTeamCount_EY;
static uint32 _targetBGPlayersPerTeamCount_SA;
static uint32 _targetBGPlayersPerTeamCount_IC;
static uint32 _shared_ownership_options;
static bool _enableNpcBots;
static bool _logToDB;
static bool _xpReductionEnable;
static bool _xpReductionGroupOnly;
static bool _honorReductionEnable;
static bool _honorReductionGroupOnly;
static bool _moneyLootShareEnable;
static bool _moneyLootShareGroupOnly;
static bool _enableNpcBotsDungeons;
static bool _enableNpcBotsRaids;
static bool _enableNpcBotsBGs;
static bool _enableNpcBotsArenas;
static bool _enableDungeonFinder;
static bool _enableDungeonFinderBotsGen;
static bool _enableNpcBotsPremade;
static bool _limitNpcBotsDungeons;
static bool _limitNpcBotsRaids;
static bool _hideSpawns;
/*static*/bool _botPvP;
static bool _botMovementFoodInterrupt;
static bool _filterRaces;
static bool _displayEquipment;
static bool _showCloak;
static bool _showHelm;
static bool _sendEquipListItems;
static bool _enableBotGearBank;
static bool _transmog_enable;
static bool _transmog_mixArmorClasses;
static bool _transmog_mixWeaponClasses;
static bool _transmog_mixWeaponInvTypes;
static bool _transmog_useEquipmentSlots;
static bool _enableclass_warrior;
static bool _enableclass_paladin;
static bool _enableclass_hunter;
static bool _enableclass_rogue;
static bool _enableclass_priest;
static bool _enableclass_deathknight;
static bool _enableclass_shaman;
static bool _enableclass_mage;
static bool _enableclass_warlock;
static bool _enableclass_druid;
static bool _enableclass_blademaster;
static bool _enableclass_sphynx;
static bool _enableclass_archmage;
static bool _enableclass_dreadlord;
static bool _enableclass_spellbreaker;
static bool _enableclass_darkranger;
static bool _enableclass_necromancer;
static bool _enableclass_seawitch;
static bool _enableclass_cryptlord;
static bool _enableclass_wander_warrior;
static bool _enableclass_wander_paladin;
static bool _enableclass_wander_hunter;
static bool _enableclass_wander_rogue;
static bool _enableclass_wander_priest;
static bool _enableclass_wander_deathknight;
static bool _enableclass_wander_shaman;
static bool _enableclass_wander_mage;
static bool _enableclass_wander_warlock;
static bool _enableclass_wander_druid;
static bool _enableclass_wander_blademaster;
static bool _enableclass_wander_sphynx;
static bool _enableclass_wander_archmage;
static bool _enableclass_wander_dreadlord;
static bool _enableclass_wander_spellbreaker;
static bool _enableclass_wander_darkranger;
static bool _enableclass_wander_necromancer;
static bool _enableclass_wander_seawitch;
static bool _enableclass_wander_cryptlord;
static bool _enrageOnDismiss;
static bool _botStatLimits;
static bool _enableWanderingBotsBG;
static bool _enableConfigLevelCapBG;
static bool _enableConfigLevelCapBGFirst;
static bool _bothk_enable;
static bool _bothk_message_enable;
static bool _bothk_achievements_enable;
static bool _untarget_wnpc_questgiver;
static bool _untarget_wnpc_flightmaster;
static float _botStatLimits_dodge;
static float _botStatLimits_parry;
static float _botStatLimits_block;
static float _botStatLimits_crit;
static float _mult_dmg_physical;
static float _mult_dmg_spell;
static float _mult_healing;
static float _mult_hp;
static float _mult_dmg_wanderer;
static float _mult_healing_wanderer;
static float _mult_hp_wanderer;
static float _mult_speed_wanderer;
static float _mult_xpgain_wanderer;
static float _mult_dmg_warrior;
static float _mult_dmg_paladin;
static float _mult_dmg_hunter;
static float _mult_dmg_rogue;
static float _mult_dmg_priest;
static float _mult_dmg_deathknight;
static float _mult_dmg_shaman;
static float _mult_dmg_mage;
static float _mult_dmg_warlock;
static float _mult_dmg_druid;
static float _mult_dmg_blademaster;
static float _mult_dmg_obsidiandestroyer;
static float _mult_dmg_archmage;
static float _mult_dmg_dreadlord;
static float _mult_dmg_spellbreaker;
static float _mult_dmg_darkranger;
static float _mult_dmg_necromancer;
static float _mult_dmg_seawitch;
static float _mult_dmg_cryptlord;
static float _bothk_rate_honor;
static std::vector<float> _mult_dmg_levels;
static std::vector<float> _mult_heal_levels;
static std::vector<float> _mult_hp_levels;
static std::vector<float> _mult_mp_levels;
static LvlBrackets _max_npcbots;
static PctBrackets _botwanderer_pct_level_brackets;
static ItemLvlBrackets _botwanderer_itemlvl_level_brackets;
static PctBrackets _botdungeon_itemlvl_brackets;
static std::vector<uint32> _disabled_instance_maps;
static std::vector<uint32> _enabled_wander_node_maps;

struct BotMapItemLevelRange { uint16 map_id{}, normal{}, heroic{}; };
static constinit const std::array MAP_MAX_ITEM_LEVELS {
    BotMapItemLevelRange{ .map_id = 574, .normal = 155, .heroic = 200 }, // UK
    BotMapItemLevelRange{ .map_id = 575, .normal = 187, .heroic = 200 }, // UP
    BotMapItemLevelRange{ .map_id = 576, .normal = 159, .heroic = 200 }, // NE
    BotMapItemLevelRange{ .map_id = 578, .normal = 187, .heroic = 200 }, // OC
    BotMapItemLevelRange{ .map_id = 595, .normal = 187, .heroic = 200 }, // CoS
    BotMapItemLevelRange{ .map_id = 599, .normal = 183, .heroic = 200 }, // HoS
    BotMapItemLevelRange{ .map_id = 600, .normal = 171, .heroic = 200 }, // DT
    BotMapItemLevelRange{ .map_id = 601, .normal = 163, .heroic = 200 }, // AN
    BotMapItemLevelRange{ .map_id = 602, .normal = 187, .heroic = 200 }, // HoL
    BotMapItemLevelRange{ .map_id = 604, .normal = 179, .heroic = 200 }, // GD
    BotMapItemLevelRange{ .map_id = 608, .normal = 175, .heroic = 200 }, // VH
    BotMapItemLevelRange{ .map_id = 619, .normal = 167, .heroic = 200 }, // AK
    BotMapItemLevelRange{ .map_id = 632, .normal = 219, .heroic = 232 }, // FoS
    BotMapItemLevelRange{ .map_id = 650, .normal = 200, .heroic = 219 }, // TC5
    BotMapItemLevelRange{ .map_id = 658, .normal = 219, .heroic = 232 }, // PoS
    BotMapItemLevelRange{ .map_id = 668, .normal = 219, .heroic = 232 }, // HoR
};

void AddSC_botconfig_scripts();
void AddSC_death_knight_bot();
void AddSC_druid_bot();
void AddSC_hunter_bot();
void AddSC_mage_bot();
void AddSC_paladin_bot();
void AddSC_priest_bot();
void AddSC_rogue_bot();
void AddSC_shaman_bot();
void AddSC_warlock_bot();
void AddSC_warrior_bot();
void AddSC_blademaster_bot();
void AddSC_sphynx_bot();
void AddSC_archmage_bot();
void AddSC_dreadlord_bot();
void AddSC_spellbreaker_bot();
void AddSC_dark_ranger_bot();
void AddSC_necromancer_bot();
void AddSC_sea_witch_bot();
void AddSC_crypt_lord_bot();
void AddSC_archmage_bot_pets();
void AddSC_dreadlord_bot_pets();
void AddSC_dark_ranger_bot_pets();
void AddSC_necromancer_bot_pets();
void AddSC_sea_witch_bot_pets();
void AddSC_crypt_lord_bot_pets();
void AddSC_hunter_bot_pets();
void AddSC_warlock_bot_pets();
void AddSC_deathknight_bot_pets();
void AddSC_priest_bot_pets();
void AddSC_shaman_bot_pets();
void AddSC_mage_bot_pets();
void AddSC_druid_bot_pets();
void AddSC_script_bot_commands();
void AddSC_script_bot_giver();
void AddSC_botdatamgr_scripts();

void AddNpcBotScripts()
{
    AddSC_botconfig_scripts();
    AddSC_death_knight_bot();
    AddSC_druid_bot();
    AddSC_hunter_bot();
    AddSC_mage_bot();
    AddSC_paladin_bot();
    AddSC_priest_bot();
    AddSC_rogue_bot();
    AddSC_shaman_bot();
    AddSC_warlock_bot();
    AddSC_warrior_bot();
    AddSC_blademaster_bot();
    AddSC_sphynx_bot();
    AddSC_archmage_bot();
    AddSC_dreadlord_bot();
    AddSC_spellbreaker_bot();
    AddSC_dark_ranger_bot();
    AddSC_necromancer_bot();
    AddSC_sea_witch_bot();
    AddSC_crypt_lord_bot();
    AddSC_archmage_bot_pets();
    AddSC_dreadlord_bot_pets();
    AddSC_dark_ranger_bot_pets();
    AddSC_necromancer_bot_pets();
    AddSC_sea_witch_bot_pets();
    AddSC_crypt_lord_bot_pets();
    AddSC_hunter_bot_pets();
    AddSC_warlock_bot_pets();
    AddSC_deathknight_bot_pets();
    AddSC_priest_bot_pets();
    AddSC_shaman_bot_pets();
    AddSC_mage_bot_pets();
    AddSC_druid_bot_pets();
    AddSC_script_bot_commands();
    AddSC_script_bot_giver();
    AddSC_botdatamgr_scripts();
}

class NPCBotsConfigScript : public WorldScript
{
public:
    NPCBotsConfigScript() : WorldScript("NPCBotsConfigScript") { }

    void OnAfterConfigLoad(bool reload) override
    {
        if (!_first_load)
            _initNpcbotsConfig(reload);
    }

    static void ReloadConfig()
    {
        if (_first_load)
        {
            _initNpcbotsConfig(false);
            _first_load = false;
            return;
        }

        BOT_LOG_INFO("misc", "Re-Loading config settings...");
        sWorld->LoadConfigSettings(true);
        sMapMgr->InitializeVisibilityDistanceInfo();
        BOT_LOG_INFO("misc", "World config settings reloaded.");
    }

private:
    inline static bool _first_load{ true };

    static void _initNpcbotsConfig(bool reload)
    {
        _loadConfig(reload);

        BOT_LOG_INFO("server.loading", ">> NPCBots config {}.", reload ? "re-loaded" : "loaded");

        if (_enableNpcBots)
            BOT_LOG_INFO("server.loading", ">> NPCBots system enabled");
    }

    static void _loadConfig(bool reload)
    {
        _enableNpcBots                  = sConfigMgr->GetBoolDefault("NpcBot.Enable", true);
        _logToDB                        = sConfigMgr->GetBoolDefault("NpcBot.LogToDB", true);
        _maxClassNpcBots                = sConfigMgr->GetIntDefault("NpcBot.MaxBotsPerClass", 1);
        _maxAccountNpcBots              = sConfigMgr->GetIntDefault("NpcBot.MaxBotsPerAccount", 0);
        _filterRaces                    = sConfigMgr->GetBoolDefault("NpcBot.Botgiver.FilterRaces", false);
        _shared_ownership_options       = sConfigMgr->GetIntDefault("NpcBot.SharedOwnership.Options", 0);
        _maxSharedOwners                = sConfigMgr->GetIntDefault("NpcBot.SharedOwnership.MaxOwners", 0);
        _basefollowdist                 = sConfigMgr->GetIntDefault("NpcBot.BaseFollowDistance", 30);
        _xpReductionEnable              = sConfigMgr->GetBoolDefault("NpcBot.XpReduction.Enable", false);
        _xpReductionGroupOnly           = sConfigMgr->GetBoolDefault("NpcBot.XpReduction.GroupOnly", false);
        _xpReductionExtraAmount         = sConfigMgr->GetIntDefault("NpcBot.XpReduction.Extra.Amount", 0);
        _xpReductionExtraStartingNumber = sConfigMgr->GetIntDefault("NpcBot.XpReduction.Extra.StartingNumber", 2);
        _honorReductionEnable           = sConfigMgr->GetBoolDefault("NpcBot.HonorReduction.Enable", false);
        _honorReductionGroupOnly        = sConfigMgr->GetBoolDefault("NpcBot.HonorReduction.GroupOnly", false);
        _moneyLootShareEnable           = sConfigMgr->GetBoolDefault("NpcBot.MoneyShare.Enable", false);
        _moneyLootShareGroupOnly        = sConfigMgr->GetBoolDefault("NpcBot.MoneyShare.GroupOnly", false);
        _mountLevel60                   = sConfigMgr->GetIntDefault("NpcBot.MountLevel.60", 20);
        _mountLevel100                  = sConfigMgr->GetIntDefault("NpcBot.MountLevel.100", 40);
        _healTargetIconFlags            = sConfigMgr->GetIntDefault("NpcBot.HealTargetIconMask", 0);
        _tankingTargetIconFlags         = sConfigMgr->GetIntDefault("NpcBot.TankTargetIconMask", 0);
        _offTankingTargetIconFlags      = sConfigMgr->GetIntDefault("NpcBot.OffTankTargetIconMask", 0);
        _dpsTargetIconFlags             = sConfigMgr->GetIntDefault("NpcBot.DPSTargetIconMask", 0);
        _rangedDpsTargetIconFlags       = sConfigMgr->GetIntDefault("NpcBot.RangedDPSTargetIconMask", 0);
        _noDpsTargetIconFlags           = sConfigMgr->GetIntDefault("NpcBot.NoDPSTargetIconMask", 0);
        _mult_dmg_physical              = sConfigMgr->GetFloatDefault("NpcBot.Mult.Damage.Physical", 1.0f);
        _mult_dmg_spell                 = sConfigMgr->GetFloatDefault("NpcBot.Mult.Damage.Spell", 1.0f);
        _mult_healing                   = sConfigMgr->GetFloatDefault("NpcBot.Mult.Healing", 1.0f);
        _mult_hp                        = sConfigMgr->GetFloatDefault("NpcBot.Mult.HP", 1.0f);
        _mult_dmg_wanderer              = sConfigMgr->GetFloatDefault("NpcBot.Mult.Wanderer.Damage", 1.0f);
        _mult_healing_wanderer          = sConfigMgr->GetFloatDefault("NpcBot.Mult.Wanderer.Healing", 1.0f);
        _mult_hp_wanderer               = sConfigMgr->GetFloatDefault("NpcBot.Mult.Wanderer.HP", 1.0f);
        _mult_speed_wanderer            = sConfigMgr->GetFloatDefault("NpcBot.Mult.Wanderer.Speed", 1.0f);
        _mult_dmg_warrior               = sConfigMgr->GetFloatDefault("NpcBot.Mult.Damage.Warrior", 1.0f);
        _mult_dmg_paladin               = sConfigMgr->GetFloatDefault("NpcBot.Mult.Damage.Paladin", 1.0f);
        _mult_dmg_hunter                = sConfigMgr->GetFloatDefault("NpcBot.Mult.Damage.Hunter", 1.0f);
        _mult_dmg_rogue                 = sConfigMgr->GetFloatDefault("NpcBot.Mult.Damage.Rogue", 1.0f);
        _mult_dmg_priest                = sConfigMgr->GetFloatDefault("NpcBot.Mult.Damage.Priest", 1.0f);
        _mult_dmg_deathknight           = sConfigMgr->GetFloatDefault("NpcBot.Mult.Damage.DeathKnight", 1.0f);
        _mult_dmg_shaman                = sConfigMgr->GetFloatDefault("NpcBot.Mult.Damage.Shaman", 1.0f);
        _mult_dmg_mage                  = sConfigMgr->GetFloatDefault("NpcBot.Mult.Damage.Mage", 1.0f);
        _mult_dmg_warlock               = sConfigMgr->GetFloatDefault("NpcBot.Mult.Damage.Warlock", 1.0f);
        _mult_dmg_druid                 = sConfigMgr->GetFloatDefault("NpcBot.Mult.Damage.Druid", 1.0f);
        _mult_dmg_blademaster           = sConfigMgr->GetFloatDefault("NpcBot.Mult.Damage.Blademaster", 1.0f);
        _mult_dmg_obsidiandestroyer     = sConfigMgr->GetFloatDefault("NpcBot.Mult.Damage.ObsidianDestroyer", 1.0f);
        _mult_dmg_archmage              = sConfigMgr->GetFloatDefault("NpcBot.Mult.Damage.Archmage", 1.0f);
        _mult_dmg_dreadlord             = sConfigMgr->GetFloatDefault("NpcBot.Mult.Damage.Dreadlord", 1.0f);
        _mult_dmg_spellbreaker          = sConfigMgr->GetFloatDefault("NpcBot.Mult.Damage.SpellBreaker", 1.0f);
        _mult_dmg_darkranger            = sConfigMgr->GetFloatDefault("NpcBot.Mult.Damage.DarkRanger", 1.0f);
        _mult_dmg_necromancer           = sConfigMgr->GetFloatDefault("NpcBot.Mult.Damage.Necromancer", 1.0f);
        _mult_dmg_seawitch              = sConfigMgr->GetFloatDefault("NpcBot.Mult.Damage.SeaWitch", 1.0f);
        _mult_dmg_cryptlord             = sConfigMgr->GetFloatDefault("NpcBot.Mult.Damage.CryptLord", 1.0f);
        _enableNpcBotsDungeons          = sConfigMgr->GetBoolDefault("NpcBot.Enable.Dungeon", true);
        _enableNpcBotsRaids             = sConfigMgr->GetBoolDefault("NpcBot.Enable.Raid", false);
        _enableNpcBotsBGs               = sConfigMgr->GetBoolDefault("NpcBot.Enable.BG", false);
        _enableNpcBotsArenas            = sConfigMgr->GetBoolDefault("NpcBot.Enable.Arena", false);
        _enableDungeonFinder            = sConfigMgr->GetBoolDefault("NpcBot.Enable.DungeonFinder", true);
        _enableDungeonFinderBotsGen     = sConfigMgr->GetBoolDefault("NpcBot.DungeonBots.Enable", false);
        _enableNpcBotsPremade           = sConfigMgr->GetBoolDefault("NpcBot.Premade.Enable", false);
        _limitNpcBotsDungeons           = sConfigMgr->GetBoolDefault("NpcBot.Limit.Dungeon", true);
        _limitNpcBotsRaids              = sConfigMgr->GetBoolDefault("NpcBot.Limit.Raid", true);
        _hideSpawns                     = sConfigMgr->GetBoolDefault("NpcBot.HideSpawns", false);
        _botInfoPacketsLimit            = sConfigMgr->GetIntDefault("NpcBot.InfoPacketsLimit", -1);
        _npcBotsCostHire                = sConfigMgr->GetIntDefault("NpcBot.Cost.Hire", 1000000);
        _npcBotsCostRent                = sConfigMgr->GetIntDefault("NpcBot.Cost.Rent", 0);
        _npcBotUpdateDelayBase          = sConfigMgr->GetIntDefault("NpcBot.UpdateDelay.Base", 0);
        _npcBotEngageDelayDPS_default   = sConfigMgr->GetIntDefault("NpcBot.EngageDelay.DPS", 0);
        _npcBotEngageDelayHeal_default  = sConfigMgr->GetIntDefault("NpcBot.EngageDelay.Heal", 0);
        _npcBotOwnerExpireTime          = sConfigMgr->GetIntDefault("NpcBot.OwnershipExpireTime", 0);
        _npcBotOwnerExpireMode          = sConfigMgr->GetIntDefault("NpcBot.OwnershipExpireMode", 0);
        _botPvP                         = sConfigMgr->GetBoolDefault("NpcBot.PvP", true);
        _botMovementFoodInterrupt       = sConfigMgr->GetBoolDefault("NpcBot.Movements.InterruptFood", false);
        _displayEquipment               = sConfigMgr->GetBoolDefault("NpcBot.EquipmentDisplay.Enable", true);
        _showCloak                      = sConfigMgr->GetBoolDefault("NpcBot.EquipmentDisplay.ShowCloak", true);
        _showHelm                       = sConfigMgr->GetBoolDefault("NpcBot.EquipmentDisplay.ShowHelm", false);
        _sendEquipListItems             = sConfigMgr->GetBoolDefault("NpcBot.Gossip.ShowEquipmentListItems", false);
        _enableBotGearBank              = sConfigMgr->GetBoolDefault("NpcBot.GearBank.Enable", true);
        _gearBankCapacity               = sConfigMgr->GetIntDefault("NpcBot.GearBank.Capacity", 40);
        _gearBankEquipmentSetsCount     = sConfigMgr->GetIntDefault("NpcBot.GearBank.EquipmentSets", 0);
        _transmog_enable                = sConfigMgr->GetBoolDefault("NpcBot.Transmog.Enable", false);
        _transmog_mixArmorClasses       = sConfigMgr->GetBoolDefault("NpcBot.Transmog.MixArmorClasses", false);
        _transmog_mixWeaponClasses      = sConfigMgr->GetBoolDefault("NpcBot.Transmog.MixWeaponClasses", false);
        _transmog_mixWeaponInvTypes     = sConfigMgr->GetBoolDefault("NpcBot.Transmog.MixWeaponInventoryTypes", false);
        _transmog_useEquipmentSlots     = sConfigMgr->GetBoolDefault("NpcBot.Transmog.UseEquipmentSlots", false);
        _enableclass_warrior            = sConfigMgr->GetBoolDefault("NpcBot.Classes.Warrior.Enable", true);
        _enableclass_paladin            = sConfigMgr->GetBoolDefault("NpcBot.Classes.Paladin.Enable", true);
        _enableclass_hunter             = sConfigMgr->GetBoolDefault("NpcBot.Classes.Hunter.Enable", true);
        _enableclass_rogue              = sConfigMgr->GetBoolDefault("NpcBot.Classes.Rogue.Enable", true);
        _enableclass_priest             = sConfigMgr->GetBoolDefault("NpcBot.Classes.Priest.Enable", true);
        _enableclass_deathknight        = sConfigMgr->GetBoolDefault("NpcBot.Classes.DeathKnight.Enable", true);
        _enableclass_shaman             = sConfigMgr->GetBoolDefault("NpcBot.Classes.Shaman.Enable", true);
        _enableclass_mage               = sConfigMgr->GetBoolDefault("NpcBot.Classes.Mage.Enable", true);
        _enableclass_warlock            = sConfigMgr->GetBoolDefault("NpcBot.Classes.Warlock.Enable", true);
        _enableclass_druid              = sConfigMgr->GetBoolDefault("NpcBot.Classes.Druid.Enable", true);
        _enableclass_blademaster        = sConfigMgr->GetBoolDefault("NpcBot.Classes.Blademaster.Enable", false);
        _enableclass_sphynx             = sConfigMgr->GetBoolDefault("NpcBot.Classes.ObsidianDestroyer.Enable", true);
        _enableclass_archmage           = sConfigMgr->GetBoolDefault("NpcBot.Classes.Archmage.Enable", true);
        _enableclass_dreadlord          = sConfigMgr->GetBoolDefault("NpcBot.Classes.Dreadlord.Enable", true);
        _enableclass_spellbreaker       = sConfigMgr->GetBoolDefault("NpcBot.Classes.SpellBreaker.Enable", true);
        _enableclass_darkranger         = sConfigMgr->GetBoolDefault("NpcBot.Classes.DarkRanger.Enable", true);
        _enableclass_necromancer        = sConfigMgr->GetBoolDefault("NpcBot.Classes.Necromancer.Enable", true);
        _enableclass_seawitch           = sConfigMgr->GetBoolDefault("NpcBot.Classes.SeaWitch.Enable", true);
        _enableclass_cryptlord          = sConfigMgr->GetBoolDefault("NpcBot.Classes.CryptLord.Enable", true);
        _enableclass_wander_warrior     = sConfigMgr->GetBoolDefault("NpcBot.WanderingBots.Classes.Warrior.Enable", true);
        _enableclass_wander_paladin     = sConfigMgr->GetBoolDefault("NpcBot.WanderingBots.Classes.Paladin.Enable", true);
        _enableclass_wander_hunter      = sConfigMgr->GetBoolDefault("NpcBot.WanderingBots.Classes.Hunter.Enable", true);
        _enableclass_wander_rogue       = sConfigMgr->GetBoolDefault("NpcBot.WanderingBots.Classes.Rogue.Enable", true);
        _enableclass_wander_priest      = sConfigMgr->GetBoolDefault("NpcBot.WanderingBots.Classes.Priest.Enable", true);
        _enableclass_wander_deathknight = sConfigMgr->GetBoolDefault("NpcBot.WanderingBots.Classes.DeathKnight.Enable", true);
        _enableclass_wander_shaman      = sConfigMgr->GetBoolDefault("NpcBot.WanderingBots.Classes.Shaman.Enable", true);
        _enableclass_wander_mage        = sConfigMgr->GetBoolDefault("NpcBot.WanderingBots.Classes.Mage.Enable", true);
        _enableclass_wander_warlock     = sConfigMgr->GetBoolDefault("NpcBot.WanderingBots.Classes.Warlock.Enable", true);
        _enableclass_wander_druid       = sConfigMgr->GetBoolDefault("NpcBot.WanderingBots.Classes.Druid.Enable", true);
        _enableclass_wander_blademaster = sConfigMgr->GetBoolDefault("NpcBot.WanderingBots.Classes.Blademaster.Enable", false);
        _enableclass_wander_sphynx      = sConfigMgr->GetBoolDefault("NpcBot.WanderingBots.Classes.ObsidianDestroyer.Enable", true);
        _enableclass_wander_archmage    = sConfigMgr->GetBoolDefault("NpcBot.WanderingBots.Classes.Archmage.Enable", true);
        _enableclass_wander_dreadlord   = sConfigMgr->GetBoolDefault("NpcBot.WanderingBots.Classes.Dreadlord.Enable", true);
        _enableclass_wander_spellbreaker= sConfigMgr->GetBoolDefault("NpcBot.WanderingBots.Classes.SpellBreaker.Enable", true);
        _enableclass_wander_darkranger  = sConfigMgr->GetBoolDefault("NpcBot.WanderingBots.Classes.DarkRanger.Enable", true);
        _enableclass_wander_necromancer = sConfigMgr->GetBoolDefault("NpcBot.WanderingBots.Classes.Necromancer.Enable", true);
        _enableclass_wander_seawitch    = sConfigMgr->GetBoolDefault("NpcBot.WanderingBots.Classes.SeaWitch.Enable", true);
        _enableclass_wander_cryptlord   = sConfigMgr->GetBoolDefault("NpcBot.WanderingBots.Classes.CryptLord.Enable", true);
        _untarget_wnpc_questgiver       = sConfigMgr->GetBoolDefault("NpcBot.WanderingBots.SkipTarget.Questgiver", false);
        _untarget_wnpc_flightmaster     = sConfigMgr->GetBoolDefault("NpcBot.WanderingBots.SkipTarget.Flightmaster", false);
        _enrageOnDismiss                = sConfigMgr->GetBoolDefault("NpcBot.EnrageOnDismiss", true);
        _botStatLimits                  = sConfigMgr->GetBoolDefault("NpcBot.Stats.Limits.Enable", false);
        _botStatLimits_dodge            = sConfigMgr->GetFloatDefault("NpcBot.Stats.Limits.Dodge", 95.0f);
        _botStatLimits_parry            = sConfigMgr->GetFloatDefault("NpcBot.Stats.Limits.Parry", 95.0f);
        _botStatLimits_block            = sConfigMgr->GetFloatDefault("NpcBot.Stats.Limits.Block", 95.0f);
        _botStatLimits_crit             = sConfigMgr->GetFloatDefault("NpcBot.Stats.Limits.Crit", 95.0f);
        _desiredWanderingBotsCount      = sConfigMgr->GetIntDefault("NpcBot.WanderingBots.Continents.Count", 0);
        _killrewardWandererMoneyBase    = sConfigMgr->GetIntDefault("NpcBot.WanderingBots.KillReward.Money", 0);
        _killrewardWandererItemCount    = sConfigMgr->GetIntDefault("NpcBot.WanderingBots.KillReward.ItemCount", 0);
        _killrewardWandererItemQuality  = sConfigMgr->GetIntDefault("NpcBot.WanderingBots.KillReward.ItemQuality", int(ITEM_QUALITY_RARE));
        _mult_xpgain_wanderer           = sConfigMgr->GetFloatDefault("NpcBot.WanderingBots.Continents.XPGain", 1.0f);
        _enableWanderingBotsBG          = sConfigMgr->GetBoolDefault("NpcBot.WanderingBots.BG.Enable", false);
        _enableConfigLevelCapBG         = sConfigMgr->GetBoolDefault("NpcBot.WanderingBots.BG.CapLevel", false);
        _enableConfigLevelCapBGFirst    = sConfigMgr->GetBoolDefault("NpcBot.WanderingBots.BG.CapLevelByFirstPlayer", false);
        _targetBGPlayersPerTeamCount_AV = sConfigMgr->GetIntDefault("NpcBot.WanderingBots.BG.TargetTeamPlayersCount.AV", 30);
        _targetBGPlayersPerTeamCount_WS = sConfigMgr->GetIntDefault("NpcBot.WanderingBots.BG.TargetTeamPlayersCount.WS", 8);
        _targetBGPlayersPerTeamCount_AB = sConfigMgr->GetIntDefault("NpcBot.WanderingBots.BG.TargetTeamPlayersCount.AB", 12);
        _targetBGPlayersPerTeamCount_EY = sConfigMgr->GetIntDefault("NpcBot.WanderingBots.BG.TargetTeamPlayersCount.EY", 12);
        _targetBGPlayersPerTeamCount_SA = sConfigMgr->GetIntDefault("NpcBot.WanderingBots.BG.TargetTeamPlayersCount.SA", 0);
        _targetBGPlayersPerTeamCount_IC = sConfigMgr->GetIntDefault("NpcBot.WanderingBots.BG.TargetTeamPlayersCount.IC", 0);
        _bothk_enable                   = sConfigMgr->GetBoolDefault("NpcBot.HK.Enable", true);
        _bothk_message_enable           = sConfigMgr->GetBoolDefault("NpcBot.HK.Message.Enable", false);
        _bothk_achievements_enable      = sConfigMgr->GetBoolDefault("NpcBot.HK.Achievements.Enable", false);
        _bothk_rate_honor               = sConfigMgr->GetFloatDefault("NpcBot.HK.Rate.Honor", 1.0);

        if (reload)
            BotLogger::Log(NPCBOT_LOG_CONFIG_RELOAD, uint32(0));

        _max_npcbots = {};
        std::string max_npcbots_by_levels = sConfigMgr->GetStringDefault("NpcBot.MaxBots", "1,1,1,1,1,1,1,1,1");
        std::vector<std::string_view> toks0 = Bcore::Tokenize(max_npcbots_by_levels, ',', false);
        ASSERT(toks0.size() == BRACKETS_COUNT, "NpcBot.MaxBots must have exactly %u values", uint32(BRACKETS_COUNT));
        for (std::size_t i{}; i != toks0.size(); ++i)
        {
            Optional<uint8> val = Bcore::StringTo<uint8>(toks0[i]);
            if (val == std::nullopt)
                BOT_LOG_ERROR("server.loading", "NpcBot.MaxBots contains invalid uint8 value '{}', set to default", toks0[i]);
            uint8 uval = val.value_or(uint8(0));
            if (i > 0)
            {
                uint8 prev = _max_npcbots[i - 1];
                if (prev > uval)
                {
                    BOT_LOG_WARN("server.loading", "NpcBot.MaxBots value at offset {} is {} which is lower than previous value {}!", uint32(i), uint32(uval), uint32(prev));
                    //uval = prev;
                }
                if (uval >= MAXRAIDSIZE)
                {
                    BOT_LOG_ERROR("server.loading", "NpcBot.MaxBots value at offset {} is {} > 39, enforcing max value!", uint32(i), uint32(uval));
                    uval = uint8(MAXRAIDSIZE - 1);
                }
            }
            _max_npcbots[i] = uval;
        }

        _mult_dmg_levels.clear();
        std::string mult_dps_by_levels = sConfigMgr->GetStringDefault("NpcBot.Mult.Damage.Levels", "1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0");
        std::vector<std::string_view> toks1 = Bcore::Tokenize(mult_dps_by_levels, ',', false);
        ASSERT(toks1.size() >= BRACKETS_COUNT, "NpcBot.Mult.Damage.Levels must have at least %u values", uint32(BRACKETS_COUNT));
        for (std::size_t i{}; i != toks1.size(); ++i)
        {
            Optional<float> val = Bcore::StringTo<float>(toks1[i]);
            if (val == std::nullopt)
                BOT_LOG_ERROR("server.loading", "NpcBot.Mult.Damage.Levels contains invalid float value '{}', set to default", toks1[i]);
            float fval = val.value_or(1.0f);
            RoundToInterval(fval, 0.1f, 10.f);
            _mult_dmg_levels.push_back(fval);
        }

        _mult_heal_levels.clear();
        std::string mult_healing_by_levels = sConfigMgr->GetStringDefault("NpcBot.Mult.Healing.Levels", "1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0");
        std::vector<std::string_view> toks5 = Bcore::Tokenize(mult_healing_by_levels, ',', false);
        ASSERT(toks5.size() >= BRACKETS_COUNT, "NpcBot.Mult.Healing.Levels must have at least %u values", uint32(BRACKETS_COUNT));
        for (std::size_t i{}; i != toks5.size(); ++i)
        {
            Optional<float> val = Bcore::StringTo<float>(toks5[i]);
            if (val == std::nullopt)
                BOT_LOG_ERROR("server.loading", "NpcBot.Mult.Healing.Levels contains invalid float value '{}', set to default", toks5[i]);
            float fval = val.value_or(1.0f);
            RoundToInterval(fval, 0.1f, 10.f);
            _mult_heal_levels.push_back(fval);
        }

        _mult_hp_levels.clear();
        std::string mult_hp_by_levels = sConfigMgr->GetStringDefault("NpcBot.Mult.HP.Levels", "1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0");
        std::vector<std::string_view> toks6 = Bcore::Tokenize(mult_hp_by_levels, ',', false);
        ASSERT(toks6.size() >= BRACKETS_COUNT, "NpcBot.Mult.HP.Levels must have at least %u values", uint32(BRACKETS_COUNT));
        for (std::size_t i{}; i != toks6.size(); ++i)
        {
            Optional<float> val = Bcore::StringTo<float>(toks6[i]);
            if (val == std::nullopt)
                BOT_LOG_ERROR("server.loading", "NpcBot.Mult.HP.Levels contains invalid float value '{}', set to default", toks6[i]);
            float fval = val.value_or(1.0f);
            RoundToInterval(fval, 0.1f, 10.f);
            _mult_hp_levels.push_back(fval);
        }

        _mult_mp_levels.clear();
        std::string mult_mp_by_levels = sConfigMgr->GetStringDefault("NpcBot.Mult.MP.Levels", "1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0");
        std::vector<std::string_view> toks7 = Bcore::Tokenize(mult_mp_by_levels, ',', false);
        ASSERT(toks7.size() >= BRACKETS_COUNT, "NpcBot.Mult.MP.Levels must have at least %u values", uint32(BRACKETS_COUNT));
        for (std::size_t i{}; i != toks7.size(); ++i)
        {
            Optional<float> val = Bcore::StringTo<float>(toks7[i]);
            if (val == std::nullopt)
                BOT_LOG_ERROR("server.loading", "NpcBot.Mult.MP.Levels contains invalid float value '{}', set to default", toks7[i]);
            float fval = val.value_or(1.0f);
            RoundToInterval(fval, 0.1f, 10.f);
            _mult_mp_levels.push_back(fval);
        }

        _botwanderer_pct_level_brackets = {};
        std::string wanderers_by_levels = sConfigMgr->GetStringDefault("NpcBot.WanderingBots.Continents.Levels", "20,15,15,10,10,15,15,0,0");
        std::vector<std::string_view> toks2 = Bcore::Tokenize(wanderers_by_levels, ',', false);
        ASSERT(toks2.size() == BRACKETS_COUNT, "NpcBot.WanderingBots.Continents.Levels must have exactly %u values", uint32(BRACKETS_COUNT));
        uint32 total_pct = 0;
        for (std::size_t i{}; i != toks2.size(); ++i)
        {
            Optional<uint32> val = Bcore::StringTo<uint32>(toks2[i]);
            if (val == std::nullopt)
                BOT_LOG_ERROR("server.loading", "NpcBot.WanderingBots.Continents.Levels contains invalid uint32 value '{}', set to default", toks2[i]);
            uint32 uval = val.value_or(uint32(0));
            total_pct += uval;
            _botwanderer_pct_level_brackets[i] = uval;
        }
        ASSERT(total_pct == 100u, "NpcBot.WanderingBots.Continents.Levels sum of values must be exactly 100!");

        _enabled_wander_node_maps.clear();
        std::string enabled_wander_node_maps = sConfigMgr->GetStringDefault("NpcBot.WanderingBots.Continents.Maps", "0,1,530,571");
        std::vector<std::string_view> toks3 = Bcore::Tokenize(enabled_wander_node_maps, ',', false);
        for (std::size_t i{}; i != toks3.size(); ++i)
        {
            Optional<uint32> val = Bcore::StringTo<uint32>(toks3[i]);
            if (val == std::nullopt)
            {
                BOT_LOG_ERROR("server.loading", "NpcBot.WanderingBots.Continents.Maps contains invalid uint32 value '{}', skipped", toks3[i]);
                continue;
            }
            uint32 uval = val.value_or(uint32(0));
            MapEntry const* mapEntry = sMapStore.LookupEntry(uval);
            if (!mapEntry || !mapEntry->IsContinent())
            {
                BOT_LOG_ERROR("server.loading", "NpcBot.WanderingBots.Continents.Maps contains invalid continent map id '{}', skipped", uval);
                continue;
            }
            _enabled_wander_node_maps.push_back(uval);
        }
        if (_enabled_wander_node_maps.empty())
        {
            BOT_LOG_ERROR("server.loading", "NpcBot.WanderingBots.Continents.Maps does not provide any valid maps! Wandering bots will not be spawned!");
            _desiredWanderingBotsCount = 0;
        }

        _disabled_instance_maps.clear();
        std::string disabled_instance_maps = sConfigMgr->GetStringDefault("NpcBot.DisableInstances", "");
        std::vector<std::string_view> toks4 = Bcore::Tokenize(disabled_instance_maps, ',', false);
        for (std::size_t i{}; i != toks4.size(); ++i)
        {
            Optional<uint32> val = Bcore::StringTo<uint32>(toks4[i]);
            if (val == std::nullopt)
            {
                BOT_LOG_ERROR("server.loading", "NpcBot.DisableInstances contains invalid uint32 value '{}', skipped", toks4[i]);
                continue;
            }
            uint32 uval = val.value_or(uint32(0));
            MapEntry const* mapEntry = sMapStore.LookupEntry(uval);
            if (!mapEntry || !mapEntry->IsDungeon())
            {
                BOT_LOG_ERROR("server.loading", "NpcBot.DisableInstances contains invalid instance map id '{}', skipped", uval);
                continue;
            }
            _disabled_instance_maps.push_back(uval);
        }

        _botwanderer_itemlvl_level_brackets = {};
        std::string itemlevel_by_levels = sConfigMgr->GetStringDefault("NpcBot.WanderingBots.MaxItemLevel.Levels", "0,0,0,0,0,0,0,0,0");
        std::vector<std::string_view> tok8 = Bcore::Tokenize(itemlevel_by_levels, ',', false);
        ASSERT(tok8.size() == BRACKETS_COUNT, "NpcBot.WanderingBots.MaxItemLevel.Levels must have exactly %u values", uint32(BRACKETS_COUNT));
        for (std::size_t i{}; i != tok8.size(); ++i)
        {
            Optional<uint32> val = Bcore::StringTo<uint32>(tok8[i]);
            if (val == std::nullopt)
                BOT_LOG_ERROR("server.loading", "NpcBot.WanderingBots.MaxItemLevel.Levels contains invalid uint32 value '{}', set to default", tok8[i]);
            uint32 uval = val.value_or(uint32(0));
            _botwanderer_itemlvl_level_brackets[i] = uval;
        }

        _botdungeon_itemlvl_brackets = {};
        std::string itemlevel_ratio_by_levels = sConfigMgr->GetStringDefault("NpcBot.DungeonBots.MaxItemLevel.Ratio", "0,0,0,0,0,0,0,0,0");
        std::vector<std::string_view> tok9 = Bcore::Tokenize(itemlevel_ratio_by_levels, ',', false);
        ASSERT(tok9.size() == BRACKETS_COUNT, "NpcBot.DungeonBots.MaxItemLevel.Ratio must have exactly %u values", uint32(BRACKETS_COUNT));
        for (std::size_t i{}; i != tok9.size(); ++i)
        {
            Optional<uint32> val = Bcore::StringTo<uint32>(tok9[i]);
            if (val == std::nullopt)
                BOT_LOG_ERROR("server.loading", "NpcBot.DungeonBots.MaxItemLevel.Ratio contains invalid float value '{}', set to default", tok9[i]);
            uint32 uval = val.value_or(uint32(0));
            _botdungeon_itemlvl_brackets[i] = uval;
        }

        //limits
        _mountLevel100 = std::max<uint8>(_mountLevel100, _mountLevel60);
        RoundToInterval(_mult_dmg_physical, 0.1f, 10.f);
        RoundToInterval(_mult_dmg_spell, 0.1f, 10.f);
        RoundToInterval(_mult_healing, 0.1f, 10.f);
        RoundToInterval(_mult_hp, 0.1f, 10.f);
        RoundToInterval(_mult_dmg_wanderer, 0.1f, 10.f);
        RoundToInterval(_mult_healing_wanderer, 0.1f, 10.f);
        RoundToInterval(_mult_hp_wanderer, 0.1f, 10.f);
        RoundToInterval(_mult_speed_wanderer, 0.1f, 10.f);
        RoundToInterval(_mult_xpgain_wanderer, 0.0f, 100.f);
        RoundToInterval(_mult_dmg_warrior, 0.1f, 10.f);
        RoundToInterval(_mult_dmg_paladin, 0.1f, 10.f);
        RoundToInterval(_mult_dmg_hunter, 0.1f, 10.f);
        RoundToInterval(_mult_dmg_rogue, 0.1f, 10.f);
        RoundToInterval(_mult_dmg_priest, 0.1f, 10.f);
        RoundToInterval(_mult_dmg_deathknight, 0.1f, 10.f);
        RoundToInterval(_mult_dmg_shaman, 0.1f, 10.f);
        RoundToInterval(_mult_dmg_mage, 0.1f, 10.f);
        RoundToInterval(_mult_dmg_warlock, 0.1f, 10.f);
        RoundToInterval(_mult_dmg_druid, 0.1f, 10.f);
        RoundToInterval(_mult_dmg_blademaster, 0.1f, 10.f);
        RoundToInterval(_mult_dmg_obsidiandestroyer, 0.1f, 10.f);
        RoundToInterval(_mult_dmg_archmage, 0.1f, 10.f);
        RoundToInterval(_mult_dmg_dreadlord, 0.1f, 10.f);
        RoundToInterval(_mult_dmg_spellbreaker, 0.1f, 10.f);
        RoundToInterval(_mult_dmg_darkranger, 0.1f, 10.f);
        RoundToInterval(_mult_dmg_necromancer, 0.1f, 10.f);
        RoundToInterval(_mult_dmg_seawitch, 0.1f, 10.f);
        RoundToInterval(_mult_dmg_cryptlord, 0.1f, 10.f);
        RoundToInterval(_bothk_rate_honor, 0.1f, 10.f);
        RoundToInterval(_killrewardWandererItemCount, uint32(0), uint32(MAX_NR_LOOT_ITEMS));
        RoundToInterval(_killrewardWandererItemQuality, uint32(ITEM_QUALITY_POOR), uint32(ITEM_QUALITY_HEIRLOOM));
        RoundToInterval(_maxSharedOwners, uint8(0), uint8(MAXRAIDSIZE - 1));

        if ((_shared_ownership_options | SHARED_OWNER_OPTION_MASK_ALL) != SHARED_OWNER_OPTION_MASK_ALL)
        {
            BOT_LOG_ERROR("server.loading", "NpcBot.SharedOwnershipOptions contains unknown values outside of full mask {}! Disabled.", SHARED_OWNER_OPTION_MASK_ALL);
            _shared_ownership_options = 0;
        }

        _resolveConfigConflicts();
    }

    static void _resolveConfigConflicts()
    {
        if (_gearBankEquipmentSetsCount > MAX_BOT_EQUIPMENT_SETS)
        {
            BOT_LOG_ERROR("server.loading", "NpcBot.GearBank.EquipmentSets can't be greater than {}, reduced (was {})!", uint32(MAX_BOT_EQUIPMENT_SETS), _gearBankEquipmentSetsCount);
            _gearBankEquipmentSetsCount = MAX_BOT_EQUIPMENT_SETS;
        }

        uint8 dpsFlags = /*_tankingTargetIconFlags | _offTankingTargetIconFlags | */_dpsTargetIconFlags | _rangedDpsTargetIconFlags;
        if (uint8 interFlags = (_noDpsTargetIconFlags & dpsFlags))
        {
            _noDpsTargetIconFlags &= ~interFlags;
            BOT_LOG_ERROR("server.loading", "NpcBot.NoDPSTargetIconMask intersects with dps targets flags {:#X}! Removed, new mask: {:#X}",
                uint32(interFlags), uint32(_noDpsTargetIconFlags));
        }

        if (!_enabled_wander_node_maps.empty())
        {
            uint8 minbotlevel = DEFAULT_MAX_LEVEL;
            uint8 maxbotlevel = 0;
            for (uint32 mapid : _enabled_wander_node_maps)
            {
                minbotlevel = std::min<uint8>(minbotlevel, BotDataMgr::GetMinLevelForMapId(mapid));
                maxbotlevel = std::max<uint8>(maxbotlevel, BotDataMgr::GetMaxLevelForMapId(mapid));
            }
            for (int8 j = minbotlevel / 10 - 1; j >= 0; --j)
            {
                if (_botwanderer_pct_level_brackets[j] > 0)
                {
                    uint32 pct = _botwanderer_pct_level_brackets[j];
                    _botwanderer_pct_level_brackets[minbotlevel / 10] += pct;
                    _botwanderer_pct_level_brackets[j] = 0;
                    BOT_LOG_WARN("server.loading", "NpcBot.WanderingBots.Continents.Levels conflicts with NpcBot.WanderingBots.Continents.Maps: no map for levels {}-{}! Transferring extra {}% to levels {}-{}",
                        uint32(j ? j * 10 : 1), uint32(j * 10 + 9), pct, std::max<uint32>(minbotlevel / 10 * 10, 1), uint32(minbotlevel / 10 * 10 + 9));
                }
            }
            for (std::size_t i = std::size_t(maxbotlevel) / 10 + 1; i < _botwanderer_pct_level_brackets.size(); ++i)
            {
                if (_botwanderer_pct_level_brackets[i] > 0)
                {
                    uint32 pct = _botwanderer_pct_level_brackets[i];
                    _botwanderer_pct_level_brackets[maxbotlevel / 10] += pct;
                    _botwanderer_pct_level_brackets[i] = 0;
                    BOT_LOG_WARN("server.loading", "NpcBot.WanderingBots.Continents.Levels conflicts with NpcBot.WanderingBots.Continents.Maps: no map for levels {}-{}! Transferring extra {}% to levels {}-{}",
                        uint32(i ? i * 10 : 1), uint32(i * 10 + 9), pct, std::max<uint32>(maxbotlevel, 1), uint32(maxbotlevel + 9));
                }
            }
        }
    }
};

void BotCfg::ReloadConfig()
{
    NPCBotsConfigScript::ReloadConfig();
}

bool BotCfg::IsNpcBotModEnabled()
{
    return _enableNpcBots;
}

bool BotCfg::IsNpcBotLogEnabled()
{
    return _logToDB;
}

bool BotCfg::IsNpcBotDungeonFinderEnabled()
{
    return _enableDungeonFinder;
}

bool BotCfg::IsNpcBotDungeonFinderBotGenerationEnabled()
{
    return _enableDungeonFinderBotsGen;
}

bool BotCfg::LimitNpcBotsInDungeons()
{
    return _limitNpcBotsDungeons;
}
bool BotCfg::LimitNpcBotsInRaids()
{
    return _limitNpcBotsRaids;
}

bool BotCfg::IsNpcBotsPremadeEnabled()
{
    return _enableNpcBotsPremade;
}

uint32 BotCfg::GetNpcBotCostRent(uint8 level, uint8 botclass)
{
    return _normalizedCostForLevel(_npcBotsCostRent, botclass, level);
}

uint32 BotCfg::GetNpcBotCostHire(uint8 level, uint8 botclass)
{
    return _normalizedCostForLevel(_npcBotsCostHire, botclass, level);
}

std::string BotCfg::GetNpcBotCostStr(uint8 level, uint8 botclass)
{
    std::ostringstream money;

    if (uint32 cost = GetNpcBotCostHire(level, botclass))
    {
        uint32 gold = uint32(cost / GOLD);
        cost -= (gold * GOLD);
        uint32 silver = uint32(cost / SILVER);
        cost -= (silver * SILVER);

        if (gold != 0)
            money << gold << " |TInterface\\Icons\\INV_Misc_Coin_01:8|t";
        if (silver != 0)
            money << silver << " |TInterface\\Icons\\INV_Misc_Coin_03:8|t";
        if (cost)
            money << cost << " |TInterface\\Icons\\INV_Misc_Coin_05:8|t";
    }

    if (uint32 rcost = GetNpcBotCostRent(level, botclass))
    {
        uint32 gold = uint32(rcost / GOLD);
        rcost -= (gold * GOLD);
        uint32 silver = uint32(rcost / SILVER);
        rcost -= (silver * SILVER);

        money << " + |TInterface\\Icons\\INV_Misc_PocketWatch_01:16|t";

        if (gold != 0)
            money << gold << " |TInterface\\Icons\\INV_Misc_Coin_01:8|t";
        if (silver != 0)
            money << silver << " |TInterface\\Icons\\INV_Misc_Coin_03:8|t";
        if (rcost)
            money << rcost << " |TInterface\\Icons\\INV_Misc_Coin_05:8|t";
    }

    return money.str();
}

bool BotCfg::DisplayEquipment()
{
    return _displayEquipment;
}

bool BotCfg::ShowEquippedCloak()
{
    return _showCloak;
}

bool BotCfg::ShowEquippedHelm()
{
    return _showHelm;
}

bool BotCfg::SendEquipListItems()
{
    return _sendEquipListItems;
}

bool BotCfg::IsGearBankEnabled()
{
    return _enableBotGearBank;
}

bool BotCfg::IsTransmogEnabled()
{
    return _transmog_enable;
}
bool BotCfg::MixArmorClasses()
{
    return _transmog_mixArmorClasses;
}
bool BotCfg::MixWeaponClasses()
{
    return _transmog_mixWeaponClasses;
}
bool BotCfg::MixWeaponInventoryTypes()
{
    return _transmog_mixWeaponInvTypes;
}
bool BotCfg::TransmogUseEquipmentSlots()
{
    return _transmog_useEquipmentSlots;
}

bool BotCfg::IsClassEnabled(uint8 m_class)
{
    switch (m_class)
    {
        case BOT_CLASS_WARRIOR:
            return _enableclass_warrior;
        case BOT_CLASS_PALADIN:
            return _enableclass_paladin;
        case BOT_CLASS_HUNTER:
            return _enableclass_hunter;
        case BOT_CLASS_ROGUE:
            return _enableclass_rogue;
        case BOT_CLASS_PRIEST:
            return _enableclass_priest;
        case BOT_CLASS_DEATH_KNIGHT:
            return _enableclass_deathknight;
        case BOT_CLASS_SHAMAN:
            return _enableclass_shaman;
        case BOT_CLASS_MAGE:
            return _enableclass_mage;
        case BOT_CLASS_WARLOCK:
            return _enableclass_warlock;
        case BOT_CLASS_DRUID:
            return _enableclass_druid;
        case BOT_CLASS_BM:
            return _enableclass_blademaster;
        case BOT_CLASS_SPHYNX:
            return _enableclass_sphynx;
        case BOT_CLASS_ARCHMAGE:
            return _enableclass_archmage;
        case BOT_CLASS_DREADLORD:
            return _enableclass_dreadlord;
        case BOT_CLASS_SPELLBREAKER:
            return _enableclass_spellbreaker;
        case BOT_CLASS_DARK_RANGER:
            return _enableclass_darkranger;
        case BOT_CLASS_NECROMANCER:
            return _enableclass_necromancer;
        case BOT_CLASS_SEA_WITCH:
            return _enableclass_seawitch;
        case BOT_CLASS_CRYPT_LORD:
            return _enableclass_cryptlord;
        default:
            return true;
    }
}

bool BotCfg::IsWanderingClassEnabled(uint8 m_class)
{
    switch (m_class)
    {
        case BOT_CLASS_WARRIOR:
            return _enableclass_wander_warrior;
        case BOT_CLASS_PALADIN:
            return _enableclass_wander_paladin;
        case BOT_CLASS_HUNTER:
            return _enableclass_wander_hunter;
        case BOT_CLASS_ROGUE:
            return _enableclass_wander_rogue;
        case BOT_CLASS_PRIEST:
            return _enableclass_wander_priest;
        case BOT_CLASS_DEATH_KNIGHT:
            return _enableclass_wander_deathknight;
        case BOT_CLASS_SHAMAN:
            return _enableclass_wander_shaman;
        case BOT_CLASS_MAGE:
            return _enableclass_wander_mage;
        case BOT_CLASS_WARLOCK:
            return _enableclass_wander_warlock;
        case BOT_CLASS_DRUID:
            return _enableclass_wander_druid;
        case BOT_CLASS_BM:
            return _enableclass_wander_blademaster;
        case BOT_CLASS_SPHYNX:
            return _enableclass_wander_sphynx;
        case BOT_CLASS_ARCHMAGE:
            return _enableclass_wander_archmage;
        case BOT_CLASS_DREADLORD:
            return _enableclass_wander_dreadlord;
        case BOT_CLASS_SPELLBREAKER:
            return _enableclass_wander_spellbreaker;
        case BOT_CLASS_DARK_RANGER:
            return _enableclass_wander_darkranger;
        case BOT_CLASS_NECROMANCER:
            return _enableclass_wander_necromancer;
        case BOT_CLASS_SEA_WITCH:
            return _enableclass_wander_seawitch;
        case BOT_CLASS_CRYPT_LORD:
            return _enableclass_wander_cryptlord;
        default:
            return true;
    }
}

bool BotCfg::EnableWanderingUntargetNpcQuestgiver()
{
    return _untarget_wnpc_questgiver;
}
bool BotCfg::EnableWanderingUntargetNpcFlightmaster()
{
    return _untarget_wnpc_flightmaster;
}

bool BotCfg::HideBotSpawns()
{
    return _hideSpawns;
}
bool BotCfg::IsEnrageOnDimissEnabled()
{
    return _enrageOnDismiss;
}
bool BotCfg::IsBotStatsLimitsEnabled()
{
    return _botStatLimits;
}
bool BotCfg::IsPvPEnabled()
{
    return _botPvP;
}
bool BotCfg::IsFoodInterruptedByMovement()
{
    return _botMovementFoodInterrupt;
}
bool BotCfg::FilterRaces()
{
    return _filterRaces;
}
bool BotCfg::IsBotGenerationEnabledBGs()
{
    return _enableWanderingBotsBG;
}
bool BotCfg::IsBotLevelCappedByConfigBG()
{
    return _enableConfigLevelCapBG;
}
bool BotCfg::IsBotLevelCappedByConfigBGFirstPlayer()
{
    return _enableConfigLevelCapBGFirst;
}
bool BotCfg::IsBotGenerationEnabledWorldMapId(uint32 mapId)
{
    return std::ranges::find(_enabled_wander_node_maps, mapId) != std::cend(_enabled_wander_node_maps);
}
bool BotCfg::IsBotHKEnabled()
{
    return _bothk_enable;
}
bool BotCfg::IsBotHKMessageEnabled()
{
    return _bothk_message_enable;
}
bool BotCfg::IsBotHKAchievementsEnabled()
{
    return _bothk_achievements_enable;
}
bool BotCfg::IsSharedOwnerOptionEnabled(SharedOwnerOptionMask options)
{
    return std::ranges::all_of(std::array{ SHARED_OWNER_OPTION_MASK_ENABLE, options }, [=](uint32 mask) { return !!(_shared_ownership_options & mask); });
}
uint8 BotCfg::GetMaxClassBots()
{
    return _maxClassNpcBots;
}
uint8 BotCfg::GetMaxAccountBots()
{
    return _maxAccountNpcBots;
}
uint8 BotCfg::GetMaxSharedOwners()
{
    return _maxSharedOwners;
}
uint32 BotCfg::GetGearBankCapacity()
{
    return _gearBankCapacity;
}
uint32 BotCfg::GetGearBankEquipmentSetsCount()
{
    return _gearBankEquipmentSetsCount;
}
uint8 BotCfg::GetHealTargetIconFlags()
{
    return _healTargetIconFlags;
}
uint8 BotCfg::GetTankTargetIconFlags()
{
    return _tankingTargetIconFlags;
}
uint8 BotCfg::GetOffTankTargetIconFlags()
{
    return _offTankingTargetIconFlags;
}
uint8 BotCfg::GetDPSTargetIconFlags()
{
    return _dpsTargetIconFlags;
}
uint8 BotCfg::GetRangedDPSTargetIconFlags()
{
    return _rangedDpsTargetIconFlags;
}
uint8 BotCfg::GetNoDPSTargetIconFlags()
{
    return _noDpsTargetIconFlags;
}
uint32 BotCfg::GetBaseUpdateDelay()
{
    return _npcBotUpdateDelayBase;
}
uint32 BotCfg::GetOwnershipExpireTime()
{
    return _npcBotOwnerExpireTime;
}
uint8 BotCfg::GetOwnershipExpireMode()
{
    return _npcBotOwnerExpireMode;
}
uint32 BotCfg::GetDesiredWanderingBotsCount()
{
    return _desiredWanderingBotsCount;
}
uint32 BotCfg::GetBGTargetTeamPlayersCount(BattlegroundTypeId bgTypeId)
{
    switch (bgTypeId)
    {
        case BATTLEGROUND_AV:
            return _targetBGPlayersPerTeamCount_AV;
        case BATTLEGROUND_WS:
            return _targetBGPlayersPerTeamCount_WS;
        case BATTLEGROUND_AB:
            return _targetBGPlayersPerTeamCount_AB;
        case BATTLEGROUND_EY:
            return _targetBGPlayersPerTeamCount_EY;
        case BATTLEGROUND_SA:
            return _targetBGPlayersPerTeamCount_SA;
        case BATTLEGROUND_IC:
            return _targetBGPlayersPerTeamCount_IC;
        default:
            return 0;
    }
}
float BotCfg::GetBotHKHonorRate()
{
    return _bothk_rate_honor;
}
float BotCfg::GetBotStatLimitDodge()
{
    return _botStatLimits_dodge;
}
float BotCfg::GetBotStatLimitParry()
{
    return _botStatLimits_parry;
}
float BotCfg::GetBotStatLimitBlock()
{
    return _botStatLimits_block;
}
float BotCfg::GetBotStatLimitCrit()
{
    return _botStatLimits_crit;
}

bool BotCfg::IsNpcBotXpReductionEnabled()
{
    return _xpReductionEnable;
}
bool BotCfg::IsNpcBotXpReductionGroupOnly()
{
    return _xpReductionGroupOnly;
}
uint8 BotCfg::GetNpcBotXpReductionExtraAmount()
{
    return _xpReductionExtraAmount;
}
uint8 BotCfg::GetNpcBotXpReductionExtraStartingNumber()
{
    return _xpReductionExtraStartingNumber;
}

bool BotCfg::IsNpcBotHonorReductionEnabled()
{
    return _honorReductionEnable;
}
bool BotCfg::IsNpcBotHonorReductionGroupOnly()
{
    return _honorReductionGroupOnly;
}

bool BotCfg::GetNpcBotMoneyShareEnabled()
{
    return _moneyLootShareEnable;
}
bool BotCfg::GetNpcBotMoneyShareGroupOnly()
{
    return _moneyLootShareGroupOnly;
}

uint8 BotCfg::GetNpcBotMountLevel60()
{
    return _mountLevel60;
}
uint8 BotCfg::GetNpcBotMountLevel100()
{
    return _mountLevel100;
}

uint8 BotCfg::GetMaxNpcBots(uint8 level)
{
    return _max_npcbots[std::min<size_t>(BRACKETS_COUNT - 1, level / 10)];
}

int32 BotCfg::GetBotInfoPacketsLimit()
{
    return _botInfoPacketsLimit;
}

float BotCfg::GetBotDamageModPhysical()
{
    return _mult_dmg_physical;
}
float BotCfg::GetBotDamageModSpell()
{
    return _mult_dmg_spell;
}
float BotCfg::GetBotHealingMod()
{
    return _mult_healing;
}
float BotCfg::GetBotHPMod()
{
    return _mult_hp;
}
float BotCfg::GetBotWandererDamageMod()
{
    return _mult_dmg_wanderer;
}
float BotCfg::GetBotWandererHealingMod()
{
    return _mult_healing_wanderer;
}
float BotCfg::GetBotWandererHPMod()
{
    return _mult_hp_wanderer;
}
float BotCfg::GetBotWandererSpeedMod()
{
    return _mult_speed_wanderer;
}
float BotCfg::GetBotWandererXPGainMod()
{
    return _mult_xpgain_wanderer;
}
PctBrackets BotCfg::GetBotWandererLevelBrackets()
{
    return _botwanderer_pct_level_brackets;
}
uint32 BotCfg::GetBotWandererMaxItemLevel(uint8 level)
{
    return _botwanderer_itemlvl_level_brackets[std::min<std::size_t>(BRACKETS_COUNT - 1, level / 10)];
}
uint32 BotCfg::GetBotWandererKillRewardMoney()
{
    return _killrewardWandererMoneyBase;
}
uint32 BotCfg::GetBotWandererKillRewardItemMaxCount()
{
    return _killrewardWandererItemCount;
}
uint32 BotCfg::GetBotWandererKillRewardItemMaxQuality()
{
    return _killrewardWandererItemQuality;
}

uint32 BotCfg::GetBotDungeonMaxItemLevel(uint8 level, uint16 map_id, Difficulty map_difficulty)
{
    const bool heroic = map_difficulty == Difficulty::DUNGEON_DIFFICULTY_HEROIC;
    auto ci = std::ranges::find_if(MAP_MAX_ITEM_LEVELS, [map_id](BotMapItemLevelRange r) { return r.map_id == map_id; });
    const uint16 ilvl = ci != MAP_MAX_ITEM_LEVELS.cend() ? heroic ? ci->heroic : ci->normal : heroic ? MAX_ITEM_LEVEL_WOTLK_HEROIC : MAX_ITEM_LEVEL_WOTLK_NORMAL;
    return static_cast<uint32>(std::ceil(CalculatePct(static_cast<float>(ilvl), _botdungeon_itemlvl_brackets[std::min<std::size_t>(BRACKETS_COUNT - 1, level / 10)])));
}

float BotCfg::GetBotDamageModByClass(uint8 botclass)
{
    switch (botclass)
    {
        case BOT_CLASS_WARRIOR:
            return _mult_dmg_warrior;
        case BOT_CLASS_PALADIN:
            return _mult_dmg_paladin;
        case BOT_CLASS_HUNTER:
            return _mult_dmg_hunter;
        case BOT_CLASS_ROGUE:
            return _mult_dmg_rogue;
        case BOT_CLASS_PRIEST:
            return _mult_dmg_priest;
        case BOT_CLASS_DEATH_KNIGHT:
            return _mult_dmg_deathknight;
        case BOT_CLASS_SHAMAN:
            return _mult_dmg_shaman;
        case BOT_CLASS_MAGE:
            return _mult_dmg_mage;
        case BOT_CLASS_WARLOCK:
            return _mult_dmg_warlock;
        case BOT_CLASS_DRUID:
            return _mult_dmg_druid;
        case BOT_CLASS_BM:
            return _mult_dmg_blademaster;
        case BOT_CLASS_SPHYNX:
            return _mult_dmg_obsidiandestroyer;
        case BOT_CLASS_ARCHMAGE:
            return _mult_dmg_archmage;
        case BOT_CLASS_DREADLORD:
            return _mult_dmg_dreadlord;
        case BOT_CLASS_SPELLBREAKER:
            return _mult_dmg_spellbreaker;
        case BOT_CLASS_DARK_RANGER:
            return _mult_dmg_darkranger;
        case BOT_CLASS_NECROMANCER:
            return _mult_dmg_necromancer;
        case BOT_CLASS_SEA_WITCH:
            return _mult_dmg_seawitch;
        case BOT_CLASS_CRYPT_LORD:
            return _mult_dmg_cryptlord;
        default:
            return 1.0;
    }
}

float BotCfg::GetBotDamageModByLevel(uint8 botlevel)
{
    uint8 bracket = botlevel / 10;
    if (bracket < _mult_dmg_levels.size())
        return _mult_dmg_levels[bracket];
    return 1.0f;
}
float BotCfg::GetBotHealingModByLevel(uint8 botlevel)
{
    uint8 bracket = botlevel / 10;
    if (bracket < _mult_heal_levels.size())
        return _mult_heal_levels[bracket];
    return 1.0f;
}
float BotCfg::GetBotHPModByLevel(uint8 botlevel)
{
    uint8 bracket = botlevel / 10;
    if (bracket < _mult_hp_levels.size())
        return _mult_hp_levels[bracket];
    return 1.0f;
}
float BotCfg::GetBotMPModByLevel(uint8 botlevel)
{
    uint8 bracket = botlevel / 10;
    if (bracket < _mult_mp_levels.size())
        return _mult_mp_levels[bracket];
    return 1.0f;
}

uint8 BotCfg::GetFollowDistDefault()
{
    return _basefollowdist;
}
uint32 BotCfg::GetEngageDelayDPSDefault()
{
    return _npcBotEngageDelayDPS_default;
}
uint32 BotCfg::GetEngageDelayHealDefault()
{
    return _npcBotEngageDelayHeal_default;
}

bool BotCfg::IsMapAllowedForBots(Map const* map)
{
    if ((!_enableNpcBotsBGs && map->IsBattleground()) ||
        (!_enableNpcBotsArenas && map->IsBattleArena()) ||
        (!_enableNpcBotsDungeons && map->IsNonRaidDungeon()) ||
        (!_enableNpcBotsRaids && map->IsRaid()))
        return false;

    if (map->IsDungeon() && !_disabled_instance_maps.empty() && std::ranges::find(_disabled_instance_maps, map->GetId()) != _disabled_instance_maps.cend())
        return false;

    return true;
}

uint32 BotCfg::_normalizedCostForLevel(uint32 cost_base, uint8 bot_class, uint8 level)
{
    //assuming default 1000000
    //level 1: 500  //5  silver
    //10 : 10000    //1  gold
    //20 : 50000    //5  gold
    //30 : 200000   //20 gold
    //40 : 500000   //50 gold
    //rest is linear
    //rare / rareelite bots have their cost adjusted
    uint32 cost =
        level < 10 ? cost_base / 2000 : //5 silver
        level < 20 ? cost_base / 100 :  //1 gold
        level < 30 ? cost_base / 20 :   //5 gold
        level < 40 ? cost_base / 5 :    //20 gold
        (cost_base * (level - (level % 10))) / DEFAULT_MAX_LEVEL; //50 - 100 gold

    switch (bot_class)
    {
        case BOT_CLASS_BM:
        case BOT_CLASS_ARCHMAGE:
        case BOT_CLASS_SPELLBREAKER:
        case BOT_CLASS_NECROMANCER:
            cost += cost; //200%
            break;
        case BOT_CLASS_SPHYNX:
        case BOT_CLASS_DREADLORD:
        case BOT_CLASS_DARK_RANGER:
        case BOT_CLASS_SEA_WITCH:
        case BOT_CLASS_CRYPT_LORD:
            cost += cost * 4; //500%
            break;
        default:
            break;
    }

    return cost;
}

void AddSC_botconfig_scripts()
{
    new NPCBotsConfigScript();
}

#ifdef _MSC_VER
# pragma warning(pop)
#endif
