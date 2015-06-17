USE mysql;

DROP TABLE IF EXISTS mysql.firewall_whitelist;
DROP TABLE IF EXISTS mysql.firewall_users;

DROP FUNCTION IF EXISTS set_firewall_mode;
DROP FUNCTION IF EXISTS normalize_statement;

#DROP AGGREGATE IF EXISTS read_firewall_whitelist;


DROP PROCEDURE IF EXISTS sp_set_firewall_mode; 
