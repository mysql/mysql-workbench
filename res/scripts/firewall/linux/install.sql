# Copyright (c) 2015 Oracle and/or its affiliates. All rights reserved.
# Install firewall tables
USE mysql;
CREATE TABLE IF NOT EXISTS mysql.firewall_whitelist( USERHOST VARCHAR(80) NOT NULL, RULE text NOT NULL) engine= MyISAM;
CREATE TABLE IF NOT EXISTS mysql.firewall_users( USERHOST VARCHAR(80) PRIMARY KEY, MODE ENUM ('OFF', 'RECORDING', 'PROTECTING', 'RESET') DEFAULT 'OFF') engine= MyISAM;

INSTALL PLUGIN mysql_firewall SONAME 'firewall.so';
INSTALL PLUGIN mysql_firewall_whitelist SONAME 'firewall.so';
INSTALL PLUGIN mysql_firewall_users SONAME 'firewall.so';

CREATE FUNCTION set_firewall_mode RETURNS STRING SONAME 'firewall.so';
CREATE FUNCTION normalize_statement RETURNS STRING SONAME 'firewall.so';
CREATE AGGREGATE FUNCTION read_firewall_whitelist RETURNS STRING SONAME 'firewall.so';
CREATE AGGREGATE FUNCTION read_firewall_users RETURNS STRING SONAME 'firewall.so';
delimiter //
CREATE PROCEDURE sp_set_firewall_mode (IN arg_userhost VARCHAR(80), IN arg_mode varchar(12))
BEGIN
IF arg_mode = "RECORDING" THEN
  SELECT read_firewall_whitelist(arg_userhost,FW.rule) FROM mysql.firewall_whitelist FW WHERE FW.userhost=arg_userhost;
END IF;
SELECT set_firewall_mode(arg_userhost, arg_mode);
if arg_mode = "RESET" THEN
  SET arg_mode = "OFF";
END IF;
INSERT IGNORE INTO mysql.firewall_users VALUES (arg_userhost, arg_mode);
UPDATE mysql.firewall_users SET mode=arg_mode WHERE userhost = arg_userhost;

IF arg_mode = "PROTECTING" OR arg_mode = "OFF" THEN
  DELETE FROM mysql.firewall_whitelist WHERE USERHOST = arg_userhost;
  INSERT INTO mysql.firewall_whitelist SELECT USERHOST,RULE FROM INFORMATION_SCHEMA.mysql_firewall_whitelist WHERE USERHOST=arg_userhost;
END IF;
END //
delimiter ;


