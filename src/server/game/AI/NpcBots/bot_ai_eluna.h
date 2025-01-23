#ifndef BOT_AI_ELUNA_H
#define BOT_AI_ELUNA_H


#include <string>

class bot_ai; // Forward declaration

/**
 * Method to enable additionally functionality for
 * Eluna control of NPCBots in AzerothCore.
 */
class BotAIEluna
{
    public:
        BotAIEluna(bot_ai* ai) : _ai(ai) { }
        ~BotAIEluna() = default;

        bool CanEquip(ItemTemplate const* newProto, uint8 slot, bool ignoreItemLevel, Item const* newItem = nullptr);
        bool Equip(uint8 slot, Item* newItem, ObjectGuid receiver);
        bool Unequip(uint8 slot, ObjectGuid receiver);
        std::string BotDump(Player const* player, Unit const* unit) const;

    private:
        bot_ai* _ai;
};

#endif // _BOT_AI_H
