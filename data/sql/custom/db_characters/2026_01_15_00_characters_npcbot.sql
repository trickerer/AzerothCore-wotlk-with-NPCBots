/*!50003 DROP PROCEDURE IF EXISTS `sp__add_column_if_not_exists`*/;

DELIMITER ;;

/*!50003 CREATE*/
/*!50003 PROCEDURE `sp__add_column_if_not_exists`(IN p_table_name VARCHAR(64), IN p_column_name VARCHAR(64), IN p_column_options TEXT)
BEGIN
    IF NOT EXISTS (
        SELECT 1
        FROM INFORMATION_SCHEMA.COLUMNS
        WHERE TABLE_SCHEMA = DATABASE()
          AND TABLE_NAME = p_table_name
          AND COLUMN_NAME = p_column_name
    ) THEN
        SET @sql = CONCAT('ALTER TABLE ', p_table_name, ' ADD ', p_column_name, ' ', p_column_options);

        PREPARE stmt FROM @sql;
        EXECUTE stmt;
        DEALLOCATE PREPARE stmt;
    END IF;
END */;;

DELIMITER ;

CALL sp__add_column_if_not_exists('`characters_npcbot`', '`shared_owners`', 'longtext CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci DEFAULT NULL AFTER `hire_time`');

DROP PROCEDURE `sp__add_column_if_not_exists`;
