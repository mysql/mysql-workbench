/*
CREATE LOGFILE GROUP logfile_group
    ADD UNDOFILE 'undo_file'
    [INITIAL_SIZE [=] initial_size]
    [UNDO_BUFFER_SIZE [=] undo_buffer_size]
    ENGINE [=] engine_name
*/
/*
M or G is not supported in size constants contrary to documentation
*/

CREATE LOGFILE GROUP logfile_group1
ADD UNDOFILE 'undo_file1.dat'
INITIAL_SIZE = 128
UNDO_BUFFER_SIZE = 8
ENGINE = NDB;

/*
CREATE TABLESPACE tablespace
    ADD DATAFILE 'file'
    USE LOGFILE GROUP logfile_group
    [EXTENT_SIZE [=] extent_size]
    [INITIAL_SIZE [=] initial_size]
    ENGINE [=] engine
*/
/*
M or G is not supported in size constants contrary to documentation
*/

CREATE TABLESPACE tablespace1
ADD DATAFILE 'file1'
USE LOGFILE GROUP logfile_group1
EXTENT_SIZE = 4
INITIAL_SIZE = 128
ENGINE = NDB;
