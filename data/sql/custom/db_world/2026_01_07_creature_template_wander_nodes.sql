-- Insert new columns minwaittime, maxwaittime, proximity

DELIMITER $$

CREATE PROCEDURE tmpAddColumnIfNotExists()
BEGIN
    IF NOT EXISTS (
        SELECT * FROM INFORMATION_SCHEMA.COLUMNS
        WHERE TABLE_SCHEMA = DATABASE()
        AND TABLE_NAME = 'creature_template_npcbot_wander_nodes'
        AND COLUMN_NAME = 'minwaittime'
    ) THEN
        ALTER TABLE `creature_template_npcbot_wander_nodes` 
            ADD `minwaittime` INT UNSIGNED NOT NULL DEFAULT '3000' AFTER `maxlevel`, 
            ADD `maxwaittime` INT UNSIGNED NOT NULL DEFAULT '7000' AFTER `minwaittime`, 
            ADD `proximity` FLOAT NOT NULL DEFAULT '0' AFTER `maxwaittime`;
    END IF;
END$$

DELIMITER ;

-- Call the procedure to run the logic
CALL tmpAddColumnIfNotExists();

-- Drop the procedure afterward if it's a one-off change
DROP PROCEDURE tmpAddColumnIfNotExists;
