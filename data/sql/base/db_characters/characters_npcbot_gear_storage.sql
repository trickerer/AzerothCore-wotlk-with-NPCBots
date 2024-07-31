--
SET FOREIGN_KEY_CHECKS=0;
-- ----------------------------
-- Table structure for characters_npcbot_gear_storage
-- ----------------------------
DROP TABLE IF EXISTS `characters_npcbot_gear_storage`;
CREATE TABLE `characters_npcbot_gear_storage` (
  `guid` int(10) unsigned NOT NULL DEFAULT '0',
  `item_guid` int(10) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`item_guid`),
  KEY `existing_player` (`guid`),
  CONSTRAINT `FK_npcbot_gear_storage_item_guid` FOREIGN KEY (`item_guid`) REFERENCES `item_instance` (`guid`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `FK_npcbot_gear_storage_guid` FOREIGN KEY (`guid`) REFERENCES `characters` (`guid`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='NPC Bot Gear Storage System';
