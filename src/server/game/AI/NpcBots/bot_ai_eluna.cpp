#include "bot_ai.h"
#include "bot_ai_eluna.h"
#include "botcommon.h"

bool BotAIEluna::CanEquip(ItemTemplate const* newProto, uint8 slot, bool ignoreItemLevel, Item const* newItem)
{
    return _ai->_canEquip(newProto, slot, ignoreItemLevel, newItem);
}

bool BotAIEluna::Unequip(uint8 slot, ObjectGuid receiver)
{
    BotEquipResult result = _ai->_unequip(slot, receiver, false, false);
    return result == BotEquipResult::BOT_EQUIP_RESULT_OK;
}

bool BotAIEluna::Equip(uint8 slot, Item* newItem, ObjectGuid receiver)
{
    BotEquipResult result = _ai->_equip(slot, newItem, receiver, false, false);
    return result == BotEquipResult::BOT_EQUIP_RESULT_OK;
}

std::string BotAIEluna::BotDump(Player const* player, Unit const* unit) const
{
    return _ai->_listAuras(player, unit, false);
}
