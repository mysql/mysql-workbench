-- This script creates a table named general_log in the test database and
-- populates it with some values.

START TRANSACTION;

CREATE DATABASE IF NOT EXISTS test;
USE test;

DROP TABLE IF EXISTS `general_log`;

CREATE TABLE `general_log` (
  `event_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `user_host` mediumtext NOT NULL,
  `thread_id` int(11) NOT NULL,
  `server_id` int(10) unsigned NOT NULL,
  `command_type` varchar(64) NOT NULL,
  `argument` mediumtext NOT NULL
) ENGINE=CSV DEFAULT CHARSET=utf8 COMMENT='General log';

INSERT INTO `general_log` VALUES ('2011-10-10 15:19:22','root[root] @ localhost []',3,1,'Quit','');
INSERT INTO `general_log` VALUES ('2011-10-10 15:19:36','[root] @ localhost []',4,1,'Connect','root@localhost on ');
INSERT INTO `general_log` VALUES ('2011-10-10 15:19:36','root[root] @ localhost []',4,1,'Query','set autocommit=1');
INSERT INTO `general_log` VALUES ('2011-10-10 15:19:36','root[root] @ localhost []',4,1,'Query','SET SESSION TRANSACTION ISOLATION LEVEL REPEATABLE READ');
INSERT INTO `general_log` VALUES ('2011-10-10 15:19:36','root[root] @ localhost []',4,1,'Query','SHOW SESSION VARIABLES LIKE \'lower_case_table_names\'');
INSERT INTO `general_log` VALUES ('2011-10-10 15:19:36','root[root] @ localhost []',4,1,'Query','select version() as version');
INSERT INTO `general_log` VALUES ('2011-10-10 15:19:36','root[root] @ localhost []',4,1,'Query','show plugins');
INSERT INTO `general_log` VALUES ('2011-10-10 15:19:36','root[root] @ localhost []',4,1,'Query','SHOW VARIABLES LIKE \'hostname\'');
INSERT INTO `general_log` VALUES ('2011-10-10 15:19:36','root[root] @ localhost []',4,1,'Query','SHOW VARIABLES LIKE \'datadir\'');
INSERT INTO `general_log` VALUES ('2011-10-10 15:19:36','root[root] @ localhost []',4,1,'Query','SHOW VARIABLES LIKE \'general_log_file\'');
INSERT INTO `general_log` VALUES ('2011-10-10 15:19:36','root[root] @ localhost []',4,1,'Query','SHOW VARIABLES LIKE \'slow_query_log_file\'');
INSERT INTO `general_log` VALUES ('2011-10-10 15:19:36','root[root] @ localhost []',4,1,'Query','SHOW VARIABLES LIKE \'log_error\'');
INSERT INTO `general_log` VALUES ('2011-10-10 15:19:36','[root] @ localhost []',5,1,'Connect','root@localhost on ');
INSERT INTO `general_log` VALUES ('2011-10-10 15:19:36','root[root] @ localhost []',5,1,'Query','set autocommit=1');
INSERT INTO `general_log` VALUES ('2011-10-10 15:19:36','root[root] @ localhost []',5,1,'Query','SET SESSION TRANSACTION ISOLATION LEVEL REPEATABLE READ');
INSERT INTO `general_log` VALUES ('2011-10-10 15:19:36','root[root] @ localhost []',5,1,'Query','SHOW SESSION VARIABLES LIKE \'lower_case_table_names\'');
INSERT INTO `general_log` VALUES ('2011-10-10 15:19:36','root[root] @ localhost []',4,1,'Query','SHOW PROCESSLIST');
INSERT INTO `general_log` VALUES ('2011-10-10 15:19:36','root[root] @ localhost []',4,1,'Query','SHOW PROCESSLIST');
INSERT INTO `general_log` VALUES ('2011-10-10 15:19:36','root[root] @ localhost []',4,1,'Query','select version() as version');
INSERT INTO `general_log` VALUES ('2011-10-10 15:19:36','root[root] @ localhost []',4,1,'Query','show plugins');

COMMIT;
