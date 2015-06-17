USE mysql;

UNINSTALL PLUGIN mysql_firewall_users;
UNINSTALL PLUGIN mysql_firewall_whitelist;
UNINSTALL PLUGIN mysql_firewall;

DROP TABLE IF EXISTS mysql.firewall_whitelist;
DROP TABLE IF EXISTS mysql.firewall_users;

DROP FUNCTION IF EXISTS set_firewall_mode;
DROP FUNCTION IF EXISTS normalize_statement;

DROP FUNCTION IF EXISTS read_firewall_whitelist;
DROP FUNCTION IF EXISTS read_firewall_users;

DROP PROCEDURE IF EXISTS sp_set_firewall_mode; 
