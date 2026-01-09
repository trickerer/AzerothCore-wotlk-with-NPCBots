--
ALTER TABLE `creature_template_npcbot_wander_nodes` ADD `minwaittime` INT UNSIGNED NOT NULL DEFAULT '3000' AFTER `maxlevel`, ADD `maxwaittime` INT UNSIGNED NOT NULL DEFAULT '7000' AFTER `minwaittime`, ADD `proximity` FLOAT NOT NULL DEFAULT '0' AFTER `maxwaittime`;
