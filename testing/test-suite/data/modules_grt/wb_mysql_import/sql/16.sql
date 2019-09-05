CREATE SCHEMA `test`;
USE `test`;

-- create object per each object type
CREATE VIEW `view1` AS SELECT `id` FROM `table1`;

CREATE TABLE `table1` (`id` INTEGER);

CREATE UNIQUE INDEX `index1` ON `table1` (`id`);

DELIMITER //
CREATE TRIGGER `trigger1` BEFORE INSERT ON `table1` FOR EACH ROW BEGIN END;
//
CREATE FUNCTION `func1`() RETURNS tinyint(4) return 1
//
CREATE PROCEDURE `proc1`() BEGIN END
//
DELIMITER ;

CREATE LOGFILE GROUP `logfile_group1`
ADD UNDOFILE 'undo_file1.dat'
INITIAL_SIZE = 128
UNDO_BUFFER_SIZE = 8
ENGINE = NDB;

CREATE TABLESPACE `tablespace1`
ADD DATAFILE 'file1'
USE LOGFILE GROUP `logfile_group1`
EXTENT_SIZE = 4
INITIAL_SIZE = 1G
ENGINE = NDB;

CREATE SERVER `server_name1`
FOREIGN DATA WRAPPER `wrapper_name1`
OPTIONS(
HOST 'host-literal',
DATABASE 'database-literal',
USER 'user-literal',
PASSWORD 'password-literal',
SOCKET 'socket-literal',
OWNER 'owner-literal',
PORT 3306);

-- repeat creation of all the same objects in the same schema
CREATE VIEW `view1` AS SELECT `id` FROM `table1`;

CREATE TABLE `table1` (`id` INTEGER);

CREATE UNIQUE INDEX `index1` ON `table1` (`id`);

DELIMITER //
CREATE TRIGGER `trigger1` BEFORE INSERT ON `table1` FOR EACH ROW BEGIN END
//
CREATE FUNCTION `func1`() RETURNS tinyint(4) return 1
//
CREATE PROCEDURE `proc1`() BEGIN END
//
DELIMITER ;

CREATE LOGFILE GROUP `logfile_group1`
ADD UNDOFILE 'undo_file1.dat'
INITIAL_SIZE = 128
UNDO_BUFFER_SIZE = 8
ENGINE = NDB;

CREATE TABLESPACE `tablespace1`
ADD DATAFILE 'file1'
USE LOGFILE GROUP `logfile_group1`
EXTENT_SIZE = 4
INITIAL_SIZE = 1M
ENGINE = NDB;

CREATE SERVER `server_name1`
FOREIGN DATA WRAPPER `wrapper_name1`
OPTIONS(
HOST 'host-literal',
DATABASE 'database-literal',
USER 'user-literal',
PASSWORD 'password-literal',
SOCKET 'socket-literal',
OWNER 'owner-literal',
PORT 3306);

CREATE SCHEMA `test2`;
USE `test2`;

-- repeat creation of all the same objects now in another schema
CREATE VIEW `view1` AS SELECT `id` FROM `table1`;

CREATE TABLE `table1` (`id` INTEGER);

CREATE UNIQUE INDEX `index1` ON `table1` (`id`);

DELIMITER //
CREATE TRIGGER `trigger1` BEFORE INSERT ON `table1` FOR EACH ROW BEGIN END
//
CREATE FUNCTION `func1`() RETURNS tinyint(4) return 1
//
CREATE PROCEDURE `proc1`() BEGIN
/*
some very long comment (to check sql statement cutting)
some very long comment (to check sql statement cutting)
some very long comment (to check sql statement cutting)
some very long comment (to check sql statement cutting)
some very long comment (to check sql statement cutting)
some very long comment (to check sql statement cutting)
some very long comment (to check sql statement cutting)
some very long comment (to check sql statement cutting)
some very long comment (to check sql statement cutting)
some very long comment (to check sql statement cutting)
*/
END
//
DELIMITER ;

-- Log file groups and table spaces are bound to the catalog and hence overwrite the definitions
-- from above which we have defined in the context of the first schema.
CREATE LOGFILE GROUP `logfile_group1`
ADD UNDOFILE 'undo_file1.dat'
INITIAL_SIZE = 128
UNDO_BUFFER_SIZE = 8
ENGINE = NDB;

CREATE TABLESPACE `tablespace1`
ADD DATAFILE 'file1'
USE LOGFILE GROUP `logfile_group1`
EXTENT_SIZE = 4
INITIAL_SIZE = 3M
ENGINE = NDB;

CREATE SERVER `server_name1`
FOREIGN DATA WRAPPER `wrapper_name1`
OPTIONS(
HOST 'host-literal',
DATABASE 'database-literal',
USER 'user-literal',
PASSWORD 'password-literal',
SOCKET 'socket-literal',
OWNER 'owner-literal',
PORT 3306);
