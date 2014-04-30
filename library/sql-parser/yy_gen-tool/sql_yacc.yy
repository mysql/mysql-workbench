/* 
   Copyright (c) 2000, 2014, Oracle and/or its affiliates. All rights reserved.
  
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; version 2 of the
   License.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
   02110-1301  USA
 */

/* sql_yacc.yy */

%{
typedef void* YYSTYPE;
#define YYSTYPE_IS_DECLARED

#include <stdio.h>
#include "myx_sql_tree_item.h"
#include "myx_lex_helpers.h"
using mysql_parser::yyerror;
using mysql_parser::yylex;
%}

%pure_parser                                    /* We have threads */

/*
   Comments for TOKENS.
   For each token, please include in the same line a comment that contains
   the following tags:
   SQL-2003-R : Reserved keyword as per SQL-2003
   SQL-2003-N : Non Reserved keyword as per SQL-2003
   SQL-1999-R : Reserved keyword as per SQL-1999
   SQL-1999-N : Non Reserved keyword as per SQL-1999
   MYSQL      : MySQL extention (unspecified)
   MYSQL-FUNC : MySQL extention, function
   INTERNAL   : Not a real token, lex optimization
   OPERATOR   : SQL operator
   FUTURE-USE : Reserved for futur use

   This makes the code grep-able, and helps maintenance.
*/

%token  ABORT_SYM                     /* INTERNAL (used in lex) */
%token  ACCESSIBLE_SYM
%token  ACTION                        /* SQL-2003-N */
%token  ADD                           /* SQL-2003-R */
%token  ADDDATE_SYM                   /* MYSQL-FUNC */
%token  AFTER_SYM                     /* SQL-2003-N */
%token  AGAINST
%token  AGGREGATE_SYM
%token  ALGORITHM_SYM
%token  ALL                           /* SQL-2003-R */
%token  ALTER                         /* SQL-2003-R */
%token  ANALYSE_SYM
%token  ANALYZE_SYM
%token  AND_AND_SYM                   /* OPERATOR */
%token  AND_SYM                       /* SQL-2003-R */
%token  ANY_SYM                       /* SQL-2003-R */
%token  AS                            /* SQL-2003-R */
%token  ASC                           /* SQL-2003-N */
%token  ASCII_SYM                     /* MYSQL-FUNC */
%token  ASENSITIVE_SYM                /* FUTURE-USE */
%token  AT_SYM                        /* SQL-2003-R */
%token  AUTOEXTEND_SIZE_SYM
%token  AUTO_INC
%token  AVG_ROW_LENGTH
%token  AVG_SYM                       /* SQL-2003-N */
%token  BACKUP_SYM
%token  BEFORE_SYM                    /* SQL-2003-N */
%token  BEGIN_SYM                     /* SQL-2003-R */
%token  BETWEEN_SYM                   /* SQL-2003-R */
%token  BIGINT                        /* SQL-2003-R */
%token  BINARY                        /* SQL-2003-R */
%token  BINLOG_SYM
%token  BIN_NUM
%token  BIT_AND                       /* MYSQL-FUNC */
%token  BIT_OR                        /* MYSQL-FUNC */
%token  BIT_SYM                       /* MYSQL-FUNC */
%token  BIT_XOR                       /* MYSQL-FUNC */
%token  BLOB_SYM                      /* SQL-2003-R */
%token  BLOCK_SYM
%token  BOOLEAN_SYM                   /* SQL-2003-R */
%token  BOOL_SYM
%token  BOTH                          /* SQL-2003-R */
%token  BTREE_SYM
%token  BY                            /* SQL-2003-R */
%token  BYTE_SYM
%token  CACHE_SYM
%token  CALL_SYM                      /* SQL-2003-R */
%token  CASCADE                       /* SQL-2003-N */
%token  CASCADED                      /* SQL-2003-R */
%token  CASE_SYM                      /* SQL-2003-R */
%token  CAST_SYM                      /* SQL-2003-R */
%token  CATALOG_NAME_SYM              /* SQL-2003-N */
%token  CHAIN_SYM                     /* SQL-2003-N */
%token  CHANGE
%token  CHANGED
%token  CHARSET
%token  CHAR_SYM                      /* SQL-2003-R */
%token  CHECKSUM_SYM
%token  CHECK_SYM                     /* SQL-2003-R */
%token  CIPHER_SYM
%token  CLASS_ORIGIN_SYM              /* SQL-2003-N */
%token  CLIENT_SYM
%token  CLOSE_SYM                     /* SQL-2003-R */
%token  COALESCE                      /* SQL-2003-N */
%token  CODE_SYM
%token  COLLATE_SYM                   /* SQL-2003-R */
%token  COLLATION_SYM                 /* SQL-2003-N */
%token  COLUMNS
%token  COLUMN_SYM                    /* SQL-2003-R */
%token  COLUMN_FORMAT_SYM
%token  COLUMN_NAME_SYM               /* SQL-2003-N */
%token  COMMENT_SYM
%token  COMMITTED_SYM                 /* SQL-2003-N */
%token  COMMIT_SYM                    /* SQL-2003-R */
%token  COMPACT_SYM
%token  COMPLETION_SYM
%token  COMPRESSED_SYM
%token  CONCURRENT
%token  CONDITION_SYM                 /* SQL-2003-R, SQL-2008-R */
%token  CONNECTION_SYM
%token  CONSISTENT_SYM
%token  CONSTRAINT                    /* SQL-2003-R */
%token  CONSTRAINT_CATALOG_SYM        /* SQL-2003-N */
%token  CONSTRAINT_NAME_SYM           /* SQL-2003-N */
%token  CONSTRAINT_SCHEMA_SYM         /* SQL-2003-N */
%token  CONTAINS_SYM                  /* SQL-2003-N */
%token  CONTEXT_SYM
%token  CONTINUE_SYM                  /* SQL-2003-R */
%token  CONVERT_SYM                   /* SQL-2003-N */
%token  COUNT_SYM                     /* SQL-2003-N */
%token  CPU_SYM
%token  CREATE                        /* SQL-2003-R */
%token  CROSS                         /* SQL-2003-R */
%token  CUBE_SYM                      /* SQL-2003-R */
%token  CURDATE                       /* MYSQL-FUNC */
%token  CURRENT_SYM                   /* SQL-2003-R */
%token  CURRENT_USER                  /* SQL-2003-R */
%token  CURSOR_SYM                    /* SQL-2003-R */
%token  CURSOR_NAME_SYM               /* SQL-2003-N */
%token  CURTIME                       /* MYSQL-FUNC */
%token  DATABASE
%token  DATABASES
%token  DATAFILE_SYM
%token  DATA_SYM                      /* SQL-2003-N */
%token  DATETIME
%token  DATE_ADD_INTERVAL             /* MYSQL-FUNC */
%token  DATE_SUB_INTERVAL             /* MYSQL-FUNC */
%token  DATE_SYM                      /* SQL-2003-R */
%token  DAY_HOUR_SYM
%token  DAY_MICROSECOND_SYM
%token  DAY_MINUTE_SYM
%token  DAY_SECOND_SYM
%token  DAY_SYM                       /* SQL-2003-R */
%token  DEALLOCATE_SYM                /* SQL-2003-R */
%token  DECIMAL_NUM
%token  DECIMAL_SYM                   /* SQL-2003-R */
%token  DECLARE_SYM                   /* SQL-2003-R */
%token  DEFAULT                       /* SQL-2003-R */
%token  DEFAULT_AUTH_SYM              /* INTERNAL */
%token  DEFINER_SYM
%token  DELAYED_SYM
%token  DELAY_KEY_WRITE_SYM
%token  DELETE_SYM                    /* SQL-2003-R */
%token  DESC                          /* SQL-2003-N */
%token  DESCRIBE                      /* SQL-2003-R */
%token  DES_KEY_FILE
%token  DETERMINISTIC_SYM             /* SQL-2003-R */
%token  DIAGNOSTICS_SYM               /* SQL-2003-N */
%token  DIRECTORY_SYM
%token  DISABLE_SYM
%token  DISCARD
%token  DISK_SYM
%token  DISTINCT                      /* SQL-2003-R */
%token  DIV_SYM
%token  DOUBLE_SYM                    /* SQL-2003-R */
%token  DO_SYM
%token  DROP                          /* SQL-2003-R */
%token  DUAL_SYM
%token  DUMPFILE
%token  DUPLICATE_SYM
%token  DYNAMIC_SYM                   /* SQL-2003-R */
%token  EACH_SYM                      /* SQL-2003-R */
%token  ELSE                          /* SQL-2003-R */
%token  ELSEIF_SYM
%token  ENABLE_SYM
%token  ENCLOSED
%token  END                           /* SQL-2003-R */
%token  ENDS_SYM
%token  END_OF_INPUT                  /* INTERNAL */
%token  ENGINES_SYM
%token  ENGINE_SYM
%token  ENUM
%token  EQ                            /* OPERATOR */
%token  EQUAL_SYM                     /* OPERATOR */
%token  ERROR_SYM
%token  ERRORS
%token  ESCAPED
%token  ESCAPE_SYM                    /* SQL-2003-R */
%token  EVENTS_SYM
%token  EVENT_SYM
%token  EVERY_SYM                     /* SQL-2003-N */
%token  EXCHANGE_SYM
%token  EXECUTE_SYM                   /* SQL-2003-R */
%token  EXISTS                        /* SQL-2003-R */
%token  EXIT_SYM
%token  EXPANSION_SYM
%token  EXPIRE_SYM
%token  EXPORT_SYM
%token  EXTENDED_SYM
%token  EXTENT_SIZE_SYM
%token  EXTRACT_SYM                   /* SQL-2003-N */
%token  FALSE_SYM                     /* SQL-2003-R */
%token  FAST_SYM
%token  FAULTS_SYM
%token  FETCH_SYM                     /* SQL-2003-R */
%token  FILE_SYM
%token  FIRST_SYM                     /* SQL-2003-N */
%token  FIXED_SYM
%token  FLOAT_NUM
%token  FLOAT_SYM                     /* SQL-2003-R */
%token  FLUSH_SYM
%token  FORCE_SYM
%token  FOREIGN                       /* SQL-2003-R */
%token  FOR_SYM                       /* SQL-2003-R */
%token  FORMAT_SYM
%token  FOUND_SYM                     /* SQL-2003-R */
%token  FROM
%token  FULL                          /* SQL-2003-R */
%token  FULLTEXT_SYM
%token  FUNCTION_SYM                  /* SQL-2003-R */
%token  GE
%token  GENERAL
%token  GEOMETRYCOLLECTION
%token  GEOMETRY_SYM
%token  GET_FORMAT                    /* MYSQL-FUNC */
%token  GET_SYM                       /* SQL-2003-R */
%token  GLOBAL_SYM                    /* SQL-2003-R */
%token  GRANT                         /* SQL-2003-R */
%token  GRANTS
%token  GROUP_SYM                     /* SQL-2003-R */
%token  GROUP_CONCAT_SYM
%token  GT_SYM                        /* OPERATOR */
%token  HANDLER_SYM
%token  HASH_SYM
%token  HAVING                        /* SQL-2003-R */
%token  HELP_SYM
%token  HEX_NUM
%token  HIGH_PRIORITY
%token  HOST_SYM
%token  HOSTS_SYM
%token  HOUR_MICROSECOND_SYM
%token  HOUR_MINUTE_SYM
%token  HOUR_SECOND_SYM
%token  HOUR_SYM                      /* SQL-2003-R */
%token  IDENT
%token  IDENTIFIED_SYM
%token  IDENT_QUOTED
%token  IF
%token  IGNORE_SYM
%token  IGNORE_SERVER_IDS_SYM
%token  IMPORT
%token  INDEXES
%token  INDEX_SYM
%token  INFILE
%token  INITIAL_SIZE_SYM
%token  INNER_SYM                     /* SQL-2003-R */
%token  INOUT_SYM                     /* SQL-2003-R */
%token  INSENSITIVE_SYM               /* SQL-2003-R */
%token  INSERT                        /* SQL-2003-R */
%token  INSERT_METHOD
%token  INSTALL_SYM
%token  INTERVAL_SYM                  /* SQL-2003-R */
%token  INTO                          /* SQL-2003-R */
%token  INT_SYM                       /* SQL-2003-R */
%token  INVOKER_SYM
%token  IN_SYM                        /* SQL-2003-R */
%token  IO_AFTER_GTIDS                /* MYSQL, FUTURE-USE */
%token  IO_BEFORE_GTIDS               /* MYSQL, FUTURE-USE */
%token  IO_SYM
%token  IPC_SYM
%token  IS                            /* SQL-2003-R */
%token  ISOLATION                     /* SQL-2003-R */
%token  ISSUER_SYM
%token  ITERATE_SYM
%token  JOIN_SYM                      /* SQL-2003-R */
%token  KEYS
%token  KEY_BLOCK_SIZE
%token  KEY_SYM                       /* SQL-2003-N */
%token  KILL_SYM
%token  LANGUAGE_SYM                  /* SQL-2003-R */
%token  LAST_SYM                      /* SQL-2003-N */
%token  LE                            /* OPERATOR */
%token  LEADING                       /* SQL-2003-R */
%token  LEAVES
%token  LEAVE_SYM
%token  LEFT                          /* SQL-2003-R */
%token  LESS_SYM
%token  LEVEL_SYM
%token  LEX_HOSTNAME
%token  LIKE                          /* SQL-2003-R */
%token  LIMIT
%token  LINEAR_SYM
%token  LINES
%token  LINESTRING
%token  LIST_SYM
%token  LOAD
%token  LOCAL_SYM                     /* SQL-2003-R */
%token  LOCATOR_SYM                   /* SQL-2003-N */
%token  LOCKS_SYM
%token  LOCK_SYM
%token  LOGFILE_SYM
%token  LOGS_SYM
%token  LONGBLOB
%token  LONGTEXT
%token  LONG_NUM
%token  LONG_SYM
%token  LOOP_SYM
%token  LOW_PRIORITY
%token  LT                            /* OPERATOR */
%token  MASTER_AUTO_POSITION_SYM
%token  MASTER_BIND_SYM
%token  MASTER_CONNECT_RETRY_SYM
%token  MASTER_DELAY_SYM
%token  MASTER_HOST_SYM
%token  MASTER_LOG_FILE_SYM
%token  MASTER_LOG_POS_SYM
%token  MASTER_PASSWORD_SYM
%token  MASTER_PORT_SYM
%token  MASTER_RETRY_COUNT_SYM
%token  MASTER_SERVER_ID_SYM
%token  MASTER_SSL_CAPATH_SYM
%token  MASTER_SSL_CA_SYM
%token  MASTER_SSL_CERT_SYM
%token  MASTER_SSL_CIPHER_SYM
%token  MASTER_SSL_CRL_SYM
%token  MASTER_SSL_CRLPATH_SYM
%token  MASTER_SSL_KEY_SYM
%token  MASTER_SSL_SYM
%token  MASTER_SSL_VERIFY_SERVER_CERT_SYM
%token  MASTER_SYM
%token  MASTER_USER_SYM
%token  MASTER_HEARTBEAT_PERIOD_SYM
%token  MATCH                         /* SQL-2003-R */
%token  MAX_CONNECTIONS_PER_HOUR
%token  MAX_QUERIES_PER_HOUR
%token  MAX_ROWS
%token  MAX_SIZE_SYM
%token  MAX_SYM                       /* SQL-2003-N */
%token  MAX_UPDATES_PER_HOUR
%token  MAX_USER_CONNECTIONS_SYM
%token  MAX_VALUE_SYM                 /* SQL-2003-N */
%token  MEDIUMBLOB
%token  MEDIUMINT
%token  MEDIUMTEXT
%token  MEDIUM_SYM
%token  MEMORY_SYM
%token  MERGE_SYM                     /* SQL-2003-R */
%token  MESSAGE_TEXT_SYM              /* SQL-2003-N */
%token  MICROSECOND_SYM               /* MYSQL-FUNC */
%token  MIGRATE_SYM
%token  MINUTE_MICROSECOND_SYM
%token  MINUTE_SECOND_SYM
%token  MINUTE_SYM                    /* SQL-2003-R */
%token  MIN_ROWS
%token  MIN_SYM                       /* SQL-2003-N */
%token  MODE_SYM
%token  MODIFIES_SYM                  /* SQL-2003-R */
%token  MODIFY_SYM
%token  MOD_SYM                       /* SQL-2003-N */
%token  MONTH_SYM                     /* SQL-2003-R */
%token  MULTILINESTRING
%token  MULTIPOINT
%token  MULTIPOLYGON
%token  MUTEX_SYM
%token  MYSQL_ERRNO_SYM
%token  NAMES_SYM                     /* SQL-2003-N */
%token  NAME_SYM                      /* SQL-2003-N */
%token  NATIONAL_SYM                  /* SQL-2003-R */
%token  NATURAL                       /* SQL-2003-R */
%token  NCHAR_STRING
%token  NCHAR_SYM                     /* SQL-2003-R */
%token  NDBCLUSTER_SYM
%token  NE                            /* OPERATOR */
%token  NEG
%token  NEW_SYM                       /* SQL-2003-R */
%token  NEXT_SYM                      /* SQL-2003-N */
%token  NODEGROUP_SYM
%token  NONE_SYM                      /* SQL-2003-R */
%token  NOT2_SYM
%token  NOT_SYM                       /* SQL-2003-R */
%token  NOW_SYM
%token  NO_SYM                        /* SQL-2003-R */
%token  NO_WAIT_SYM
%token  NO_WRITE_TO_BINLOG
%token  NULL_SYM                      /* SQL-2003-R */
%token  NUM
%token  NUMBER_SYM                    /* SQL-2003-N */
%token  NUMERIC_SYM                   /* SQL-2003-R */
%token  NVARCHAR_SYM
%token  OFFSET_SYM
%token  OLD_PASSWORD
%token  ON                            /* SQL-2003-R */
%token  ONE_SYM
%token  ONLY_SYM                      /* SQL-2003-R */
%token  OPEN_SYM                      /* SQL-2003-R */
%token  OPTIMIZE
%token  OPTIONS_SYM
%token  OPTION                        /* SQL-2003-N */
%token  OPTIONALLY
%token  OR2_SYM
%token  ORDER_SYM                     /* SQL-2003-R */
%token  OR_OR_SYM                     /* OPERATOR */
%token  OR_SYM                        /* SQL-2003-R */
%token  OUTER
%token  OUTFILE
%token  OUT_SYM                       /* SQL-2003-R */
%token  OWNER_SYM
%token  PACK_KEYS_SYM
%token  PAGE_SYM
%token  PARAM_MARKER
%token  PARSER_SYM
%token  PARTIAL                       /* SQL-2003-N */
%token  PARTITION_SYM                 /* SQL-2003-R */
%token  PARTITIONS_SYM
%token  PARTITIONING_SYM
%token  PASSWORD
%token  PHASE_SYM
%token  PLUGIN_DIR_SYM                /* INTERNAL */
%token  PLUGIN_SYM
%token  PLUGINS_SYM
%token  POINT_SYM
%token  POLYGON
%token  PORT_SYM
%token  POSITION_SYM                  /* SQL-2003-N */
%token  PRECISION                     /* SQL-2003-R */
%token  PREPARE_SYM                   /* SQL-2003-R */
%token  PRESERVE_SYM
%token  PREV_SYM
%token  PRIMARY_SYM                   /* SQL-2003-R */
%token  PRIVILEGES                    /* SQL-2003-N */
%token  PROCEDURE_SYM                 /* SQL-2003-R */
%token  PROCESS
%token  PROCESSLIST_SYM
%token  PROFILE_SYM
%token  PROFILES_SYM
%token  PROXY_SYM
%token  PURGE
%token  QUARTER_SYM
%token  QUERY_SYM
%token  QUICK
%token  RANGE_SYM                     /* SQL-2003-R */
%token  READS_SYM                     /* SQL-2003-R */
%token  READ_ONLY_SYM
%token  READ_SYM                      /* SQL-2003-N */
%token  READ_WRITE_SYM
%token  REAL                          /* SQL-2003-R */
%token  REBUILD_SYM
%token  RECOVER_SYM
%token  REDOFILE_SYM
%token  REDO_BUFFER_SIZE_SYM
%token  REDUNDANT_SYM
%token  REFERENCES                    /* SQL-2003-R */
%token  REGEXP
%token  RELAY
%token  RELAYLOG_SYM
%token  RELAY_LOG_FILE_SYM
%token  RELAY_LOG_POS_SYM
%token  RELAY_THREAD
%token  RELEASE_SYM                   /* SQL-2003-R */
%token  RELOAD
%token  REMOVE_SYM
%token  RENAME
%token  REORGANIZE_SYM
%token  REPAIR
%token  REPEATABLE_SYM                /* SQL-2003-N */
%token  REPEAT_SYM                    /* MYSQL-FUNC */
%token  REPLACE                       /* MYSQL-FUNC */
%token  REPLICATION
%token  REQUIRE_SYM
%token  RESET_SYM
%token  RESIGNAL_SYM                  /* SQL-2003-R */
%token  RESOURCES
%token  RESTORE_SYM
%token  RESTRICT
%token  RESUME_SYM
%token  RETURNED_SQLSTATE_SYM         /* SQL-2003-N */
%token  RETURNS_SYM                   /* SQL-2003-R */
%token  RETURN_SYM                    /* SQL-2003-R */
%token  REVERSE_SYM
%token  REVOKE                        /* SQL-2003-R */
%token  RIGHT                         /* SQL-2003-R */
%token  ROLLBACK_SYM                  /* SQL-2003-R */
%token  ROLLUP_SYM                    /* SQL-2003-R */
%token  ROUTINE_SYM                   /* SQL-2003-N */
%token  ROWS_SYM                      /* SQL-2003-R */
%token  ROW_FORMAT_SYM
%token  ROW_SYM                       /* SQL-2003-R */
%token  ROW_COUNT_SYM                 /* SQL-2003-N */
%token  RTREE_SYM
%token  SAVEPOINT_SYM                 /* SQL-2003-R */
%token  SCHEDULE_SYM
%token  SCHEMA_NAME_SYM               /* SQL-2003-N */
%token  SECOND_MICROSECOND_SYM
%token  SECOND_SYM                    /* SQL-2003-R */
%token  SECURITY_SYM                  /* SQL-2003-N */
%token  SELECT_SYM                    /* SQL-2003-R */
%token  SENSITIVE_SYM                 /* FUTURE-USE */
%token  SEPARATOR_SYM
%token  SERIALIZABLE_SYM              /* SQL-2003-N */
%token  SERIAL_SYM
%token  SESSION_SYM                   /* SQL-2003-N */
%token  SERVER_SYM
%token  SERVER_OPTIONS
%token  SET                           /* SQL-2003-R */
%token  SET_VAR
%token  SHARE_SYM
%token  SHIFT_LEFT                    /* OPERATOR */
%token  SHIFT_RIGHT                   /* OPERATOR */
%token  SHOW
%token  SHUTDOWN
%token  SIGNAL_SYM                    /* SQL-2003-R */
%token  SIGNED_SYM
%token  SIMPLE_SYM                    /* SQL-2003-N */
%token  SLAVE
%token  SLOW
%token  SMALLINT                      /* SQL-2003-R */
%token  SNAPSHOT_SYM
%token  SOCKET_SYM
%token  SONAME_SYM
%token  SOUNDS_SYM
%token  SOURCE_SYM
%token  SPATIAL_SYM
%token  SPECIFIC_SYM                  /* SQL-2003-R */
%token  SQLEXCEPTION_SYM              /* SQL-2003-R */
%token  SQLSTATE_SYM                  /* SQL-2003-R */
%token  SQLWARNING_SYM                /* SQL-2003-R */
%token  SQL_AFTER_GTIDS               /* MYSQL */
%token  SQL_AFTER_MTS_GAPS            /* MYSQL */
%token  SQL_BEFORE_GTIDS              /* MYSQL */
%token  SQL_BIG_RESULT
%token  SQL_BUFFER_RESULT
%token  SQL_CACHE_SYM
%token  SQL_CALC_FOUND_ROWS
%token  SQL_NO_CACHE_SYM
%token  SQL_SMALL_RESULT
%token  SQL_SYM                       /* SQL-2003-R */
%token  SQL_THREAD
%token  SSL_SYM
%token  STARTING
%token  STARTS_SYM
%token  START_SYM                     /* SQL-2003-R */
%token  STATS_AUTO_RECALC_SYM
%token  STATS_PERSISTENT_SYM
%token  STATS_SAMPLE_PAGES_SYM
%token  STATUS_SYM
%token  STDDEV_SAMP_SYM               /* SQL-2003-N */
%token  STD_SYM
%token  STOP_SYM
%token  STORAGE_SYM
%token  STRAIGHT_JOIN
%token  STRING_SYM
%token  SUBCLASS_ORIGIN_SYM           /* SQL-2003-N */
%token  SUBDATE_SYM
%token  SUBJECT_SYM
%token  SUBPARTITIONS_SYM
%token  SUBPARTITION_SYM
%token  SUBSTRING                     /* SQL-2003-N */
%token  SUM_SYM                       /* SQL-2003-N */
%token  SUPER_SYM
%token  SUSPEND_SYM
%token  SWAPS_SYM
%token  SWITCHES_SYM
%token  SYSDATE
%token  TABLES
%token  TABLESPACE
%token  TABLE_REF_PRIORITY
%token  TABLE_SYM                     /* SQL-2003-R */
%token  TABLE_CHECKSUM_SYM
%token  TABLE_NAME_SYM                /* SQL-2003-N */
%token  TEMPORARY                     /* SQL-2003-N */
%token  TEMPTABLE_SYM
%token  TERMINATED
%token  TEXT_STRING
%token  TEXT_SYM
%token  THAN_SYM
%token  THEN_SYM                      /* SQL-2003-R */
%token  TIMESTAMP                     /* SQL-2003-R */
%token  TIMESTAMP_ADD
%token  TIMESTAMP_DIFF
%token  TIME_SYM                      /* SQL-2003-R */
%token  TINYBLOB
%token  TINYINT
%token  TINYTEXT
%token  TO_SYM                        /* SQL-2003-R */
%token  TRAILING                      /* SQL-2003-R */
%token  TRANSACTION_SYM
%token  TRIGGERS_SYM
%token  TRIGGER_SYM                   /* SQL-2003-R */
%token  TRIM                          /* SQL-2003-N */
%token  TRUE_SYM                      /* SQL-2003-R */
%token  TRUNCATE_SYM
%token  TYPES_SYM
%token  TYPE_SYM                      /* SQL-2003-N */
%token  UDF_RETURNS_SYM
%token  ULONGLONG_NUM
%token  UNCOMMITTED_SYM               /* SQL-2003-N */
%token  UNDEFINED_SYM
%token  UNDERSCORE_CHARSET
%token  UNDOFILE_SYM
%token  UNDO_BUFFER_SIZE_SYM
%token  UNDO_SYM                      /* FUTURE-USE */
%token  UNICODE_SYM
%token  UNINSTALL_SYM
%token  UNION_SYM                     /* SQL-2003-R */
%token  UNIQUE_SYM
%token  UNKNOWN_SYM                   /* SQL-2003-R */
%token  UNLOCK_SYM
%token  UNSIGNED
%token  UNTIL_SYM
%token  UPDATE_SYM                    /* SQL-2003-R */
%token  UPGRADE_SYM
%token  USAGE                         /* SQL-2003-N */
%token  USER                          /* SQL-2003-R */
%token  USE_FRM
%token  USE_SYM
%token  USING                         /* SQL-2003-R */
%token  UTC_DATE_SYM
%token  UTC_TIMESTAMP_SYM
%token  UTC_TIME_SYM
%token  VALUES                        /* SQL-2003-R */
%token  VALUE_SYM                     /* SQL-2003-R */
%token  VARBINARY
%token  VARCHAR                       /* SQL-2003-R */
%token  VARIABLES
%token  VARIANCE_SYM
%token  VARYING                       /* SQL-2003-R */
%token  VAR_SAMP_SYM
%token  VIEW_SYM                      /* SQL-2003-N */
%token  WAIT_SYM
%token  WARNINGS
%token  WEEK_SYM
%token  WEIGHT_STRING_SYM
%token  WHEN_SYM                      /* SQL-2003-R */
%token  WHERE                         /* SQL-2003-R */
%token  WHILE_SYM
%token  WITH                          /* SQL-2003-R */
%token  WITH_CUBE_SYM                 /* INTERNAL */
%token  WITH_ROLLUP_SYM               /* INTERNAL */
%token  WORK_SYM                      /* SQL-2003-N */
%token  WRAPPER_SYM
%token  WRITE_SYM                     /* SQL-2003-N */
%token  X509_SYM
%token  XA_SYM
%token  XML_SYM
%token  XOR
%token  YEAR_MONTH_SYM
%token  YEAR_SYM                      /* SQL-2003-R */
%token  ZEROFILL

%left   JOIN_SYM INNER_SYM STRAIGHT_JOIN CROSS LEFT RIGHT
/* A dummy token to force the priority of table_ref production in a join. */
%left   TABLE_REF_PRIORITY
%left   SET_VAR
%left   OR_OR_SYM OR_SYM OR2_SYM
%left   XOR
%left   AND_SYM AND_AND_SYM
%left   BETWEEN_SYM CASE_SYM WHEN_SYM THEN_SYM ELSE
%left   EQ EQUAL_SYM GE GT_SYM LE LT NE IS LIKE REGEXP IN_SYM
%left   '|'
%left   '&'
%left   SHIFT_LEFT SHIFT_RIGHT
%left   '-' '+'
%left   '*' '/' '%' DIV_SYM MOD_SYM
%left   '^'
%left   NEG '~'
%right  NOT_SYM NOT2_SYM
%right  BINARY COLLATE_SYM
%left  INTERVAL_SYM

%%

query:
      END_OF_INPUT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= NULL;
          mysql_parser::SqlAstStatics::tree(static_cast<mysql_parser::SqlAstNode*>($$));
        }
      }
    | verb_clause ';' opt_end_of_input
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= $1;
          mysql_parser::SqlAstStatics::tree(static_cast<mysql_parser::SqlAstNode*>($$));
        }
      }
    | verb_clause END_OF_INPUT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= $1;
          mysql_parser::SqlAstStatics::tree(static_cast<mysql_parser::SqlAstNode*>($$));
        }
      }
    ;

opt_end_of_input:
      /* empty */
      {
          $$= NULL;
      }
    | END_OF_INPUT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_end_of_input);
        }
      }
    ;

verb_clause:
      statement
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_verb_clause);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | begin
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_verb_clause);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

statement:
      alter
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | analyze
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | binlog_base64_event
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | call
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | change
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | check
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | checksum
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | commit
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | create
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | deallocate
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | delete
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | describe
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | do
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | drop
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | execute
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | flush
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | get_diagnostics
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | grant
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | handler
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | help
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | insert
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | install
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | kill
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | load
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | lock
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | optimize
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | keycache
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | partition_entry
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | preload
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | prepare
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | purge
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | release
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | rename
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | repair
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | replace
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | reset
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | resignal_stmt
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | revoke
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | rollback
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | savepoint
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | select
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | set
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | signal_stmt
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | show
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | slave
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | start
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | truncate
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | uninstall
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | unlock
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | update
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | use
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | xa
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

deallocate:
      deallocate_or_drop PREPARE_SYM ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_deallocate);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_PREPARE_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

deallocate_or_drop:
      DEALLOCATE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_deallocate_or_drop);
        }
      }
    | DROP
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_deallocate_or_drop);
        }
      }
    ;

prepare:
      PREPARE_SYM ident FROM prepare_src
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_prepare);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PREPARE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_FROM));
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

prepare_src:
      TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_prepare_src);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | '@' ident_or_text
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_prepare_src);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_64));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

execute:
      EXECUTE_SYM ident execute_using
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_execute);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_EXECUTE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

execute_using:
      /* empty */
      {
          $$= NULL;
      }
    | USING execute_var_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_execute_using);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_USING));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

execute_var_list:
      execute_var_list ',' execute_var_ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_execute_var_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | execute_var_ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_execute_var_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

execute_var_ident:
      '@' ident_or_text
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_execute_var_ident);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_64));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

help:
      HELP_SYM ident_or_text
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_help);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_HELP_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

change:
      CHANGE MASTER_SYM TO_SYM master_defs
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_change);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CHANGE));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_MASTER_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_TO_SYM));
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

master_defs:
      master_def
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_master_defs);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | master_defs ',' master_def
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_master_defs);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

master_def:
      MASTER_HOST_SYM EQ TEXT_STRING_sys_nonewline
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_master_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MASTER_HOST_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | MASTER_BIND_SYM EQ TEXT_STRING_sys_nonewline
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_master_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MASTER_BIND_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | MASTER_USER_SYM EQ TEXT_STRING_sys_nonewline
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_master_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MASTER_USER_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | MASTER_PASSWORD_SYM EQ TEXT_STRING_sys_nonewline
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_master_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MASTER_PASSWORD_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | MASTER_PORT_SYM EQ ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_master_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MASTER_PORT_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | MASTER_CONNECT_RETRY_SYM EQ ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_master_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MASTER_CONNECT_RETRY_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | MASTER_RETRY_COUNT_SYM EQ ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_master_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MASTER_RETRY_COUNT_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | MASTER_DELAY_SYM EQ ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_master_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MASTER_DELAY_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | MASTER_SSL_SYM EQ ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_master_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MASTER_SSL_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | MASTER_SSL_CA_SYM EQ TEXT_STRING_sys_nonewline
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_master_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MASTER_SSL_CA_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | MASTER_SSL_CAPATH_SYM EQ TEXT_STRING_sys_nonewline
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_master_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MASTER_SSL_CAPATH_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | MASTER_SSL_CERT_SYM EQ TEXT_STRING_sys_nonewline
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_master_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MASTER_SSL_CERT_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | MASTER_SSL_CIPHER_SYM EQ TEXT_STRING_sys_nonewline
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_master_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MASTER_SSL_CIPHER_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | MASTER_SSL_KEY_SYM EQ TEXT_STRING_sys_nonewline
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_master_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MASTER_SSL_KEY_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | MASTER_SSL_VERIFY_SERVER_CERT_SYM EQ ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_master_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MASTER_SSL_VERIFY_SERVER_CERT_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | MASTER_SSL_CRL_SYM EQ TEXT_STRING_sys_nonewline
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_master_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MASTER_SSL_CRL_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | MASTER_SSL_CRLPATH_SYM EQ TEXT_STRING_sys_nonewline
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_master_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MASTER_SSL_CRLPATH_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | MASTER_HEARTBEAT_PERIOD_SYM EQ NUM_literal
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_master_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MASTER_HEARTBEAT_PERIOD_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | IGNORE_SERVER_IDS_SYM EQ '(' ignore_server_id_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_master_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_IGNORE_SERVER_IDS_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_40));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_41));
        }
      }
    | MASTER_AUTO_POSITION_SYM EQ ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_master_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MASTER_AUTO_POSITION_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | master_file_def
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_master_def);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

ignore_server_id_list:
      /* empty */
      {
          $$= NULL;
      }
    | ignore_server_id
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ignore_server_id_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | ignore_server_id_list ',' ignore_server_id
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_ignore_server_id_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

ignore_server_id:
      ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_ignore_server_id);
        }
      }
    ;

master_file_def:
      MASTER_LOG_FILE_SYM EQ TEXT_STRING_sys_nonewline
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_master_file_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MASTER_LOG_FILE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | MASTER_LOG_POS_SYM EQ ulonglong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_master_file_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MASTER_LOG_POS_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | RELAY_LOG_FILE_SYM EQ TEXT_STRING_sys_nonewline
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_master_file_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_RELAY_LOG_FILE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | RELAY_LOG_POS_SYM EQ ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_master_file_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_RELAY_LOG_POS_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

create:
      CREATE opt_table_options TABLE_SYM opt_if_not_exists table_ident create2
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CREATE));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_TABLE_SYM));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, $6);
        }
      }
    | CREATE opt_unique INDEX_SYM ident key_alg ON table_ident '(' key_list ')' normal_key_options opt_index_lock_algorithm
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CREATE));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_INDEX_SYM));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_ON));
          mysql_parser::add_ast_child_node($$, $7);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($8, sql::_40));
          mysql_parser::add_ast_child_node($$, $9);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($10, sql::_41));
          mysql_parser::add_ast_child_node($$, $11);
          mysql_parser::add_ast_child_node($$, $12);
        }
      }
    | CREATE fulltext INDEX_SYM ident init_key_options ON table_ident '(' key_list ')' fulltext_key_options opt_index_lock_algorithm
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CREATE));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_INDEX_SYM));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_ON));
          mysql_parser::add_ast_child_node($$, $7);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($8, sql::_40));
          mysql_parser::add_ast_child_node($$, $9);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($10, sql::_41));
          mysql_parser::add_ast_child_node($$, $11);
          mysql_parser::add_ast_child_node($$, $12);
        }
      }
    | CREATE spatial INDEX_SYM ident init_key_options ON table_ident '(' key_list ')' spatial_key_options opt_index_lock_algorithm
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CREATE));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_INDEX_SYM));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_ON));
          mysql_parser::add_ast_child_node($$, $7);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($8, sql::_40));
          mysql_parser::add_ast_child_node($$, $9);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($10, sql::_41));
          mysql_parser::add_ast_child_node($$, $11);
          mysql_parser::add_ast_child_node($$, $12);
        }
      }
    | CREATE DATABASE opt_if_not_exists ident opt_create_database_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CREATE));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_DATABASE));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    | CREATE view_or_trigger_or_sp_or_event
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CREATE));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | CREATE USER clear_privileges grant_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CREATE));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_USER));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | CREATE LOGFILE_SYM GROUP_SYM logfile_group_info
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CREATE));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_LOGFILE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_GROUP_SYM));
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | CREATE TABLESPACE tablespace_info
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CREATE));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_TABLESPACE));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | CREATE server_def
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CREATE));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

server_def:
      SERVER_SYM ident_or_text FOREIGN DATA_SYM WRAPPER_SYM ident_or_text OPTIONS_SYM '(' server_options_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_server_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SERVER_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_FOREIGN));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_DATA_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_WRAPPER_SYM));
          mysql_parser::add_ast_child_node($$, $6);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($7, sql::_OPTIONS_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($8, sql::_40));
          mysql_parser::add_ast_child_node($$, $9);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($10, sql::_41));
        }
      }
    ;

server_options_list:
      server_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_server_options_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | server_options_list ',' server_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_server_options_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

server_option:
      USER TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_server_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_USER));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | HOST_SYM TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_server_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_HOST_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | DATABASE TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_server_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DATABASE));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | OWNER_SYM TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_server_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_OWNER_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | PASSWORD TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_server_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PASSWORD));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | SOCKET_SYM TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_server_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SOCKET_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | PORT_SYM ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_server_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PORT_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

event_tail:
      remember_name EVENT_SYM opt_if_not_exists sp_name ON SCHEDULE_SYM ev_schedule_time opt_ev_on_completion opt_ev_status opt_ev_comment DO_SYM ev_sql_stmt
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_event_tail);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EVENT_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_ON));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_SCHEDULE_SYM));
          mysql_parser::add_ast_child_node($$, $7);
          mysql_parser::add_ast_child_node($$, $8);
          mysql_parser::add_ast_child_node($$, $9);
          mysql_parser::add_ast_child_node($$, $10);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($11, sql::_DO_SYM));
          mysql_parser::add_ast_child_node($$, $12);
        }
      }
    ;

ev_schedule_time:
      EVERY_SYM expr interval ev_starts ev_ends
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ev_schedule_time);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_EVERY_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    | AT_SYM expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ev_schedule_time);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_AT_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

opt_ev_status:
      /* empty */
      {
          $$= NULL;
      }
    | ENABLE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_ev_status);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ENABLE_SYM));
        }
      }
    | DISABLE_SYM ON SLAVE
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_ev_status);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DISABLE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_ON));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_SLAVE));
        }
      }
    | DISABLE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_ev_status);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DISABLE_SYM));
        }
      }
    ;

ev_starts:
      /* empty */
      {
          $$= NULL;
      }
    | STARTS_SYM expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ev_starts);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_STARTS_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

ev_ends:
      /* empty */
      {
          $$= NULL;
      }
    | ENDS_SYM expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ev_ends);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ENDS_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

opt_ev_on_completion:
      /* empty */
      {
          $$= NULL;
      }
    | ev_on_completion
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_ev_on_completion);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

ev_on_completion:
      ON COMPLETION_SYM PRESERVE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ev_on_completion);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ON));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_COMPLETION_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_PRESERVE_SYM));
        }
      }
    | ON COMPLETION_SYM NOT_SYM PRESERVE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ev_on_completion);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ON));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_COMPLETION_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_NOT_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_PRESERVE_SYM));
        }
      }
    ;

opt_ev_comment:
      /* empty */
      {
          $$= NULL;
      }
    | COMMENT_SYM TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_ev_comment);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_COMMENT_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

ev_sql_stmt:
      ev_sql_stmt_inner
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_ev_sql_stmt);
        }
      }
    ;

ev_sql_stmt_inner:
      sp_proc_stmt_statement
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ev_sql_stmt_inner);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sp_proc_stmt_return
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ev_sql_stmt_inner);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sp_proc_stmt_if
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ev_sql_stmt_inner);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | case_stmt_specification
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ev_sql_stmt_inner);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sp_labeled_block
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ev_sql_stmt_inner);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sp_unlabeled_block
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ev_sql_stmt_inner);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sp_labeled_control
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ev_sql_stmt_inner);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sp_proc_stmt_unlabeled
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ev_sql_stmt_inner);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sp_proc_stmt_leave
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ev_sql_stmt_inner);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sp_proc_stmt_iterate
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ev_sql_stmt_inner);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sp_proc_stmt_open
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ev_sql_stmt_inner);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sp_proc_stmt_fetch
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ev_sql_stmt_inner);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sp_proc_stmt_close
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ev_sql_stmt_inner);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

clear_privileges:
      /* empty */
      {
          $$= NULL;
      }
    ;

sp_name:
      ident '.' ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_name);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_46));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_name);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

sp_a_chistics:
      /* empty */
      {
          $$= NULL;
      }
    | sp_a_chistics sp_chistic
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_sp_a_chistics);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

sp_c_chistics:
      /* empty */
      {
          $$= NULL;
      }
    | sp_c_chistics sp_c_chistic
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_sp_c_chistics);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

sp_chistic:
      COMMENT_SYM TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_chistic);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_COMMENT_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | LANGUAGE_SYM SQL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_chistic);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_LANGUAGE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_SQL_SYM));
        }
      }
    | NO_SYM SQL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_chistic);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_NO_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_SQL_SYM));
        }
      }
    | CONTAINS_SYM SQL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_chistic);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CONTAINS_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_SQL_SYM));
        }
      }
    | READS_SYM SQL_SYM DATA_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_chistic);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_READS_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_SQL_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_DATA_SYM));
        }
      }
    | MODIFIES_SYM SQL_SYM DATA_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_chistic);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MODIFIES_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_SQL_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_DATA_SYM));
        }
      }
    | sp_suid
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_chistic);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

sp_c_chistic:
      sp_chistic
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_c_chistic);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | DETERMINISTIC_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_c_chistic);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DETERMINISTIC_SYM));
        }
      }
    | not DETERMINISTIC_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_c_chistic);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_DETERMINISTIC_SYM));
        }
      }
    ;

sp_suid:
      SQL_SYM SECURITY_SYM DEFINER_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_suid);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SQL_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_SECURITY_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_DEFINER_SYM));
        }
      }
    | SQL_SYM SECURITY_SYM INVOKER_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_suid);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SQL_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_SECURITY_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_INVOKER_SYM));
        }
      }
    ;

call:
      CALL_SYM sp_name opt_sp_cparam_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_call);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CALL_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

opt_sp_cparam_list:
      /* empty */
      {
          $$= NULL;
      }
    | '(' opt_sp_cparams ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_sp_cparam_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
        }
      }
    ;

opt_sp_cparams:
      /* empty */
      {
          $$= NULL;
      }
    | sp_cparams
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_sp_cparams);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

sp_cparams:
      sp_cparams ',' expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_sp_cparams);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_cparams);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

sp_fdparam_list:
      /* empty */
      {
          $$= NULL;
      }
    | sp_fdparams
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_fdparam_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

sp_fdparams:
      sp_fdparams ',' sp_fdparam
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_sp_fdparams);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | sp_fdparam
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_fdparams);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

sp_init_param:
      /* empty */
      {
          $$= NULL;
      }
    ;

sp_fdparam:
      ident sp_init_param type_with_opt_collate
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_fdparam);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

sp_pdparam_list:
      /* empty */
      {
          $$= NULL;
      }
    | sp_pdparams
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_pdparam_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

sp_pdparams:
      sp_pdparams ',' sp_pdparam
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_sp_pdparams);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | sp_pdparam
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_pdparams);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

sp_pdparam:
      sp_opt_inout sp_init_param ident type_with_opt_collate
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_pdparam);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

sp_opt_inout:
      /* empty */
      {
          $$= NULL;
      }
    | IN_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_sp_opt_inout);
        }
      }
    | OUT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_sp_opt_inout);
        }
      }
    | INOUT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_sp_opt_inout);
        }
      }
    ;

sp_proc_stmts:
      /* empty */
      {
          $$= NULL;
      }
    | sp_proc_stmts sp_proc_stmt ';'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_sp_proc_stmts);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_59));
        }
      }
    ;

sp_proc_stmts1:
      sp_proc_stmt ';'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_proc_stmts1);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_59));
        }
      }
    | sp_proc_stmts1 sp_proc_stmt ';'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_sp_proc_stmts1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_59));
        }
      }
    ;

sp_decls:
      /* empty */
      {
          $$= NULL;
      }
    | sp_decls sp_decl ';'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_sp_decls);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_59));
        }
      }
    ;

sp_decl:
      DECLARE_SYM sp_decl_idents type_with_opt_collate sp_opt_default
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_decl);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DECLARE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | DECLARE_SYM ident CONDITION_SYM FOR_SYM sp_cond
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_decl);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DECLARE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_CONDITION_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_FOR_SYM));
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    | DECLARE_SYM sp_handler_type HANDLER_SYM FOR_SYM sp_hcond_list sp_proc_stmt
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_decl);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DECLARE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_HANDLER_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_FOR_SYM));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, $6);
        }
      }
    | DECLARE_SYM ident CURSOR_SYM FOR_SYM select
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_decl);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DECLARE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_CURSOR_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_FOR_SYM));
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    ;

sp_handler_type:
      EXIT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_sp_handler_type);
        }
      }
    | CONTINUE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_sp_handler_type);
        }
      }
    ;

sp_hcond_list:
      sp_hcond_element
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_hcond_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sp_hcond_list ',' sp_hcond_element
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_sp_hcond_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

sp_hcond_element:
      sp_hcond
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_hcond_element);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

sp_cond:
      ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_cond);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sqlstate
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_cond);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

sqlstate:
      SQLSTATE_SYM opt_value TEXT_STRING_literal
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sqlstate);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SQLSTATE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

opt_value:
      /* empty */
      {
          $$= NULL;
      }
    | VALUE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_value);
        }
      }
    ;

sp_hcond:
      sp_cond
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_hcond);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_hcond);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | SQLWARNING_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_hcond);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SQLWARNING_SYM));
        }
      }
    | not FOUND_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_hcond);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_FOUND_SYM));
        }
      }
    | SQLEXCEPTION_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_hcond);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SQLEXCEPTION_SYM));
        }
      }
    ;

signal_stmt:
      SIGNAL_SYM signal_value opt_set_signal_information
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_signal_stmt);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SIGNAL_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

signal_value:
      ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_signal_value);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sqlstate
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_signal_value);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

opt_signal_value:
      /* empty */
      {
          $$= NULL;
      }
    | signal_value
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_signal_value);
        }
      }
    ;

opt_set_signal_information:
      /* empty */
      {
          $$= NULL;
      }
    | SET signal_information_item_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_set_signal_information);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SET));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

signal_information_item_list:
      signal_condition_information_item_name EQ signal_allowed_expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_signal_information_item_list);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | signal_information_item_list ',' signal_condition_information_item_name EQ signal_allowed_expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_signal_information_item_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    ;

signal_allowed_expr:
      literal
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_signal_allowed_expr);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | variable
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_signal_allowed_expr);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | simple_ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_signal_allowed_expr);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

signal_condition_information_item_name:
      CLASS_ORIGIN_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_signal_condition_information_item_name);
        }
      }
    | SUBCLASS_ORIGIN_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_signal_condition_information_item_name);
        }
      }
    | CONSTRAINT_CATALOG_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_signal_condition_information_item_name);
        }
      }
    | CONSTRAINT_SCHEMA_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_signal_condition_information_item_name);
        }
      }
    | CONSTRAINT_NAME_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_signal_condition_information_item_name);
        }
      }
    | CATALOG_NAME_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_signal_condition_information_item_name);
        }
      }
    | SCHEMA_NAME_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_signal_condition_information_item_name);
        }
      }
    | TABLE_NAME_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_signal_condition_information_item_name);
        }
      }
    | COLUMN_NAME_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_signal_condition_information_item_name);
        }
      }
    | CURSOR_NAME_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_signal_condition_information_item_name);
        }
      }
    | MESSAGE_TEXT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_signal_condition_information_item_name);
        }
      }
    | MYSQL_ERRNO_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_signal_condition_information_item_name);
        }
      }
    ;

resignal_stmt:
      RESIGNAL_SYM opt_signal_value opt_set_signal_information
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_resignal_stmt);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_RESIGNAL_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

get_diagnostics:
      GET_SYM which_area DIAGNOSTICS_SYM diagnostics_information
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_get_diagnostics);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_GET_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_DIAGNOSTICS_SYM));
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

which_area:
      /* empty */
      {
          $$= NULL;
      }
    | CURRENT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_which_area);
        }
      }
    ;

diagnostics_information:
      statement_information
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_diagnostics_information);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | CONDITION_SYM condition_number condition_information
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_diagnostics_information);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CONDITION_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

statement_information:
      statement_information_item
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement_information);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | statement_information ',' statement_information_item
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_statement_information);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

statement_information_item:
      simple_target_specification EQ statement_information_item_name
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_statement_information_item);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

simple_target_specification:
      ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_target_specification);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | '@' ident_or_text
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_target_specification);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_64));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

statement_information_item_name:
      NUMBER_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_statement_information_item_name);
        }
      }
    | ROW_COUNT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_statement_information_item_name);
        }
      }
    ;

condition_number:
      signal_allowed_expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_condition_number);
        }
      }
    ;

condition_information:
      condition_information_item
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_condition_information);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | condition_information ',' condition_information_item
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_condition_information);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

condition_information_item:
      simple_target_specification EQ condition_information_item_name
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_condition_information_item);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

condition_information_item_name:
      CLASS_ORIGIN_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_condition_information_item_name);
        }
      }
    | SUBCLASS_ORIGIN_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_condition_information_item_name);
        }
      }
    | CONSTRAINT_CATALOG_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_condition_information_item_name);
        }
      }
    | CONSTRAINT_SCHEMA_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_condition_information_item_name);
        }
      }
    | CONSTRAINT_NAME_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_condition_information_item_name);
        }
      }
    | CATALOG_NAME_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_condition_information_item_name);
        }
      }
    | SCHEMA_NAME_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_condition_information_item_name);
        }
      }
    | TABLE_NAME_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_condition_information_item_name);
        }
      }
    | COLUMN_NAME_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_condition_information_item_name);
        }
      }
    | CURSOR_NAME_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_condition_information_item_name);
        }
      }
    | MESSAGE_TEXT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_condition_information_item_name);
        }
      }
    | MYSQL_ERRNO_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_condition_information_item_name);
        }
      }
    | RETURNED_SQLSTATE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_condition_information_item_name);
        }
      }
    ;

sp_decl_idents:
      ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_decl_idents);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sp_decl_idents ',' ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_sp_decl_idents);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

sp_opt_default:
      /* empty */
      {
          $$= NULL;
      }
    | DEFAULT expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_opt_default);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DEFAULT));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

sp_proc_stmt:
      sp_proc_stmt_statement
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_proc_stmt);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sp_proc_stmt_return
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_proc_stmt);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sp_proc_stmt_if
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_proc_stmt);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | case_stmt_specification
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_proc_stmt);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sp_labeled_block
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_proc_stmt);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sp_unlabeled_block
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_proc_stmt);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sp_labeled_control
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_proc_stmt);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sp_proc_stmt_unlabeled
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_proc_stmt);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sp_proc_stmt_leave
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_proc_stmt);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sp_proc_stmt_iterate
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_proc_stmt);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sp_proc_stmt_open
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_proc_stmt);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sp_proc_stmt_fetch
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_proc_stmt);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sp_proc_stmt_close
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_proc_stmt);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

sp_proc_stmt_if:
      IF sp_if END IF
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_proc_stmt_if);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_IF));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_END));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_IF));
        }
      }
    ;

sp_proc_stmt_statement:
      statement
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_sp_proc_stmt_statement);
        }
      }
    ;

sp_proc_stmt_return:
      RETURN_SYM expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_proc_stmt_return);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_RETURN_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

sp_proc_stmt_unlabeled:
      sp_unlabeled_control
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_proc_stmt_unlabeled);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

sp_proc_stmt_leave:
      LEAVE_SYM label_ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_proc_stmt_leave);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_LEAVE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

sp_proc_stmt_iterate:
      ITERATE_SYM label_ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_proc_stmt_iterate);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ITERATE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

sp_proc_stmt_open:
      OPEN_SYM ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_proc_stmt_open);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_OPEN_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

sp_proc_stmt_fetch:
      FETCH_SYM sp_opt_fetch_noise ident INTO sp_fetch_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_proc_stmt_fetch);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_FETCH_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_INTO));
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    ;

sp_proc_stmt_close:
      CLOSE_SYM ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_proc_stmt_close);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CLOSE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

sp_opt_fetch_noise:
      /* empty */
      {
          $$= NULL;
      }
    | NEXT_SYM FROM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_opt_fetch_noise);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_NEXT_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_FROM));
        }
      }
    | FROM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_opt_fetch_noise);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_FROM));
        }
      }
    ;

sp_fetch_list:
      ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_fetch_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sp_fetch_list ',' ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_sp_fetch_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

sp_if:
      expr THEN_SYM sp_proc_stmts1 sp_elseifs
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_if);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_THEN_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

sp_elseifs:
      /* empty */
      {
          $$= NULL;
      }
    | ELSEIF_SYM sp_if
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_elseifs);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ELSEIF_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | ELSE sp_proc_stmts1
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_elseifs);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ELSE));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

case_stmt_specification:
      simple_case_stmt
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_case_stmt_specification);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | searched_case_stmt
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_case_stmt_specification);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

simple_case_stmt:
      CASE_SYM expr simple_when_clause_list else_clause_opt END CASE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_case_stmt);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CASE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_END));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_CASE_SYM));
        }
      }
    ;

searched_case_stmt:
      CASE_SYM searched_when_clause_list else_clause_opt END CASE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_searched_case_stmt);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CASE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_END));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_CASE_SYM));
        }
      }
    ;

simple_when_clause_list:
      simple_when_clause
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_when_clause_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | simple_when_clause_list simple_when_clause
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_simple_when_clause_list);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

searched_when_clause_list:
      searched_when_clause
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_searched_when_clause_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | searched_when_clause_list searched_when_clause
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_searched_when_clause_list);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

simple_when_clause:
      WHEN_SYM expr THEN_SYM sp_proc_stmts1
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_when_clause);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_WHEN_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_THEN_SYM));
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

searched_when_clause:
      WHEN_SYM expr THEN_SYM sp_proc_stmts1
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_searched_when_clause);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_WHEN_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_THEN_SYM));
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

else_clause_opt:
      /* empty */
      {
          $$= NULL;
      }
    | ELSE sp_proc_stmts1
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_else_clause_opt);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ELSE));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

sp_labeled_control:
      label_ident ':' sp_unlabeled_control sp_opt_label
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_labeled_control);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_58));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

sp_opt_label:
      /* empty */
      {
          $$= NULL;
      }
    | label_ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_sp_opt_label);
        }
      }
    ;

sp_labeled_block:
      label_ident ':' sp_block_content sp_opt_label
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_labeled_block);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_58));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

sp_unlabeled_block:
      sp_block_content
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_unlabeled_block);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

sp_block_content:
      BEGIN_SYM sp_decls sp_proc_stmts END
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_block_content);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_BEGIN_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_END));
        }
      }
    ;

sp_unlabeled_control:
      LOOP_SYM sp_proc_stmts1 END LOOP_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_unlabeled_control);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_LOOP_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_END));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_LOOP_SYM));
        }
      }
    | WHILE_SYM expr DO_SYM sp_proc_stmts1 END WHILE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_unlabeled_control);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_WHILE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_DO_SYM));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_END));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_WHILE_SYM));
        }
      }
    | REPEAT_SYM sp_proc_stmts1 UNTIL_SYM expr END REPEAT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_unlabeled_control);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_REPEAT_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_UNTIL_SYM));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_END));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_REPEAT_SYM));
        }
      }
    ;

trg_action_time:
      BEFORE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_trg_action_time);
        }
      }
    | AFTER_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_trg_action_time);
        }
      }
    ;

trg_event:
      INSERT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_trg_event);
        }
      }
    | UPDATE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_trg_event);
        }
      }
    | DELETE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_trg_event);
        }
      }
    ;

change_tablespace_access:
      tablespace_name ts_access_mode
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_change_tablespace_access);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

change_tablespace_info:
      tablespace_name CHANGE ts_datafile change_ts_option_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_change_tablespace_info);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_CHANGE));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

tablespace_info:
      tablespace_name ADD ts_datafile opt_logfile_group_name tablespace_option_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_tablespace_info);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_ADD));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    ;

opt_logfile_group_name:
      /* empty */
      {
          $$= NULL;
      }
    | USE_SYM LOGFILE_SYM GROUP_SYM ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_logfile_group_name);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_USE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_LOGFILE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_GROUP_SYM));
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

alter_tablespace_info:
      tablespace_name ADD ts_datafile alter_tablespace_option_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_tablespace_info);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_ADD));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | tablespace_name DROP ts_datafile alter_tablespace_option_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_tablespace_info);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_DROP));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

logfile_group_info:
      logfile_group_name add_log_file logfile_group_option_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_logfile_group_info);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

alter_logfile_group_info:
      logfile_group_name add_log_file alter_logfile_group_option_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_logfile_group_info);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

add_log_file:
      ADD lg_undofile
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_add_log_file);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ADD));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | ADD lg_redofile
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_add_log_file);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ADD));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

change_ts_option_list:
      change_ts_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_change_ts_option_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

change_ts_options:
      change_ts_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_change_ts_options);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | change_ts_options change_ts_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_change_ts_options);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | change_ts_options ',' change_ts_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_change_ts_options);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

change_ts_option:
      opt_ts_initial_size
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_change_ts_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | opt_ts_autoextend_size
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_change_ts_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | opt_ts_max_size
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_change_ts_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

tablespace_option_list:
      /* empty */
      {
          $$= NULL;
      }
    | tablespace_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_tablespace_option_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

tablespace_options:
      tablespace_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_tablespace_options);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | tablespace_options tablespace_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_tablespace_options);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | tablespace_options ',' tablespace_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_tablespace_options);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

tablespace_option:
      opt_ts_initial_size
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_tablespace_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | opt_ts_autoextend_size
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_tablespace_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | opt_ts_max_size
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_tablespace_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | opt_ts_extent_size
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_tablespace_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | opt_ts_nodegroup
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_tablespace_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | opt_ts_engine
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_tablespace_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | ts_wait
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_tablespace_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | opt_ts_comment
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_tablespace_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

alter_tablespace_option_list:
      /* empty */
      {
          $$= NULL;
      }
    | alter_tablespace_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_tablespace_option_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

alter_tablespace_options:
      alter_tablespace_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_tablespace_options);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | alter_tablespace_options alter_tablespace_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_alter_tablespace_options);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | alter_tablespace_options ',' alter_tablespace_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_alter_tablespace_options);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

alter_tablespace_option:
      opt_ts_initial_size
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_tablespace_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | opt_ts_autoextend_size
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_tablespace_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | opt_ts_max_size
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_tablespace_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | opt_ts_engine
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_tablespace_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | ts_wait
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_tablespace_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

logfile_group_option_list:
      /* empty */
      {
          $$= NULL;
      }
    | logfile_group_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_logfile_group_option_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

logfile_group_options:
      logfile_group_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_logfile_group_options);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | logfile_group_options logfile_group_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_logfile_group_options);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | logfile_group_options ',' logfile_group_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_logfile_group_options);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

logfile_group_option:
      opt_ts_initial_size
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_logfile_group_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | opt_ts_undo_buffer_size
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_logfile_group_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | opt_ts_redo_buffer_size
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_logfile_group_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | opt_ts_nodegroup
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_logfile_group_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | opt_ts_engine
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_logfile_group_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | ts_wait
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_logfile_group_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | opt_ts_comment
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_logfile_group_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

alter_logfile_group_option_list:
      /* empty */
      {
          $$= NULL;
      }
    | alter_logfile_group_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_logfile_group_option_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

alter_logfile_group_options:
      alter_logfile_group_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_logfile_group_options);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | alter_logfile_group_options alter_logfile_group_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_alter_logfile_group_options);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | alter_logfile_group_options ',' alter_logfile_group_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_alter_logfile_group_options);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

alter_logfile_group_option:
      opt_ts_initial_size
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_logfile_group_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | opt_ts_engine
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_logfile_group_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | ts_wait
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_logfile_group_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

ts_datafile:
      DATAFILE_SYM TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ts_datafile);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DATAFILE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

lg_undofile:
      UNDOFILE_SYM TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_lg_undofile);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_UNDOFILE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

lg_redofile:
      REDOFILE_SYM TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_lg_redofile);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_REDOFILE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

tablespace_name:
      ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_tablespace_name);
        }
      }
    ;

logfile_group_name:
      ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_logfile_group_name);
        }
      }
    ;

ts_access_mode:
      READ_ONLY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ts_access_mode);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_READ_ONLY_SYM));
        }
      }
    | READ_WRITE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ts_access_mode);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_READ_WRITE_SYM));
        }
      }
    | NOT_SYM ACCESSIBLE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ts_access_mode);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_NOT_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_ACCESSIBLE_SYM));
        }
      }
    ;

opt_ts_initial_size:
      INITIAL_SIZE_SYM opt_equal size_number
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_ts_initial_size);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_INITIAL_SIZE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

opt_ts_autoextend_size:
      AUTOEXTEND_SIZE_SYM opt_equal size_number
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_ts_autoextend_size);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_AUTOEXTEND_SIZE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

opt_ts_max_size:
      MAX_SIZE_SYM opt_equal size_number
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_ts_max_size);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MAX_SIZE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

opt_ts_extent_size:
      EXTENT_SIZE_SYM opt_equal size_number
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_ts_extent_size);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_EXTENT_SIZE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

opt_ts_undo_buffer_size:
      UNDO_BUFFER_SIZE_SYM opt_equal size_number
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_ts_undo_buffer_size);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_UNDO_BUFFER_SIZE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

opt_ts_redo_buffer_size:
      REDO_BUFFER_SIZE_SYM opt_equal size_number
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_ts_redo_buffer_size);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_REDO_BUFFER_SIZE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

opt_ts_nodegroup:
      NODEGROUP_SYM opt_equal real_ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_ts_nodegroup);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_NODEGROUP_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

opt_ts_comment:
      COMMENT_SYM opt_equal TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_ts_comment);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_COMMENT_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

opt_ts_engine:
      opt_storage ENGINE_SYM opt_equal storage_engines
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_ts_engine);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_ENGINE_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

ts_wait:
      WAIT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_ts_wait);
        }
      }
    | NO_WAIT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_ts_wait);
        }
      }
    ;

size_number:
      real_ulonglong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_size_number);
        }
      }
    | IDENT_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_size_number);
        }
      }
    ;

create2:
      '(' create2a
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | opt_create_table_options opt_create_partitioning create3
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create2);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | LIKE table_ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_LIKE));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | '(' LIKE table_ident ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_LIKE));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    ;

create2a:
      create_field_list ')' opt_create_table_options opt_create_partitioning create3
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create2a);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_41));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    | opt_create_partitioning create_select ')' union_opt
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create2a);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

create3:
      /* empty */
      {
          $$= NULL;
      }
    | opt_duplicate opt_as create_select union_clause
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create3);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | opt_duplicate opt_as '(' create_select ')' union_opt
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create3);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_40));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_41));
          mysql_parser::add_ast_child_node($$, $6);
        }
      }
    ;

opt_create_partitioning:
      opt_partitioning
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_create_partitioning);
        }
      }
    ;

opt_partitioning:
      /* empty */
      {
          $$= NULL;
      }
    | partitioning
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_partitioning);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

partitioning:
      PARTITION_SYM have_partitioning partition
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_partitioning);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PARTITION_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

have_partitioning:
      /* empty */
      {
          $$= NULL;
      }
    ;

partition_entry:
      PARTITION_SYM partition
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_partition_entry);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PARTITION_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

partition:
      BY part_type_def opt_num_parts opt_sub_part part_defs
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_partition);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_BY));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    ;

part_type_def:
      opt_linear KEY_SYM opt_key_algo '(' part_field_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_part_type_def);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_KEY_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_40));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | opt_linear HASH_SYM part_func
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_part_type_def);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_HASH_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | RANGE_SYM part_func
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_part_type_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_RANGE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | RANGE_SYM part_column_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_part_type_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_RANGE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | LIST_SYM part_func
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_part_type_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_LIST_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | LIST_SYM part_column_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_part_type_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_LIST_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

opt_linear:
      /* empty */
      {
          $$= NULL;
      }
    | LINEAR_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_linear);
        }
      }
    ;

opt_key_algo:
      /* empty */
      {
          $$= NULL;
      }
    | ALGORITHM_SYM EQ real_ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_key_algo);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ALGORITHM_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

part_field_list:
      /* empty */
      {
          $$= NULL;
      }
    | part_field_item_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_part_field_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

part_field_item_list:
      part_field_item
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_part_field_item_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | part_field_item_list ',' part_field_item
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_part_field_item_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

part_field_item:
      ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_part_field_item);
        }
      }
    ;

part_column_list:
      COLUMNS '(' part_field_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_part_column_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_COLUMNS));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    ;

part_func:
      '(' remember_name part_func_expr remember_end ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_part_func);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_41));
        }
      }
    ;

sub_part_func:
      '(' remember_name part_func_expr remember_end ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sub_part_func);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_41));
        }
      }
    ;

opt_num_parts:
      /* empty */
      {
          $$= NULL;
      }
    | PARTITIONS_SYM real_ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_num_parts);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PARTITIONS_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

opt_sub_part:
      /* empty */
      {
          $$= NULL;
      }
    | SUBPARTITION_SYM BY opt_linear HASH_SYM sub_part_func opt_num_subparts
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_sub_part);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SUBPARTITION_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_BY));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_HASH_SYM));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, $6);
        }
      }
    | SUBPARTITION_SYM BY opt_linear KEY_SYM opt_key_algo '(' sub_part_field_list ')' opt_num_subparts
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_sub_part);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SUBPARTITION_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_BY));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_KEY_SYM));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_40));
          mysql_parser::add_ast_child_node($$, $7);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($8, sql::_41));
          mysql_parser::add_ast_child_node($$, $9);
        }
      }
    ;

sub_part_field_list:
      sub_part_field_item
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sub_part_field_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sub_part_field_list ',' sub_part_field_item
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_sub_part_field_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

sub_part_field_item:
      ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_sub_part_field_item);
        }
      }
    ;

part_func_expr:
      bit_expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_part_func_expr);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

opt_num_subparts:
      /* empty */
      {
          $$= NULL;
      }
    | SUBPARTITIONS_SYM real_ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_num_subparts);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SUBPARTITIONS_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

part_defs:
      /* empty */
      {
          $$= NULL;
      }
    | '(' part_def_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_part_defs);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
        }
      }
    ;

part_def_list:
      part_definition
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_part_def_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | part_def_list ',' part_definition
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_part_def_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

part_definition:
      PARTITION_SYM part_name opt_part_values opt_part_options opt_sub_partition
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_part_definition);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PARTITION_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    ;

part_name:
      ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_part_name);
        }
      }
    ;

opt_part_values:
      /* empty */
      {
          $$= NULL;
      }
    | VALUES LESS_SYM THAN_SYM part_func_max
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_part_values);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_VALUES));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_LESS_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_THAN_SYM));
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | VALUES IN_SYM part_values_in
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_part_values);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_VALUES));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_IN_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

part_func_max:
      MAX_VALUE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_part_func_max);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MAX_VALUE_SYM));
        }
      }
    | part_value_item
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_part_func_max);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

part_values_in:
      part_value_item
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_part_values_in);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | '(' part_value_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_part_values_in);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
        }
      }
    ;

part_value_list:
      part_value_item
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_part_value_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | part_value_list ',' part_value_item
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_part_value_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

part_value_item:
      '(' {} part_value_item_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_part_value_item);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
        }
      }
    ;

part_value_item_list:
      part_value_expr_item
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_part_value_item_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | part_value_item_list ',' part_value_expr_item
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_part_value_item_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

part_value_expr_item:
      MAX_VALUE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_part_value_expr_item);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MAX_VALUE_SYM));
        }
      }
    | bit_expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_part_value_expr_item);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

opt_sub_partition:
      /* empty */
      {
          $$= NULL;
      }
    | '(' sub_part_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_sub_partition);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
        }
      }
    ;

sub_part_list:
      sub_part_definition
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sub_part_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sub_part_list ',' sub_part_definition
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_sub_part_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

sub_part_definition:
      SUBPARTITION_SYM sub_name opt_part_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sub_part_definition);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SUBPARTITION_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

sub_name:
      ident_or_text
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_sub_name);
        }
      }
    ;

opt_part_options:
      /* empty */
      {
          $$= NULL;
      }
    | opt_part_option_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_part_options);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

opt_part_option_list:
      opt_part_option_list opt_part_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_opt_part_option_list);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | opt_part_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_part_option_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

opt_part_option:
      TABLESPACE opt_equal ident_or_text
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_part_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TABLESPACE));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | opt_storage ENGINE_SYM opt_equal storage_engines
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_part_option);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_ENGINE_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | NODEGROUP_SYM opt_equal real_ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_part_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_NODEGROUP_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | MAX_ROWS opt_equal real_ulonglong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_part_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MAX_ROWS));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | MIN_ROWS opt_equal real_ulonglong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_part_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MIN_ROWS));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | DATA_SYM DIRECTORY_SYM opt_equal TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_part_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DATA_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_DIRECTORY_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | INDEX_SYM DIRECTORY_SYM opt_equal TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_part_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_INDEX_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_DIRECTORY_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | COMMENT_SYM opt_equal TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_part_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_COMMENT_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

create_select:
      SELECT_SYM select_options select_item_list opt_select_from
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_select);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SELECT_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

opt_as:
      /* empty */
      {
          $$= NULL;
      }
    | AS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_as);
        }
      }
    ;

opt_create_database_options:
      /* empty */
      {
          $$= NULL;
      }
    | create_database_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_create_database_options);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

create_database_options:
      create_database_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_database_options);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | create_database_options create_database_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_create_database_options);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

create_database_option:
      default_collation
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_database_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | default_charset
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_database_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

opt_table_options:
      /* empty */
      {
          $$= NULL;
      }
    | table_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_table_options);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

table_options:
      table_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_table_options);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | table_option table_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_table_options);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::merge_ast_child_nodes($$, $2);
        }
      }
    ;

table_option:
      TEMPORARY
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_table_option);
        }
      }
    ;

opt_if_not_exists:
      /* empty */
      {
          $$= NULL;
      }
    | IF not EXISTS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_if_not_exists);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_IF));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_EXISTS));
        }
      }
    ;

opt_create_table_options:
      /* empty */
      {
          $$= NULL;
      }
    | create_table_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_create_table_options);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

create_table_options_space_separated:
      create_table_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_options_space_separated);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | create_table_option create_table_options_space_separated
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_options_space_separated);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::merge_ast_child_nodes($$, $2);
        }
      }
    ;

create_table_options:
      create_table_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_options);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | create_table_option create_table_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_options);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::merge_ast_child_nodes($$, $2);
        }
      }
    | create_table_option ',' create_table_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_options);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::merge_ast_child_nodes($$, $3);
        }
      }
    ;

create_table_option:
      ENGINE_SYM opt_equal storage_engines
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ENGINE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | MAX_ROWS opt_equal ulonglong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MAX_ROWS));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | MIN_ROWS opt_equal ulonglong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MIN_ROWS));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | AVG_ROW_LENGTH opt_equal ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_AVG_ROW_LENGTH));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | PASSWORD opt_equal TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PASSWORD));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | COMMENT_SYM opt_equal TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_COMMENT_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | AUTO_INC opt_equal ulonglong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_AUTO_INC));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | PACK_KEYS_SYM opt_equal ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PACK_KEYS_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | PACK_KEYS_SYM opt_equal DEFAULT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PACK_KEYS_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_DEFAULT));
        }
      }
    | STATS_AUTO_RECALC_SYM opt_equal ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_STATS_AUTO_RECALC_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | STATS_AUTO_RECALC_SYM opt_equal DEFAULT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_STATS_AUTO_RECALC_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_DEFAULT));
        }
      }
    | STATS_PERSISTENT_SYM opt_equal ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_STATS_PERSISTENT_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | STATS_PERSISTENT_SYM opt_equal DEFAULT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_STATS_PERSISTENT_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_DEFAULT));
        }
      }
    | STATS_SAMPLE_PAGES_SYM opt_equal ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_STATS_SAMPLE_PAGES_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | STATS_SAMPLE_PAGES_SYM opt_equal DEFAULT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_STATS_SAMPLE_PAGES_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_DEFAULT));
        }
      }
    | CHECKSUM_SYM opt_equal ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CHECKSUM_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | TABLE_CHECKSUM_SYM opt_equal ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TABLE_CHECKSUM_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | DELAY_KEY_WRITE_SYM opt_equal ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DELAY_KEY_WRITE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | ROW_FORMAT_SYM opt_equal row_types
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ROW_FORMAT_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | UNION_SYM opt_equal '(' opt_table_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_UNION_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_40));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_41));
        }
      }
    | default_charset
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | default_collation
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | INSERT_METHOD opt_equal merge_insert_types
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_INSERT_METHOD));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | DATA_SYM DIRECTORY_SYM opt_equal TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DATA_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_DIRECTORY_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | INDEX_SYM DIRECTORY_SYM opt_equal TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_INDEX_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_DIRECTORY_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | TABLESPACE ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TABLESPACE));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | STORAGE_SYM DISK_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_STORAGE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_DISK_SYM));
        }
      }
    | STORAGE_SYM MEMORY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_STORAGE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_MEMORY_SYM));
        }
      }
    | CONNECTION_SYM opt_equal TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CONNECTION_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | KEY_BLOCK_SIZE opt_equal ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_table_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_KEY_BLOCK_SIZE));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

default_charset:
      opt_default charset opt_equal charset_name_or_default
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_default_charset);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

default_collation:
      opt_default COLLATE_SYM opt_equal collation_name_or_default
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_default_collation);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_COLLATE_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

storage_engines:
      ident_or_text
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_storage_engines);
        }
      }
    ;

known_storage_engines:
      ident_or_text
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_known_storage_engines);
        }
      }
    ;

row_types:
      DEFAULT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_row_types);
        }
      }
    | FIXED_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_row_types);
        }
      }
    | DYNAMIC_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_row_types);
        }
      }
    | COMPRESSED_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_row_types);
        }
      }
    | REDUNDANT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_row_types);
        }
      }
    | COMPACT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_row_types);
        }
      }
    ;

merge_insert_types:
      NO_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_merge_insert_types);
        }
      }
    | FIRST_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_merge_insert_types);
        }
      }
    | LAST_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_merge_insert_types);
        }
      }
    ;

opt_select_from:
      opt_limit_clause
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_select_from);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | select_from select_lock_type
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_select_from);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

udf_type:
      STRING_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_udf_type);
        }
      }
    | REAL
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_udf_type);
        }
      }
    | DECIMAL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_udf_type);
        }
      }
    | INT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_udf_type);
        }
      }
    ;

create_field_list:
      field_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_field_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

field_list:
      field_list_item
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_field_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | field_list ',' field_list_item
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_field_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

field_list_item:
      column_def
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_field_list_item);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | key_def
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_field_list_item);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

column_def:
      field_spec opt_check_constraint
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_column_def);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | field_spec references
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_column_def);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

key_def:
      normal_key_type opt_ident key_alg '(' key_list ')' normal_key_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_key_def);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_40));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
          mysql_parser::add_ast_child_node($$, $7);
        }
      }
    | fulltext opt_key_or_index opt_ident init_key_options '(' key_list ')' fulltext_key_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_key_def);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_40));
          mysql_parser::add_ast_child_node($$, $6);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($7, sql::_41));
          mysql_parser::add_ast_child_node($$, $8);
        }
      }
    | spatial opt_key_or_index opt_ident init_key_options '(' key_list ')' spatial_key_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_key_def);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_40));
          mysql_parser::add_ast_child_node($$, $6);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($7, sql::_41));
          mysql_parser::add_ast_child_node($$, $8);
        }
      }
    | opt_constraint constraint_key_type opt_ident key_alg '(' key_list ')' normal_key_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_key_def);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_40));
          mysql_parser::add_ast_child_node($$, $6);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($7, sql::_41));
          mysql_parser::add_ast_child_node($$, $8);
        }
      }
    | opt_constraint FOREIGN KEY_SYM opt_ident '(' key_list ')' references
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_key_def);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_FOREIGN));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_KEY_SYM));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_40));
          mysql_parser::add_ast_child_node($$, $6);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($7, sql::_41));
          mysql_parser::add_ast_child_node($$, $8);
        }
      }
    | opt_constraint check_constraint
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_key_def);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

opt_check_constraint:
      /* empty */
      {
          $$= NULL;
      }
    | check_constraint
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_check_constraint);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

check_constraint:
      CHECK_SYM '(' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_check_constraint);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CHECK_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    ;

opt_constraint:
      /* empty */
      {
          $$= NULL;
      }
    | constraint
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_constraint);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

constraint:
      CONSTRAINT opt_ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_constraint);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CONSTRAINT));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

field_spec:
      field_ident type opt_attribute
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_field_spec);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

type:
      int_type opt_field_length field_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | real_type opt_precision field_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | FLOAT_SYM float_options field_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_FLOAT_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | BIT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_BIT_SYM));
        }
      }
    | BIT_SYM field_length
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_BIT_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | BOOL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_BOOL_SYM));
        }
      }
    | BOOLEAN_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_BOOLEAN_SYM));
        }
      }
    | char field_length opt_binary
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | char opt_binary
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | nchar field_length opt_bin_mod
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | nchar opt_bin_mod
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | BINARY field_length
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_BINARY));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | BINARY
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_BINARY));
        }
      }
    | varchar field_length opt_binary
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | nvarchar field_length opt_bin_mod
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | VARBINARY field_length
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_VARBINARY));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | YEAR_SYM opt_field_length field_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_YEAR_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | DATE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DATE_SYM));
        }
      }
    | TIME_SYM type_datetime_precision
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TIME_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | TIMESTAMP type_datetime_precision
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TIMESTAMP));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | DATETIME type_datetime_precision
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DATETIME));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | TINYBLOB
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TINYBLOB));
        }
      }
    | BLOB_SYM opt_field_length
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_BLOB_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | spatial_type
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | MEDIUMBLOB
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MEDIUMBLOB));
        }
      }
    | LONGBLOB
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_LONGBLOB));
        }
      }
    | LONG_SYM VARBINARY
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_LONG_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_VARBINARY));
        }
      }
    | LONG_SYM varchar opt_binary
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_LONG_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | TINYTEXT opt_binary
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TINYTEXT));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | TEXT_SYM opt_field_length opt_binary
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TEXT_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | MEDIUMTEXT opt_binary
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MEDIUMTEXT));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | LONGTEXT opt_binary
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_LONGTEXT));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | DECIMAL_SYM float_options field_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DECIMAL_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | NUMERIC_SYM float_options field_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_NUMERIC_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | FIXED_SYM float_options field_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_FIXED_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | ENUM '(' string_list ')' opt_binary
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ENUM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    | SET '(' string_list ')' opt_binary
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SET));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    | LONG_SYM opt_binary
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_LONG_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | SERIAL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SERIAL_SYM));
        }
      }
    ;

spatial_type:
      GEOMETRY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_spatial_type);
        }
      }
    | GEOMETRYCOLLECTION
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_spatial_type);
        }
      }
    | POINT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_spatial_type);
        }
      }
    | MULTIPOINT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_spatial_type);
        }
      }
    | LINESTRING
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_spatial_type);
        }
      }
    | MULTILINESTRING
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_spatial_type);
        }
      }
    | POLYGON
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_spatial_type);
        }
      }
    | MULTIPOLYGON
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_spatial_type);
        }
      }
    ;

char:
      CHAR_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_char);
        }
      }
    ;

nchar:
      NCHAR_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_nchar);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_NCHAR_SYM));
        }
      }
    | NATIONAL_SYM CHAR_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_nchar);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_NATIONAL_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_CHAR_SYM));
        }
      }
    ;

varchar:
      char VARYING
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_varchar);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_VARYING));
        }
      }
    | VARCHAR
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_varchar);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_VARCHAR));
        }
      }
    ;

nvarchar:
      NATIONAL_SYM VARCHAR
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_nvarchar);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_NATIONAL_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_VARCHAR));
        }
      }
    | NVARCHAR_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_nvarchar);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_NVARCHAR_SYM));
        }
      }
    | NCHAR_SYM VARCHAR
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_nvarchar);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_NCHAR_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_VARCHAR));
        }
      }
    | NATIONAL_SYM CHAR_SYM VARYING
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_nvarchar);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_NATIONAL_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_CHAR_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_VARYING));
        }
      }
    | NCHAR_SYM VARYING
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_nvarchar);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_NCHAR_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_VARYING));
        }
      }
    ;

int_type:
      INT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_int_type);
        }
      }
    | TINYINT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_int_type);
        }
      }
    | SMALLINT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_int_type);
        }
      }
    | MEDIUMINT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_int_type);
        }
      }
    | BIGINT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_int_type);
        }
      }
    ;

real_type:
      REAL
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_real_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_REAL));
        }
      }
    | DOUBLE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_real_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DOUBLE_SYM));
        }
      }
    | DOUBLE_SYM PRECISION
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_real_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DOUBLE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_PRECISION));
        }
      }
    ;

float_options:
      /* empty */
      {
          $$= NULL;
      }
    | field_length
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_float_options);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | precision
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_float_options);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

precision:
      '(' NUM ',' NUM ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_precision);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_NUM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_44));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_NUM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_41));
        }
      }
    ;

type_datetime_precision:
      /* empty */
      {
          $$= NULL;
      }
    | '(' NUM ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type_datetime_precision);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_NUM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
        }
      }
    ;

func_datetime_precision:
      /* empty */
      {
          $$= NULL;
      }
    | '(' ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_func_datetime_precision);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_41));
        }
      }
    | '(' NUM ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_func_datetime_precision);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_NUM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
        }
      }
    ;

field_options:
      /* empty */
      {
          $$= NULL;
      }
    | field_opt_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_field_options);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

field_opt_list:
      field_opt_list field_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_field_opt_list);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | field_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_field_opt_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

field_option:
      SIGNED_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_field_option);
        }
      }
    | UNSIGNED
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_field_option);
        }
      }
    | ZEROFILL
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_field_option);
        }
      }
    ;

field_length:
      '(' LONG_NUM ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_field_length);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_LONG_NUM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
        }
      }
    | '(' ULONGLONG_NUM ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_field_length);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_ULONGLONG_NUM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
        }
      }
    | '(' DECIMAL_NUM ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_field_length);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_DECIMAL_NUM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
        }
      }
    | '(' NUM ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_field_length);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_NUM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
        }
      }
    ;

opt_field_length:
      /* empty */
      {
          $$= NULL;
      }
    | field_length
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_field_length);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

opt_precision:
      /* empty */
      {
          $$= NULL;
      }
    | precision
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_precision);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

opt_attribute:
      /* empty */
      {
          $$= NULL;
      }
    | opt_attribute_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_attribute);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

opt_attribute_list:
      opt_attribute_list attribute
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_opt_attribute_list);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | attribute
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_attribute_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

attribute:
      NULL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_attribute);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_NULL_SYM));
        }
      }
    | not NULL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_attribute);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_NULL_SYM));
        }
      }
    | DEFAULT now_or_signed_literal
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_attribute);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DEFAULT));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | ON UPDATE_SYM now
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_attribute);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ON));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_UPDATE_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | AUTO_INC
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_attribute);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_AUTO_INC));
        }
      }
    | SERIAL_SYM DEFAULT VALUE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_attribute);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SERIAL_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_DEFAULT));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_VALUE_SYM));
        }
      }
    | opt_primary KEY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_attribute);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_KEY_SYM));
        }
      }
    | UNIQUE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_attribute);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_UNIQUE_SYM));
        }
      }
    | UNIQUE_SYM KEY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_attribute);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_UNIQUE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_KEY_SYM));
        }
      }
    | COMMENT_SYM TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_attribute);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_COMMENT_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | COLLATE_SYM collation_name
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_attribute);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_COLLATE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | COLUMN_FORMAT_SYM DEFAULT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_attribute);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_COLUMN_FORMAT_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_DEFAULT));
        }
      }
    | COLUMN_FORMAT_SYM FIXED_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_attribute);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_COLUMN_FORMAT_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_FIXED_SYM));
        }
      }
    | COLUMN_FORMAT_SYM DYNAMIC_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_attribute);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_COLUMN_FORMAT_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_DYNAMIC_SYM));
        }
      }
    | STORAGE_SYM DEFAULT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_attribute);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_STORAGE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_DEFAULT));
        }
      }
    | STORAGE_SYM DISK_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_attribute);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_STORAGE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_DISK_SYM));
        }
      }
    | STORAGE_SYM MEMORY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_attribute);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_STORAGE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_MEMORY_SYM));
        }
      }
    ;

type_with_opt_collate:
      type opt_collate
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_type_with_opt_collate);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

now:
      NOW_SYM func_datetime_precision
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_now);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_NOW_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

now_or_signed_literal:
      now
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_now_or_signed_literal);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | signed_literal
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_now_or_signed_literal);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

charset:
      CHAR_SYM SET
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_charset);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CHAR_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_SET));
        }
      }
    | CHARSET
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_charset);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CHARSET));
        }
      }
    ;

charset_name:
      ident_or_text
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_charset_name);
        }
      }
    | BINARY
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_charset_name);
        }
      }
    ;

charset_name_or_default:
      charset_name
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_charset_name_or_default);
        }
      }
    | DEFAULT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_charset_name_or_default);
        }
      }
    ;

opt_load_data_charset:
      /* empty */
      {
          $$= NULL;
      }
    | charset charset_name_or_default
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_load_data_charset);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

old_or_new_charset_name:
      ident_or_text
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_old_or_new_charset_name);
        }
      }
    | BINARY
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_old_or_new_charset_name);
        }
      }
    ;

old_or_new_charset_name_or_default:
      old_or_new_charset_name
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_old_or_new_charset_name_or_default);
        }
      }
    | DEFAULT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_old_or_new_charset_name_or_default);
        }
      }
    ;

collation_name:
      ident_or_text
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_collation_name);
        }
      }
    ;

opt_collate:
      /* empty */
      {
          $$= NULL;
      }
    | COLLATE_SYM collation_name_or_default
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_collate);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_COLLATE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

collation_name_or_default:
      collation_name
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_collation_name_or_default);
        }
      }
    | DEFAULT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_collation_name_or_default);
        }
      }
    ;

opt_default:
      /* empty */
      {
          $$= NULL;
      }
    | DEFAULT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_default);
        }
      }
    ;

ascii:
      ASCII_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ascii);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ASCII_SYM));
        }
      }
    | BINARY ASCII_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ascii);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_BINARY));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_ASCII_SYM));
        }
      }
    | ASCII_SYM BINARY
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ascii);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ASCII_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_BINARY));
        }
      }
    ;

unicode:
      UNICODE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_unicode);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_UNICODE_SYM));
        }
      }
    | UNICODE_SYM BINARY
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_unicode);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_UNICODE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_BINARY));
        }
      }
    | BINARY UNICODE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_unicode);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_BINARY));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_UNICODE_SYM));
        }
      }
    ;

opt_binary:
      /* empty */
      {
          $$= NULL;
      }
    | ascii
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_binary);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | unicode
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_binary);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | BYTE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_binary);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_BYTE_SYM));
        }
      }
    | charset charset_name opt_bin_mod
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_binary);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | BINARY
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_binary);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_BINARY));
        }
      }
    | BINARY charset charset_name
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_binary);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_BINARY));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

opt_bin_mod:
      /* empty */
      {
          $$= NULL;
      }
    | BINARY
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_bin_mod);
        }
      }
    ;

ws_nweights:
      '(' real_ulong_num ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ws_nweights);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
        }
      }
    ;

ws_level_flag_desc:
      ASC
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_ws_level_flag_desc);
        }
      }
    | DESC
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_ws_level_flag_desc);
        }
      }
    ;

ws_level_flag_reverse:
      REVERSE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_ws_level_flag_reverse);
        }
      }
    ;

ws_level_flags:
      /* empty */
      {
          $$= NULL;
      }
    | ws_level_flag_desc
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ws_level_flags);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | ws_level_flag_desc ws_level_flag_reverse
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ws_level_flags);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | ws_level_flag_reverse
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ws_level_flags);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

ws_level_number:
      real_ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_ws_level_number);
        }
      }
    ;

ws_level_list_item:
      ws_level_number ws_level_flags
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ws_level_list_item);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

ws_level_list:
      ws_level_list_item
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ws_level_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | ws_level_list ',' ws_level_list_item
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_ws_level_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

ws_level_range:
      ws_level_number '-' ws_level_number
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ws_level_range);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_45));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

ws_level_list_or_range:
      ws_level_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ws_level_list_or_range);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | ws_level_range
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ws_level_list_or_range);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

opt_ws_levels:
      /* empty */
      {
          $$= NULL;
      }
    | LEVEL_SYM ws_level_list_or_range
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_ws_levels);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_LEVEL_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

opt_primary:
      /* empty */
      {
          $$= NULL;
      }
    | PRIMARY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_primary);
        }
      }
    ;

references:
      REFERENCES table_ident opt_ref_list opt_match_clause opt_on_update_delete
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_references);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_REFERENCES));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    ;

opt_ref_list:
      /* empty */
      {
          $$= NULL;
      }
    | '(' ref_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_ref_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
        }
      }
    ;

ref_list:
      ref_list ',' ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_ref_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ref_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

opt_match_clause:
      /* empty */
      {
          $$= NULL;
      }
    | MATCH FULL
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_match_clause);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MATCH));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_FULL));
        }
      }
    | MATCH PARTIAL
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_match_clause);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MATCH));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_PARTIAL));
        }
      }
    | MATCH SIMPLE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_match_clause);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MATCH));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_SIMPLE_SYM));
        }
      }
    ;

opt_on_update_delete:
      /* empty */
      {
          $$= NULL;
      }
    | ON UPDATE_SYM delete_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_on_update_delete);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ON));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_UPDATE_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | ON DELETE_SYM delete_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_on_update_delete);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ON));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_DELETE_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | ON UPDATE_SYM delete_option ON DELETE_SYM delete_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_on_update_delete);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ON));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_UPDATE_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_ON));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_DELETE_SYM));
          mysql_parser::add_ast_child_node($$, $6);
        }
      }
    | ON DELETE_SYM delete_option ON UPDATE_SYM delete_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_on_update_delete);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ON));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_DELETE_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_ON));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_UPDATE_SYM));
          mysql_parser::add_ast_child_node($$, $6);
        }
      }
    ;

delete_option:
      RESTRICT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_delete_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_RESTRICT));
        }
      }
    | CASCADE
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_delete_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CASCADE));
        }
      }
    | SET NULL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_delete_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SET));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_NULL_SYM));
        }
      }
    | NO_SYM ACTION
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_delete_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_NO_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_ACTION));
        }
      }
    | SET DEFAULT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_delete_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SET));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_DEFAULT));
        }
      }
    ;

normal_key_type:
      key_or_index
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_normal_key_type);
        }
      }
    ;

constraint_key_type:
      PRIMARY_SYM KEY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_constraint_key_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PRIMARY_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_KEY_SYM));
        }
      }
    | UNIQUE_SYM opt_key_or_index
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_constraint_key_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_UNIQUE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

key_or_index:
      KEY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_key_or_index);
        }
      }
    | INDEX_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_key_or_index);
        }
      }
    ;

opt_key_or_index:
      /* empty */
      {
          $$= NULL;
      }
    | key_or_index
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_key_or_index);
        }
      }
    ;

keys_or_index:
      KEYS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keys_or_index);
        }
      }
    | INDEX_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keys_or_index);
        }
      }
    | INDEXES
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keys_or_index);
        }
      }
    ;

opt_unique:
      /* empty */
      {
          $$= NULL;
      }
    | UNIQUE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_unique);
        }
      }
    ;

fulltext:
      FULLTEXT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_fulltext);
        }
      }
    ;

spatial:
      SPATIAL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_spatial);
        }
      }
    ;

init_key_options:
      /* empty */
      {
          $$= NULL;
      }
    ;

key_alg:
      init_key_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_key_alg);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | init_key_options key_using_alg
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_key_alg);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

normal_key_options:
      /* empty */
      {
          $$= NULL;
      }
    | normal_key_opts
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_normal_key_options);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

fulltext_key_options:
      /* empty */
      {
          $$= NULL;
      }
    | fulltext_key_opts
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_fulltext_key_options);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

spatial_key_options:
      /* empty */
      {
          $$= NULL;
      }
    | spatial_key_opts
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_spatial_key_options);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

normal_key_opts:
      normal_key_opt
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_normal_key_opts);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | normal_key_opts normal_key_opt
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_normal_key_opts);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

spatial_key_opts:
      spatial_key_opt
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_spatial_key_opts);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | spatial_key_opts spatial_key_opt
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_spatial_key_opts);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

fulltext_key_opts:
      fulltext_key_opt
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_fulltext_key_opts);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | fulltext_key_opts fulltext_key_opt
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_fulltext_key_opts);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

key_using_alg:
      USING btree_or_rtree
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_key_using_alg);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_USING));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | TYPE_SYM btree_or_rtree
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_key_using_alg);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TYPE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

all_key_opt:
      KEY_BLOCK_SIZE opt_equal ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_all_key_opt);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_KEY_BLOCK_SIZE));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | COMMENT_SYM TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_all_key_opt);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_COMMENT_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

normal_key_opt:
      all_key_opt
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_normal_key_opt);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | key_using_alg
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_normal_key_opt);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

spatial_key_opt:
      all_key_opt
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_spatial_key_opt);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

fulltext_key_opt:
      all_key_opt
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_fulltext_key_opt);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | WITH PARSER_SYM IDENT_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_fulltext_key_opt);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_WITH));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_PARSER_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

btree_or_rtree:
      BTREE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_btree_or_rtree);
        }
      }
    | RTREE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_btree_or_rtree);
        }
      }
    | HASH_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_btree_or_rtree);
        }
      }
    ;

key_list:
      key_list ',' key_part order_dir
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_key_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | key_part order_dir
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_key_list);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

key_part:
      ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_key_part);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | ident '(' NUM ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_key_part);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_NUM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    ;

opt_ident:
      /* empty */
      {
          $$= NULL;
      }
    | field_ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_ident);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

opt_component:
      /* empty */
      {
          $$= NULL;
      }
    | '.' ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_component);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_46));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

string_list:
      text_string
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_string_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | string_list ',' text_string
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_string_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

alter:
      ALTER opt_ignore TABLE_SYM table_ident alter_commands
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ALTER));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_TABLE_SYM));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    | ALTER DATABASE ident_or_empty create_database_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ALTER));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_DATABASE));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | ALTER DATABASE ident UPGRADE_SYM DATA_SYM DIRECTORY_SYM NAME_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ALTER));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_DATABASE));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_UPGRADE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_DATA_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_DIRECTORY_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($7, sql::_NAME_SYM));
        }
      }
    | ALTER PROCEDURE_SYM sp_name sp_a_chistics
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ALTER));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_PROCEDURE_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | ALTER FUNCTION_SYM sp_name sp_a_chistics
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ALTER));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_FUNCTION_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | ALTER view_algorithm definer_opt view_tail
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ALTER));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | ALTER definer_opt view_tail
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ALTER));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | ALTER definer_opt EVENT_SYM sp_name ev_alter_on_schedule_completion opt_ev_rename_to opt_ev_status opt_ev_comment opt_ev_sql_stmt
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ALTER));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_EVENT_SYM));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, $6);
          mysql_parser::add_ast_child_node($$, $7);
          mysql_parser::add_ast_child_node($$, $8);
          mysql_parser::add_ast_child_node($$, $9);
        }
      }
    | ALTER TABLESPACE alter_tablespace_info
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ALTER));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_TABLESPACE));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | ALTER LOGFILE_SYM GROUP_SYM alter_logfile_group_info
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ALTER));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_LOGFILE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_GROUP_SYM));
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | ALTER TABLESPACE change_tablespace_info
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ALTER));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_TABLESPACE));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | ALTER TABLESPACE change_tablespace_access
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ALTER));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_TABLESPACE));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | ALTER SERVER_SYM ident_or_text OPTIONS_SYM '(' server_options_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ALTER));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_SERVER_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_OPTIONS_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_40));
          mysql_parser::add_ast_child_node($$, $6);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($7, sql::_41));
        }
      }
    | ALTER USER clear_privileges alter_user_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ALTER));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_USER));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

alter_user_list:
      user PASSWORD EXPIRE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_user_list);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_PASSWORD));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_EXPIRE_SYM));
        }
      }
    | alter_user_list ',' user PASSWORD EXPIRE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_alter_user_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_PASSWORD));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_EXPIRE_SYM));
        }
      }
    ;

ev_alter_on_schedule_completion:
      /* empty */
      {
          $$= NULL;
      }
    | ON SCHEDULE_SYM ev_schedule_time
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ev_alter_on_schedule_completion);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ON));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_SCHEDULE_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | ev_on_completion
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ev_alter_on_schedule_completion);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | ON SCHEDULE_SYM ev_schedule_time ev_on_completion
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ev_alter_on_schedule_completion);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ON));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_SCHEDULE_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

opt_ev_rename_to:
      /* empty */
      {
          $$= NULL;
      }
    | RENAME TO_SYM sp_name
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_ev_rename_to);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_RENAME));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_TO_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

opt_ev_sql_stmt:
      /* empty */
      {
          $$= NULL;
      }
    | DO_SYM ev_sql_stmt
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_ev_sql_stmt);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DO_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

ident_or_empty:
      /* empty */
      {
          $$= NULL;
      }
    | ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_ident_or_empty);
        }
      }
    ;

alter_commands:
      /* empty */
      {
          $$= NULL;
      }
    | DISCARD TABLESPACE
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_commands);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DISCARD));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_TABLESPACE));
        }
      }
    | IMPORT TABLESPACE
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_commands);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_IMPORT));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_TABLESPACE));
        }
      }
    | alter_list opt_partitioning
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_commands);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | alter_list remove_partitioning
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_commands);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | remove_partitioning
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_commands);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | partitioning
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_commands);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | add_partition_rule
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_commands);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | DROP PARTITION_SYM alt_part_name_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_commands);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DROP));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_PARTITION_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | REBUILD_SYM PARTITION_SYM opt_no_write_to_binlog all_or_alt_part_name_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_commands);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_REBUILD_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_PARTITION_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | OPTIMIZE PARTITION_SYM opt_no_write_to_binlog all_or_alt_part_name_list opt_no_write_to_binlog
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_commands);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_OPTIMIZE));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_PARTITION_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    | ANALYZE_SYM PARTITION_SYM opt_no_write_to_binlog all_or_alt_part_name_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_commands);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ANALYZE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_PARTITION_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | CHECK_SYM PARTITION_SYM all_or_alt_part_name_list opt_mi_check_type
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_commands);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CHECK_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_PARTITION_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | REPAIR PARTITION_SYM opt_no_write_to_binlog all_or_alt_part_name_list opt_mi_repair_type
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_commands);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_REPAIR));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_PARTITION_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    | COALESCE PARTITION_SYM opt_no_write_to_binlog real_ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_commands);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_COALESCE));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_PARTITION_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | TRUNCATE_SYM PARTITION_SYM all_or_alt_part_name_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_commands);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TRUNCATE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_PARTITION_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | reorg_partition_rule
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_commands);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | EXCHANGE_SYM PARTITION_SYM alt_part_name_item WITH TABLE_SYM table_ident have_partitioning
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_commands);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_EXCHANGE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_PARTITION_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_WITH));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_TABLE_SYM));
          mysql_parser::add_ast_child_node($$, $6);
          mysql_parser::add_ast_child_node($$, $7);
        }
      }
    ;

remove_partitioning:
      REMOVE_SYM PARTITIONING_SYM have_partitioning
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_remove_partitioning);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_REMOVE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_PARTITIONING_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

all_or_alt_part_name_list:
      ALL
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_all_or_alt_part_name_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ALL));
        }
      }
    | alt_part_name_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_all_or_alt_part_name_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

add_partition_rule:
      ADD PARTITION_SYM opt_no_write_to_binlog add_part_extra
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_add_partition_rule);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ADD));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_PARTITION_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

add_part_extra:
      /* empty */
      {
          $$= NULL;
      }
    | '(' part_def_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_add_part_extra);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
        }
      }
    | PARTITIONS_SYM real_ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_add_part_extra);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PARTITIONS_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

reorg_partition_rule:
      REORGANIZE_SYM PARTITION_SYM opt_no_write_to_binlog reorg_parts_rule
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_reorg_partition_rule);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_REORGANIZE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_PARTITION_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

reorg_parts_rule:
      /* empty */
      {
          $$= NULL;
      }
    | alt_part_name_list INTO '(' part_def_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_reorg_parts_rule);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_INTO));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_40));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_41));
        }
      }
    ;

alt_part_name_list:
      alt_part_name_item
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alt_part_name_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | alt_part_name_list ',' alt_part_name_item
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_alt_part_name_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

alt_part_name_item:
      ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_alt_part_name_item);
        }
      }
    ;

alter_list:
      alter_list_item
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | alter_list ',' alter_list_item
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_alter_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

add_column:
      ADD opt_column
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_add_column);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ADD));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

alter_list_item:
      add_column column_def opt_place
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_list_item);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | ADD key_def
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_list_item);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ADD));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | add_column '(' create_field_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_list_item);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | CHANGE opt_column field_ident field_spec opt_place
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_list_item);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CHANGE));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    | MODIFY_SYM opt_column field_ident type opt_attribute opt_place
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_list_item);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MODIFY_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, $6);
        }
      }
    | DROP opt_column field_ident opt_restrict
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_list_item);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DROP));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | DROP FOREIGN KEY_SYM field_ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_list_item);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DROP));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_FOREIGN));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_KEY_SYM));
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | DROP PRIMARY_SYM KEY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_list_item);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DROP));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_PRIMARY_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_KEY_SYM));
        }
      }
    | DROP key_or_index field_ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_list_item);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DROP));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | DISABLE_SYM KEYS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_list_item);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DISABLE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_KEYS));
        }
      }
    | ENABLE_SYM KEYS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_list_item);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ENABLE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_KEYS));
        }
      }
    | ALTER opt_column field_ident SET DEFAULT signed_literal
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_list_item);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ALTER));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_SET));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_DEFAULT));
          mysql_parser::add_ast_child_node($$, $6);
        }
      }
    | ALTER opt_column field_ident DROP DEFAULT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_list_item);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ALTER));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_DROP));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_DEFAULT));
        }
      }
    | RENAME opt_to table_ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_list_item);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_RENAME));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | CONVERT_SYM TO_SYM charset charset_name_or_default opt_collate
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_list_item);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CONVERT_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_TO_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    | create_table_options_space_separated
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_list_item);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | FORCE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_list_item);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_FORCE_SYM));
        }
      }
    | alter_order_clause
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_list_item);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | alter_algorithm_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_list_item);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | alter_lock_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_list_item);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

opt_index_lock_algorithm:
      /* empty */
      {
          $$= NULL;
      }
    | alter_lock_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_index_lock_algorithm);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | alter_algorithm_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_index_lock_algorithm);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | alter_lock_option alter_algorithm_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_index_lock_algorithm);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | alter_algorithm_option alter_lock_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_index_lock_algorithm);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

alter_algorithm_option:
      ALGORITHM_SYM opt_equal DEFAULT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_algorithm_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ALGORITHM_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_DEFAULT));
        }
      }
    | ALGORITHM_SYM opt_equal ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_algorithm_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ALGORITHM_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

alter_lock_option:
      LOCK_SYM opt_equal DEFAULT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_lock_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_LOCK_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_DEFAULT));
        }
      }
    | LOCK_SYM opt_equal ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_lock_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_LOCK_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

opt_column:
      /* empty */
      {
          $$= NULL;
      }
    | COLUMN_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_column);
        }
      }
    ;

opt_ignore:
      /* empty */
      {
          $$= NULL;
      }
    | IGNORE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_ignore);
        }
      }
    ;

opt_restrict:
      /* empty */
      {
          $$= NULL;
      }
    | RESTRICT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_restrict);
        }
      }
    | CASCADE
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_restrict);
        }
      }
    ;

opt_place:
      /* empty */
      {
          $$= NULL;
      }
    | AFTER_SYM ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_place);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_AFTER_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | FIRST_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_place);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_FIRST_SYM));
        }
      }
    ;

opt_to:
      /* empty */
      {
          $$= NULL;
      }
    | TO_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_to);
        }
      }
    | EQ
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_to);
        }
      }
    | AS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_to);
        }
      }
    ;

slave:
      START_SYM SLAVE opt_slave_thread_option_list slave_until slave_connection_opts
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_slave);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_START_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_SLAVE));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    | STOP_SYM SLAVE opt_slave_thread_option_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_slave);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_STOP_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_SLAVE));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

start:
      START_SYM TRANSACTION_SYM opt_start_transaction_option_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_start);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_START_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_TRANSACTION_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

opt_start_transaction_option_list:
      /* empty */
      {
          $$= NULL;
      }
    | start_transaction_option_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_start_transaction_option_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

start_transaction_option_list:
      start_transaction_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_start_transaction_option_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | start_transaction_option_list ',' start_transaction_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_start_transaction_option_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

start_transaction_option:
      WITH CONSISTENT_SYM SNAPSHOT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_start_transaction_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_WITH));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_CONSISTENT_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_SNAPSHOT_SYM));
        }
      }
    | READ_SYM ONLY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_start_transaction_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_READ_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_ONLY_SYM));
        }
      }
    | READ_SYM WRITE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_start_transaction_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_READ_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_WRITE_SYM));
        }
      }
    ;

slave_connection_opts:
      slave_user_name_opt slave_user_pass_opt slave_plugin_auth_opt slave_plugin_dir_opt
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_slave_connection_opts);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

slave_user_name_opt:
      /* empty */
      {
          $$= NULL;
      }
    | USER EQ TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_slave_user_name_opt);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_USER));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

slave_user_pass_opt:
      /* empty */
      {
          $$= NULL;
      }
    | PASSWORD EQ TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_slave_user_pass_opt);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PASSWORD));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

slave_plugin_auth_opt:
      /* empty */
      {
          $$= NULL;
      }
    | DEFAULT_AUTH_SYM EQ TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_slave_plugin_auth_opt);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DEFAULT_AUTH_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

slave_plugin_dir_opt:
      /* empty */
      {
          $$= NULL;
      }
    | PLUGIN_DIR_SYM EQ TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_slave_plugin_dir_opt);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PLUGIN_DIR_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

opt_slave_thread_option_list:
      /* empty */
      {
          $$= NULL;
      }
    | slave_thread_option_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_slave_thread_option_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

slave_thread_option_list:
      slave_thread_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_slave_thread_option_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | slave_thread_option_list ',' slave_thread_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_slave_thread_option_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

slave_thread_option:
      SQL_THREAD
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_slave_thread_option);
        }
      }
    | RELAY_THREAD
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_slave_thread_option);
        }
      }
    ;

slave_until:
      /* empty */
      {
          $$= NULL;
      }
    | UNTIL_SYM slave_until_opts
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_slave_until);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_UNTIL_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

slave_until_opts:
      master_file_def
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_slave_until_opts);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | slave_until_opts ',' master_file_def
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_slave_until_opts);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | SQL_BEFORE_GTIDS EQ TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_slave_until_opts);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SQL_BEFORE_GTIDS));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | SQL_AFTER_GTIDS EQ TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_slave_until_opts);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SQL_AFTER_GTIDS));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | SQL_AFTER_MTS_GAPS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_slave_until_opts);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SQL_AFTER_MTS_GAPS));
        }
      }
    ;

checksum:
      CHECKSUM_SYM table_or_tables table_list opt_checksum_type
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_checksum);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CHECKSUM_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

opt_checksum_type:
      /* empty */
      {
          $$= NULL;
      }
    | QUICK
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_checksum_type);
        }
      }
    | EXTENDED_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_checksum_type);
        }
      }
    ;

repair:
      REPAIR opt_no_write_to_binlog table_or_tables table_list opt_mi_repair_type
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_repair);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_REPAIR));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    ;

opt_mi_repair_type:
      /* empty */
      {
          $$= NULL;
      }
    | mi_repair_types
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_mi_repair_type);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

mi_repair_types:
      mi_repair_type
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_mi_repair_types);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | mi_repair_type mi_repair_types
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_mi_repair_types);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::merge_ast_child_nodes($$, $2);
        }
      }
    ;

mi_repair_type:
      QUICK
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_mi_repair_type);
        }
      }
    | EXTENDED_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_mi_repair_type);
        }
      }
    | USE_FRM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_mi_repair_type);
        }
      }
    ;

analyze:
      ANALYZE_SYM opt_no_write_to_binlog table_or_tables table_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_analyze);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ANALYZE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

binlog_base64_event:
      BINLOG_SYM TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_binlog_base64_event);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_BINLOG_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

check:
      CHECK_SYM table_or_tables table_list opt_mi_check_type
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_check);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CHECK_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

opt_mi_check_type:
      /* empty */
      {
          $$= NULL;
      }
    | mi_check_types
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_mi_check_type);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

mi_check_types:
      mi_check_type
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_mi_check_types);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | mi_check_type mi_check_types
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_mi_check_types);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::merge_ast_child_nodes($$, $2);
        }
      }
    ;

mi_check_type:
      QUICK
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_mi_check_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_QUICK));
        }
      }
    | FAST_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_mi_check_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_FAST_SYM));
        }
      }
    | MEDIUM_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_mi_check_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MEDIUM_SYM));
        }
      }
    | EXTENDED_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_mi_check_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_EXTENDED_SYM));
        }
      }
    | CHANGED
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_mi_check_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CHANGED));
        }
      }
    | FOR_SYM UPGRADE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_mi_check_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_FOR_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_UPGRADE_SYM));
        }
      }
    ;

optimize:
      OPTIMIZE opt_no_write_to_binlog table_or_tables table_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_optimize);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_OPTIMIZE));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

opt_no_write_to_binlog:
      /* empty */
      {
          $$= NULL;
      }
    | NO_WRITE_TO_BINLOG
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_no_write_to_binlog);
        }
      }
    | LOCAL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_no_write_to_binlog);
        }
      }
    ;

rename:
      RENAME table_or_tables table_to_table_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_rename);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_RENAME));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | RENAME USER clear_privileges rename_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_rename);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_RENAME));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_USER));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

rename_list:
      user TO_SYM user
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_rename_list);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_TO_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | rename_list ',' user TO_SYM user
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_rename_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_TO_SYM));
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    ;

table_to_table_list:
      table_to_table
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_table_to_table_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | table_to_table_list ',' table_to_table
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_table_to_table_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

table_to_table:
      table_ident TO_SYM table_ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_table_to_table);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_TO_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

keycache:
      CACHE_SYM INDEX_SYM keycache_list_or_parts IN_SYM key_cache_name
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_keycache);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CACHE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_INDEX_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_IN_SYM));
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    ;

keycache_list_or_parts:
      keycache_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_keycache_list_or_parts);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | assign_to_keycache_parts
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_keycache_list_or_parts);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

keycache_list:
      assign_to_keycache
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_keycache_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | keycache_list ',' assign_to_keycache
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_keycache_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

assign_to_keycache:
      table_ident cache_keys_spec
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_assign_to_keycache);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

assign_to_keycache_parts:
      table_ident adm_partition cache_keys_spec
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_assign_to_keycache_parts);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

key_cache_name:
      ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_key_cache_name);
        }
      }
    | DEFAULT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_key_cache_name);
        }
      }
    ;

preload:
      LOAD INDEX_SYM INTO CACHE_SYM preload_list_or_parts
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_preload);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_LOAD));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_INDEX_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_INTO));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_CACHE_SYM));
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    ;

preload_list_or_parts:
      preload_keys_parts
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_preload_list_or_parts);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | preload_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_preload_list_or_parts);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

preload_list:
      preload_keys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_preload_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | preload_list ',' preload_keys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_preload_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

preload_keys:
      table_ident cache_keys_spec opt_ignore_leaves
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_preload_keys);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

preload_keys_parts:
      table_ident adm_partition cache_keys_spec opt_ignore_leaves
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_preload_keys_parts);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

adm_partition:
      PARTITION_SYM have_partitioning '(' all_or_alt_part_name_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_adm_partition);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PARTITION_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_40));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_41));
        }
      }
    ;

cache_keys_spec:
      cache_key_list_or_empty
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_cache_keys_spec);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

cache_key_list_or_empty:
      /* empty */
      {
          $$= NULL;
      }
    | key_or_index '(' opt_key_usage_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_cache_key_list_or_empty);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    ;

opt_ignore_leaves:
      /* empty */
      {
          $$= NULL;
      }
    | IGNORE_SYM LEAVES
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_ignore_leaves);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_IGNORE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_LEAVES));
        }
      }
    ;

select:
      select_init
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

select_init:
      SELECT_SYM select_init2
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_init);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SELECT_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | '(' select_paren ')' union_opt
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_init);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

select_paren:
      SELECT_SYM select_part2
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_paren);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SELECT_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | '(' select_paren ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_paren);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
        }
      }
    ;

select_paren_derived:
      SELECT_SYM select_part2_derived
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_paren_derived);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SELECT_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | '(' select_paren_derived ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_paren_derived);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
        }
      }
    ;

select_init2:
      select_part2 union_clause
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_init2);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

select_part2:
      select_options select_item_list select_into select_lock_type
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_part2);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

select_into:
      opt_order_clause opt_limit_clause
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_into);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | into
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_into);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | select_from
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_into);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | into select_from
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_into);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | select_from into
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_into);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

select_from:
      FROM join_table_list where_clause group_clause having_clause opt_order_clause opt_limit_clause procedure_analyse_clause
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_from);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_FROM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, $6);
          mysql_parser::add_ast_child_node($$, $7);
          mysql_parser::add_ast_child_node($$, $8);
        }
      }
    | FROM DUAL_SYM where_clause opt_limit_clause
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_from);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_FROM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_DUAL_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

select_options:
      /* empty */
      {
          $$= NULL;
      }
    | select_option_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_options);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

select_option_list:
      select_option_list select_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_select_option_list);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | select_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_option_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

select_option:
      query_expression_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_select_option);
        }
      }
    | SQL_NO_CACHE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_select_option);
        }
      }
    | SQL_CACHE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_select_option);
        }
      }
    ;

select_lock_type:
      /* empty */
      {
          $$= NULL;
      }
    | FOR_SYM UPDATE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_lock_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_FOR_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_UPDATE_SYM));
        }
      }
    | LOCK_SYM IN_SYM SHARE_SYM MODE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_lock_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_LOCK_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_IN_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_SHARE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_MODE_SYM));
        }
      }
    ;

select_item_list:
      select_item_list ',' select_item
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_select_item_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | select_item
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_item_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | '*'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_item_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_42));
        }
      }
    ;

select_item:
      remember_name table_wild remember_end
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_item);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | remember_name expr remember_end select_alias
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_item);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

remember_name:
      /* empty */
      {
          $$= NULL;
      }
    ;

remember_end:
      /* empty */
      {
          $$= NULL;
      }
    ;

select_alias:
      /* empty */
      {
          $$= NULL;
      }
    | AS ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_alias);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_AS));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | AS TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_alias);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_AS));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_alias);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_alias);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

optional_braces:
      /* empty */
      {
          $$= NULL;
      }
    | '(' ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_optional_braces);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_41));
        }
      }
    ;

expr:
      expr or expr %prec OR_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_expr);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | expr XOR expr %prec XOR
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_expr);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_XOR));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | expr and expr %prec AND_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_expr);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | NOT_SYM expr %prec NOT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_NOT_SYM));
          mysql_parser::merge_ast_child_nodes($$, $2);
        }
      }
    | bool_pri IS TRUE_SYM %prec IS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_expr);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_IS));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_TRUE_SYM));
        }
      }
    | bool_pri IS not TRUE_SYM %prec IS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_expr);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_IS));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_TRUE_SYM));
        }
      }
    | bool_pri IS FALSE_SYM %prec IS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_expr);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_IS));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_FALSE_SYM));
        }
      }
    | bool_pri IS not FALSE_SYM %prec IS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_expr);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_IS));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_FALSE_SYM));
        }
      }
    | bool_pri IS UNKNOWN_SYM %prec IS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_expr);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_IS));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_UNKNOWN_SYM));
        }
      }
    | bool_pri IS not UNKNOWN_SYM %prec IS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_expr);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_IS));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_UNKNOWN_SYM));
        }
      }
    | bool_pri
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_expr);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

bool_pri:
      bool_pri IS NULL_SYM %prec IS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_bool_pri);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_IS));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_NULL_SYM));
        }
      }
    | bool_pri IS not NULL_SYM %prec IS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_bool_pri);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_IS));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_NULL_SYM));
        }
      }
    | bool_pri EQUAL_SYM predicate %prec EQUAL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_bool_pri);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQUAL_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | bool_pri comp_op predicate %prec EQ
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_bool_pri);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | bool_pri comp_op all_or_any '(' subselect ')' %prec EQ
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_bool_pri);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_40));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | predicate
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_bool_pri);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

predicate:
      bit_expr IN_SYM '(' subselect ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_predicate);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_IN_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_40));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_41));
        }
      }
    | bit_expr not IN_SYM '(' subselect ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_predicate);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_IN_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_40));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | bit_expr IN_SYM '(' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_predicate);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_IN_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_40));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_41));
        }
      }
    | bit_expr IN_SYM '(' expr ',' expr_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_predicate);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_IN_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_40));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_44));
          mysql_parser::add_ast_child_node($$, $6);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($7, sql::_41));
        }
      }
    | bit_expr not IN_SYM '(' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_predicate);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_IN_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_40));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | bit_expr not IN_SYM '(' expr ',' expr_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_predicate);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_IN_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_40));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_44));
          mysql_parser::add_ast_child_node($$, $7);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($8, sql::_41));
        }
      }
    | bit_expr BETWEEN_SYM bit_expr AND_SYM predicate
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_predicate);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_BETWEEN_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_AND_SYM));
          mysql_parser::merge_ast_child_nodes($$, $5);
        }
      }
    | bit_expr not BETWEEN_SYM bit_expr AND_SYM predicate
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_predicate);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_BETWEEN_SYM));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_AND_SYM));
          mysql_parser::merge_ast_child_nodes($$, $6);
        }
      }
    | bit_expr SOUNDS_SYM LIKE bit_expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_predicate);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_SOUNDS_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_LIKE));
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | bit_expr LIKE simple_expr opt_escape
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_predicate);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_LIKE));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | bit_expr not LIKE simple_expr opt_escape
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_predicate);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_LIKE));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    | bit_expr REGEXP bit_expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_predicate);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_REGEXP));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | bit_expr not REGEXP bit_expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_predicate);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_REGEXP));
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | bit_expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_predicate);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

bit_expr:
      bit_expr '|' bit_expr %prec '|'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_bit_expr);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_124));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | bit_expr '&' bit_expr %prec '&'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_bit_expr);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_38));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | bit_expr SHIFT_LEFT bit_expr %prec SHIFT_LEFT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_bit_expr);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_SHIFT_LEFT));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | bit_expr SHIFT_RIGHT bit_expr %prec SHIFT_RIGHT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_bit_expr);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_SHIFT_RIGHT));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | bit_expr '+' bit_expr %prec '+'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_bit_expr);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_43));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | bit_expr '-' bit_expr %prec '-'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_bit_expr);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_45));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | bit_expr '+' INTERVAL_SYM expr interval %prec '+'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_bit_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_43));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_INTERVAL_SYM));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    | bit_expr '-' INTERVAL_SYM expr interval %prec '-'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_bit_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_45));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_INTERVAL_SYM));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    | bit_expr '*' bit_expr %prec '*'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_bit_expr);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_42));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | bit_expr '/' bit_expr %prec '/'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_bit_expr);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_47));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | bit_expr '%' bit_expr %prec '%'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_bit_expr);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_37));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | bit_expr DIV_SYM bit_expr %prec DIV_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_bit_expr);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_DIV_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | bit_expr MOD_SYM bit_expr %prec MOD_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_bit_expr);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_MOD_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | bit_expr '^' bit_expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_bit_expr);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_94));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | simple_expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_bit_expr);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

or:
      OR_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_or);
        }
      }
    | OR2_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_or);
        }
      }
    ;

and:
      AND_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_and);
        }
      }
    | AND_AND_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_and);
        }
      }
    ;

not:
      NOT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_not);
        }
      }
    | NOT2_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_not);
        }
      }
    ;

not2:
      '!'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_not2);
        }
      }
    | NOT2_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_not2);
        }
      }
    ;

comp_op:
      EQ
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_comp_op);
        }
      }
    | GE
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_comp_op);
        }
      }
    | GT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_comp_op);
        }
      }
    | LE
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_comp_op);
        }
      }
    | LT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_comp_op);
        }
      }
    | NE
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_comp_op);
        }
      }
    ;

all_or_any:
      ALL
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_all_or_any);
        }
      }
    | ANY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_all_or_any);
        }
      }
    ;

simple_expr:
      simple_ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_expr);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | function_call_keyword
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_expr);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | function_call_nonkeyword
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_expr);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | function_call_generic
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_expr);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | function_call_conflict
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_expr);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | simple_expr COLLATE_SYM ident_or_text %prec NEG
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_simple_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_COLLATE_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | literal
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_expr);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | param_marker
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_expr);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | variable
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_expr);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sum_expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_expr);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | simple_expr OR_OR_SYM simple_expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_expr);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_OR_OR_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | '+' simple_expr %prec NEG
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_43));
          mysql_parser::merge_ast_child_nodes($$, $2);
        }
      }
    | '-' simple_expr %prec NEG
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_45));
          mysql_parser::merge_ast_child_nodes($$, $2);
        }
      }
    | '~' simple_expr %prec NEG
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_126));
          mysql_parser::merge_ast_child_nodes($$, $2);
        }
      }
    | not2 simple_expr %prec NEG
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_expr);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::merge_ast_child_nodes($$, $2);
        }
      }
    | '(' subselect ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
        }
      }
    | '(' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
        }
      }
    | '(' expr ',' expr_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_44));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_41));
        }
      }
    | ROW_SYM '(' expr ',' expr_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ROW_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | EXISTS '(' subselect ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_EXISTS));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | '{' ident expr '}'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_123));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_125));
        }
      }
    | MATCH ident_list_arg AGAINST '(' bit_expr fulltext_options ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MATCH));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_AGAINST));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_40));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, $6);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($7, sql::_41));
        }
      }
    | BINARY simple_expr %prec NEG
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_BINARY));
          mysql_parser::merge_ast_child_nodes($$, $2);
        }
      }
    | CAST_SYM '(' expr AS cast_type ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CAST_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_AS));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | CASE_SYM opt_expr when_list opt_else END
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CASE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_END));
        }
      }
    | CONVERT_SYM '(' expr ',' cast_type ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CONVERT_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | CONVERT_SYM '(' expr USING charset_name ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CONVERT_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_USING));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | DEFAULT '(' simple_ident ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DEFAULT));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | VALUES '(' simple_ident_nospvar ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_VALUES));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | INTERVAL_SYM expr interval '+' expr %prec INTERVAL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_INTERVAL_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_43));
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    ;

function_call_keyword:
      CHAR_SYM '(' expr_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_keyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CHAR_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | CHAR_SYM '(' expr_list USING charset_name ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_keyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CHAR_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_USING));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | CURRENT_USER optional_braces
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_keyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CURRENT_USER));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | DATE_SYM '(' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_keyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DATE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | DAY_SYM '(' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_keyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DAY_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | HOUR_SYM '(' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_keyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_HOUR_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | INSERT '(' expr ',' expr ',' expr ',' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_keyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_INSERT));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_44));
          mysql_parser::add_ast_child_node($$, $7);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($8, sql::_44));
          mysql_parser::add_ast_child_node($$, $9);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($10, sql::_41));
        }
      }
    | INTERVAL_SYM '(' expr ',' expr ')' %prec INTERVAL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_keyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_INTERVAL_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | INTERVAL_SYM '(' expr ',' expr ',' expr_list ')' %prec INTERVAL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_keyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_INTERVAL_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_44));
          mysql_parser::add_ast_child_node($$, $7);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($8, sql::_41));
        }
      }
    | LEFT '(' expr ',' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_keyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_LEFT));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | MINUTE_SYM '(' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_keyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MINUTE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | MONTH_SYM '(' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_keyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MONTH_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | RIGHT '(' expr ',' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_keyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_RIGHT));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | SECOND_SYM '(' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_keyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SECOND_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | TIME_SYM '(' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_keyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TIME_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | TIMESTAMP '(' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_keyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TIMESTAMP));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | TIMESTAMP '(' expr ',' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_keyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TIMESTAMP));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | TRIM '(' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_keyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TRIM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | TRIM '(' LEADING expr FROM expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_keyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TRIM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_LEADING));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_FROM));
          mysql_parser::add_ast_child_node($$, $6);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($7, sql::_41));
        }
      }
    | TRIM '(' TRAILING expr FROM expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_keyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TRIM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_TRAILING));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_FROM));
          mysql_parser::add_ast_child_node($$, $6);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($7, sql::_41));
        }
      }
    | TRIM '(' BOTH expr FROM expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_keyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TRIM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_BOTH));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_FROM));
          mysql_parser::add_ast_child_node($$, $6);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($7, sql::_41));
        }
      }
    | TRIM '(' LEADING FROM expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_keyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TRIM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_LEADING));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_FROM));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | TRIM '(' TRAILING FROM expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_keyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TRIM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_TRAILING));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_FROM));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | TRIM '(' BOTH FROM expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_keyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TRIM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_BOTH));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_FROM));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | TRIM '(' expr FROM expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_keyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TRIM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_FROM));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | USER '(' ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_keyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_USER));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
        }
      }
    | YEAR_SYM '(' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_keyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_YEAR_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    ;

function_call_nonkeyword:
      ADDDATE_SYM '(' expr ',' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_nonkeyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ADDDATE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | ADDDATE_SYM '(' expr ',' INTERVAL_SYM expr interval ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_nonkeyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ADDDATE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_INTERVAL_SYM));
          mysql_parser::add_ast_child_node($$, $6);
          mysql_parser::add_ast_child_node($$, $7);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($8, sql::_41));
        }
      }
    | CURDATE optional_braces
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_nonkeyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CURDATE));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | CURTIME func_datetime_precision
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_nonkeyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CURTIME));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | DATE_ADD_INTERVAL '(' expr ',' INTERVAL_SYM expr interval ')' %prec INTERVAL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_nonkeyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DATE_ADD_INTERVAL));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_INTERVAL_SYM));
          mysql_parser::add_ast_child_node($$, $6);
          mysql_parser::add_ast_child_node($$, $7);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($8, sql::_41));
        }
      }
    | DATE_SUB_INTERVAL '(' expr ',' INTERVAL_SYM expr interval ')' %prec INTERVAL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_nonkeyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DATE_SUB_INTERVAL));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_INTERVAL_SYM));
          mysql_parser::add_ast_child_node($$, $6);
          mysql_parser::add_ast_child_node($$, $7);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($8, sql::_41));
        }
      }
    | EXTRACT_SYM '(' interval FROM expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_nonkeyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_EXTRACT_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_FROM));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | GET_FORMAT '(' date_time_type ',' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_nonkeyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_GET_FORMAT));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | now
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_nonkeyword);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | POSITION_SYM '(' bit_expr IN_SYM expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_nonkeyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_POSITION_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_IN_SYM));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | SUBDATE_SYM '(' expr ',' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_nonkeyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SUBDATE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | SUBDATE_SYM '(' expr ',' INTERVAL_SYM expr interval ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_nonkeyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SUBDATE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_INTERVAL_SYM));
          mysql_parser::add_ast_child_node($$, $6);
          mysql_parser::add_ast_child_node($$, $7);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($8, sql::_41));
        }
      }
    | SUBSTRING '(' expr ',' expr ',' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_nonkeyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SUBSTRING));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_44));
          mysql_parser::add_ast_child_node($$, $7);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($8, sql::_41));
        }
      }
    | SUBSTRING '(' expr ',' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_nonkeyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SUBSTRING));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | SUBSTRING '(' expr FROM expr FOR_SYM expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_nonkeyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SUBSTRING));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_FROM));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_FOR_SYM));
          mysql_parser::add_ast_child_node($$, $7);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($8, sql::_41));
        }
      }
    | SUBSTRING '(' expr FROM expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_nonkeyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SUBSTRING));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_FROM));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | SYSDATE func_datetime_precision
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_nonkeyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SYSDATE));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | TIMESTAMP_ADD '(' interval_time_stamp ',' expr ',' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_nonkeyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TIMESTAMP_ADD));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_44));
          mysql_parser::add_ast_child_node($$, $7);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($8, sql::_41));
        }
      }
    | TIMESTAMP_DIFF '(' interval_time_stamp ',' expr ',' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_nonkeyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TIMESTAMP_DIFF));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_44));
          mysql_parser::add_ast_child_node($$, $7);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($8, sql::_41));
        }
      }
    | UTC_DATE_SYM optional_braces
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_nonkeyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_UTC_DATE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | UTC_TIME_SYM func_datetime_precision
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_nonkeyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_UTC_TIME_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | UTC_TIMESTAMP_SYM func_datetime_precision
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_nonkeyword);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_UTC_TIMESTAMP_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

function_call_conflict:
      ASCII_SYM '(' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_conflict);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ASCII_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | CHARSET '(' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_conflict);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CHARSET));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | COALESCE '(' expr_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_conflict);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_COALESCE));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | COLLATION_SYM '(' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_conflict);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_COLLATION_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | DATABASE '(' ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_conflict);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DATABASE));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
        }
      }
    | IF '(' expr ',' expr ',' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_conflict);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_IF));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_44));
          mysql_parser::add_ast_child_node($$, $7);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($8, sql::_41));
        }
      }
    | FORMAT_SYM '(' expr ',' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_conflict);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_FORMAT_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | FORMAT_SYM '(' expr ',' expr ',' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_conflict);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_FORMAT_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_44));
          mysql_parser::add_ast_child_node($$, $7);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($8, sql::_41));
        }
      }
    | MICROSECOND_SYM '(' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_conflict);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MICROSECOND_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | MOD_SYM '(' expr ',' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_conflict);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MOD_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | OLD_PASSWORD '(' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_conflict);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_OLD_PASSWORD));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | PASSWORD '(' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_conflict);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PASSWORD));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | QUARTER_SYM '(' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_conflict);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_QUARTER_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | REPEAT_SYM '(' expr ',' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_conflict);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_REPEAT_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | REPLACE '(' expr ',' expr ',' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_conflict);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_REPLACE));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_44));
          mysql_parser::add_ast_child_node($$, $7);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($8, sql::_41));
        }
      }
    | REVERSE_SYM '(' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_conflict);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_REVERSE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | ROW_COUNT_SYM '(' ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_conflict);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ROW_COUNT_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
        }
      }
    | TRUNCATE_SYM '(' expr ',' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_conflict);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TRUNCATE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | WEEK_SYM '(' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_conflict);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_WEEK_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | WEEK_SYM '(' expr ',' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_conflict);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_WEEK_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | WEIGHT_STRING_SYM '(' expr opt_ws_levels ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_conflict);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_WEIGHT_STRING_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_41));
        }
      }
    | WEIGHT_STRING_SYM '(' expr AS CHAR_SYM ws_nweights opt_ws_levels ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_conflict);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_WEIGHT_STRING_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_AS));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_CHAR_SYM));
          mysql_parser::add_ast_child_node($$, $6);
          mysql_parser::add_ast_child_node($$, $7);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($8, sql::_41));
        }
      }
    | WEIGHT_STRING_SYM '(' expr AS BINARY ws_nweights ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_conflict);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_WEIGHT_STRING_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_AS));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_BINARY));
          mysql_parser::add_ast_child_node($$, $6);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($7, sql::_41));
        }
      }
    | WEIGHT_STRING_SYM '(' expr ',' ulong_num ',' ulong_num ',' ulong_num ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_conflict);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_WEIGHT_STRING_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_44));
          mysql_parser::add_ast_child_node($$, $7);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($8, sql::_44));
          mysql_parser::add_ast_child_node($$, $9);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($10, sql::_41));
        }
      }
    | geometry_function
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_conflict);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

geometry_function:
      CONTAINS_SYM '(' expr ',' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_geometry_function);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CONTAINS_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | GEOMETRYCOLLECTION '(' expr_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_geometry_function);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_GEOMETRYCOLLECTION));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | LINESTRING '(' expr_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_geometry_function);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_LINESTRING));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | MULTILINESTRING '(' expr_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_geometry_function);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MULTILINESTRING));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | MULTIPOINT '(' expr_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_geometry_function);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MULTIPOINT));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | MULTIPOLYGON '(' expr_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_geometry_function);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MULTIPOLYGON));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | POINT_SYM '(' expr ',' expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_geometry_function);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_POINT_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | POLYGON '(' expr_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_geometry_function);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_POLYGON));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    ;

function_call_generic:
      IDENT_sys '(' opt_udf_expr_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_generic);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | ident '.' ident '(' opt_expr_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_function_call_generic);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_46));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_40));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    ;

fulltext_options:
      opt_natural_language_mode opt_query_expansion
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_fulltext_options);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | IN_SYM BOOLEAN_SYM MODE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_fulltext_options);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_IN_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_BOOLEAN_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_MODE_SYM));
        }
      }
    ;

opt_natural_language_mode:
      /* empty */
      {
          $$= NULL;
      }
    | IN_SYM NATURAL LANGUAGE_SYM MODE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_natural_language_mode);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_IN_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_NATURAL));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_LANGUAGE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_MODE_SYM));
        }
      }
    ;

opt_query_expansion:
      /* empty */
      {
          $$= NULL;
      }
    | WITH QUERY_SYM EXPANSION_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_query_expansion);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_WITH));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_QUERY_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_EXPANSION_SYM));
        }
      }
    ;

opt_udf_expr_list:
      /* empty */
      {
          $$= NULL;
      }
    | udf_expr_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_udf_expr_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

udf_expr_list:
      udf_expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_udf_expr_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | udf_expr_list ',' udf_expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_udf_expr_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

udf_expr:
      remember_name expr remember_end select_alias
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_udf_expr);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

sum_expr:
      AVG_SYM '(' in_sum_expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sum_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_AVG_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | AVG_SYM '(' DISTINCT in_sum_expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sum_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_AVG_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_DISTINCT));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_41));
        }
      }
    | BIT_AND '(' in_sum_expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sum_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_BIT_AND));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | BIT_OR '(' in_sum_expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sum_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_BIT_OR));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | BIT_XOR '(' in_sum_expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sum_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_BIT_XOR));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | COUNT_SYM '(' opt_all '*' ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sum_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_COUNT_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_42));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_41));
        }
      }
    | COUNT_SYM '(' in_sum_expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sum_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_COUNT_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | COUNT_SYM '(' DISTINCT expr_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sum_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_COUNT_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_DISTINCT));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_41));
        }
      }
    | MIN_SYM '(' in_sum_expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sum_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MIN_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | MIN_SYM '(' DISTINCT in_sum_expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sum_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MIN_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_DISTINCT));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_41));
        }
      }
    | MAX_SYM '(' in_sum_expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sum_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MAX_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | MAX_SYM '(' DISTINCT in_sum_expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sum_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MAX_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_DISTINCT));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_41));
        }
      }
    | STD_SYM '(' in_sum_expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sum_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_STD_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | VARIANCE_SYM '(' in_sum_expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sum_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_VARIANCE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | STDDEV_SAMP_SYM '(' in_sum_expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sum_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_STDDEV_SAMP_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | VAR_SAMP_SYM '(' in_sum_expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sum_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_VAR_SAMP_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | SUM_SYM '(' in_sum_expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sum_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SUM_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | SUM_SYM '(' DISTINCT in_sum_expr ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sum_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SUM_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_DISTINCT));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_41));
        }
      }
    | GROUP_CONCAT_SYM '(' opt_distinct expr_list opt_gorder_clause opt_gconcat_separator ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sum_expr);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_GROUP_CONCAT_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, $6);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($7, sql::_41));
        }
      }
    ;

variable:
      '@' variable_aux
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_variable);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_64));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

variable_aux:
      ident_or_text SET_VAR expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_variable_aux);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_SET_VAR));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | ident_or_text
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_variable_aux);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | '@' opt_var_ident_type ident_or_text opt_component
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_variable_aux);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_64));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

opt_distinct:
      /* empty */
      {
          $$= NULL;
      }
    | DISTINCT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_distinct);
        }
      }
    ;

opt_gconcat_separator:
      /* empty */
      {
          $$= NULL;
      }
    | SEPARATOR_SYM text_string
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_gconcat_separator);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SEPARATOR_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

opt_gorder_clause:
      /* empty */
      {
          $$= NULL;
      }
    | ORDER_SYM BY gorder_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_gorder_clause);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ORDER_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_BY));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

gorder_list:
      gorder_list ',' order_ident order_dir
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_gorder_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | order_ident order_dir
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_gorder_list);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

in_sum_expr:
      opt_all expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_in_sum_expr);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

cast_type:
      BINARY opt_field_length
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_cast_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_BINARY));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | CHAR_SYM opt_field_length opt_binary
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_cast_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CHAR_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | NCHAR_SYM opt_field_length
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_cast_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_NCHAR_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | SIGNED_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_cast_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SIGNED_SYM));
        }
      }
    | SIGNED_SYM INT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_cast_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SIGNED_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_INT_SYM));
        }
      }
    | UNSIGNED
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_cast_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_UNSIGNED));
        }
      }
    | UNSIGNED INT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_cast_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_UNSIGNED));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_INT_SYM));
        }
      }
    | DATE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_cast_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DATE_SYM));
        }
      }
    | TIME_SYM type_datetime_precision
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_cast_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TIME_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | DATETIME type_datetime_precision
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_cast_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DATETIME));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | DECIMAL_SYM float_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_cast_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DECIMAL_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

opt_expr_list:
      /* empty */
      {
          $$= NULL;
      }
    | expr_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_expr_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

expr_list:
      expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_expr_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | expr_list ',' expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_expr_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

ident_list_arg:
      ident_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ident_list_arg);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | '(' ident_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ident_list_arg);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
        }
      }
    ;

ident_list:
      simple_ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ident_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | ident_list ',' simple_ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_ident_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

opt_expr:
      /* empty */
      {
          $$= NULL;
      }
    | expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_expr);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

opt_else:
      /* empty */
      {
          $$= NULL;
      }
    | ELSE expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_else);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ELSE));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

when_list:
      WHEN_SYM expr THEN_SYM expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_when_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_WHEN_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_THEN_SYM));
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | when_list WHEN_SYM expr THEN_SYM expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_when_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_WHEN_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_THEN_SYM));
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    ;

table_ref:
      table_factor
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_table_ref);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | join_table
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_table_ref);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

join_table_list:
      derived_table_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_join_table_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

esc_table_ref:
      table_ref
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_esc_table_ref);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | '{' ident table_ref '}'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_esc_table_ref);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_123));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_125));
        }
      }
    ;

derived_table_list:
      esc_table_ref
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_derived_table_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | derived_table_list ',' esc_table_ref
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_derived_table_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

join_table:
      table_ref normal_join table_ref %prec TABLE_REF_PRIORITY
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_join_table);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | table_ref STRAIGHT_JOIN table_factor
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_join_table);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_STRAIGHT_JOIN));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | table_ref normal_join table_ref ON expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_join_table);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_ON));
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    | table_ref STRAIGHT_JOIN table_factor ON expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_join_table);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_STRAIGHT_JOIN));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_ON));
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    | table_ref normal_join table_ref USING '(' using_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_join_table);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_USING));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_40));
          mysql_parser::add_ast_child_node($$, $6);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($7, sql::_41));
        }
      }
    | table_ref NATURAL JOIN_SYM table_factor
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_join_table);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_NATURAL));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_JOIN_SYM));
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | table_ref LEFT opt_outer JOIN_SYM table_ref ON expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_join_table);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_LEFT));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_JOIN_SYM));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_ON));
          mysql_parser::add_ast_child_node($$, $7);
        }
      }
    | table_ref LEFT opt_outer JOIN_SYM table_factor USING '(' using_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_join_table);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_LEFT));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_JOIN_SYM));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_USING));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($7, sql::_40));
          mysql_parser::add_ast_child_node($$, $8);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($9, sql::_41));
        }
      }
    | table_ref NATURAL LEFT opt_outer JOIN_SYM table_factor
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_join_table);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_NATURAL));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_LEFT));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_JOIN_SYM));
          mysql_parser::add_ast_child_node($$, $6);
        }
      }
    | table_ref RIGHT opt_outer JOIN_SYM table_ref ON expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_join_table);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_RIGHT));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_JOIN_SYM));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_ON));
          mysql_parser::add_ast_child_node($$, $7);
        }
      }
    | table_ref RIGHT opt_outer JOIN_SYM table_factor USING '(' using_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_join_table);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_RIGHT));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_JOIN_SYM));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_USING));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($7, sql::_40));
          mysql_parser::add_ast_child_node($$, $8);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($9, sql::_41));
        }
      }
    | table_ref NATURAL RIGHT opt_outer JOIN_SYM table_factor
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_join_table);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_NATURAL));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_RIGHT));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_JOIN_SYM));
          mysql_parser::add_ast_child_node($$, $6);
        }
      }
    ;

normal_join:
      JOIN_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_normal_join);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_JOIN_SYM));
        }
      }
    | INNER_SYM JOIN_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_normal_join);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_INNER_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_JOIN_SYM));
        }
      }
    | CROSS JOIN_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_normal_join);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CROSS));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_JOIN_SYM));
        }
      }
    ;

opt_use_partition:
      /* empty */
      {
          $$= NULL;
      }
    | use_partition
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_use_partition);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

use_partition:
      PARTITION_SYM '(' using_list ')' have_partitioning
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_use_partition);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PARTITION_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    ;

table_factor:
      table_ident opt_use_partition opt_table_alias opt_key_definition
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_table_factor);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | select_derived_init get_select_lex select_derived2
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_table_factor);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | '(' get_select_lex select_derived_union ')' opt_table_alias
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_table_factor);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    ;

select_derived_union:
      select_derived opt_union_order_or_limit
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_derived_union);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | select_derived_union UNION_SYM union_option query_specification opt_union_order_or_limit
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_select_derived_union);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_UNION_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    ;

select_init2_derived:
      select_part2_derived
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_init2_derived);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

select_part2_derived:
      opt_query_expression_options select_item_list opt_select_from select_lock_type
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_part2_derived);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

select_derived:
      get_select_lex derived_table_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_derived);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

select_derived2:
      select_options select_item_list opt_select_from
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_derived2);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

get_select_lex:
      /* empty */
      {
          $$= NULL;
      }
    ;

select_derived_init:
      SELECT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_select_derived_init);
        }
      }
    ;

opt_outer:
      /* empty */
      {
          $$= NULL;
      }
    | OUTER
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_outer);
        }
      }
    ;

index_hint_clause:
      /* empty */
      {
          $$= NULL;
      }
    | FOR_SYM JOIN_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_index_hint_clause);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_FOR_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_JOIN_SYM));
        }
      }
    | FOR_SYM ORDER_SYM BY
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_index_hint_clause);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_FOR_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_ORDER_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_BY));
        }
      }
    | FOR_SYM GROUP_SYM BY
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_index_hint_clause);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_FOR_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_GROUP_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_BY));
        }
      }
    ;

index_hint_type:
      FORCE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_index_hint_type);
        }
      }
    | IGNORE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_index_hint_type);
        }
      }
    ;

index_hint_definition:
      index_hint_type key_or_index index_hint_clause '(' key_usage_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_index_hint_definition);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_40));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    | USE_SYM key_or_index index_hint_clause '(' opt_key_usage_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_index_hint_definition);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_USE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_40));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
        }
      }
    ;

index_hints_list:
      index_hint_definition
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_index_hints_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | index_hints_list index_hint_definition
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_index_hints_list);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

opt_index_hints_list:
      /* empty */
      {
          $$= NULL;
      }
    | index_hints_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_index_hints_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

opt_key_definition:
      opt_index_hints_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_key_definition);
        }
      }
    ;

opt_key_usage_list:
      /* empty */
      {
          $$= NULL;
      }
    | key_usage_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_key_usage_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

key_usage_element:
      ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_key_usage_element);
        }
      }
    | PRIMARY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_key_usage_element);
        }
      }
    ;

key_usage_list:
      key_usage_element
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_key_usage_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | key_usage_list ',' key_usage_element
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_key_usage_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

using_list:
      ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_using_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | using_list ',' ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_using_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

interval:
      interval_time_stamp
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_interval);
        }
      }
    | DAY_HOUR_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_interval);
        }
      }
    | DAY_MICROSECOND_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_interval);
        }
      }
    | DAY_MINUTE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_interval);
        }
      }
    | DAY_SECOND_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_interval);
        }
      }
    | HOUR_MICROSECOND_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_interval);
        }
      }
    | HOUR_MINUTE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_interval);
        }
      }
    | HOUR_SECOND_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_interval);
        }
      }
    | MINUTE_MICROSECOND_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_interval);
        }
      }
    | MINUTE_SECOND_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_interval);
        }
      }
    | SECOND_MICROSECOND_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_interval);
        }
      }
    | YEAR_MONTH_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_interval);
        }
      }
    ;

interval_time_stamp:
      DAY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_interval_time_stamp);
        }
      }
    | WEEK_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_interval_time_stamp);
        }
      }
    | HOUR_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_interval_time_stamp);
        }
      }
    | MINUTE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_interval_time_stamp);
        }
      }
    | MONTH_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_interval_time_stamp);
        }
      }
    | QUARTER_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_interval_time_stamp);
        }
      }
    | SECOND_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_interval_time_stamp);
        }
      }
    | MICROSECOND_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_interval_time_stamp);
        }
      }
    | YEAR_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_interval_time_stamp);
        }
      }
    ;

date_time_type:
      DATE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_date_time_type);
        }
      }
    | TIME_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_date_time_type);
        }
      }
    | TIMESTAMP
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_date_time_type);
        }
      }
    | DATETIME
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_date_time_type);
        }
      }
    ;

table_alias:
      /* empty */
      {
          $$= NULL;
      }
    | AS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_table_alias);
        }
      }
    | EQ
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_table_alias);
        }
      }
    ;

opt_table_alias:
      /* empty */
      {
          $$= NULL;
      }
    | table_alias ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_table_alias);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

opt_all:
      /* empty */
      {
          $$= NULL;
      }
    | ALL
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_all);
        }
      }
    ;

where_clause:
      /* empty */
      {
          $$= NULL;
      }
    | WHERE expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_where_clause);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_WHERE));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

having_clause:
      /* empty */
      {
          $$= NULL;
      }
    | HAVING expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_having_clause);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_HAVING));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

opt_escape:
      ESCAPE_SYM simple_expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_escape);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ESCAPE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | /* empty */
      {
          $$= NULL;
      }
    ;

group_clause:
      /* empty */
      {
          $$= NULL;
      }
    | GROUP_SYM BY group_list olap_opt
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_group_clause);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_GROUP_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_BY));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

group_list:
      group_list ',' order_ident order_dir
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_group_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | order_ident order_dir
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_group_list);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

olap_opt:
      /* empty */
      {
          $$= NULL;
      }
    | WITH CUBE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_olap_opt);
        }
      }
    | WITH ROLLUP_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_olap_opt);
        }
      }
    ;

alter_order_clause:
      ORDER_SYM BY alter_order_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_order_clause);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ORDER_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_BY));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

alter_order_list:
      alter_order_list ',' alter_order_item
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_alter_order_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | alter_order_item
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_order_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

alter_order_item:
      simple_ident_nospvar order_dir
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_alter_order_item);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

opt_order_clause:
      /* empty */
      {
          $$= NULL;
      }
    | order_clause
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_order_clause);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

order_clause:
      ORDER_SYM BY order_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_order_clause);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ORDER_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_BY));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

order_list:
      order_list ',' order_ident order_dir
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_order_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | order_ident order_dir
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_order_list);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

order_dir:
      /* empty */
      {
          $$= NULL;
      }
    | ASC
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_order_dir);
        }
      }
    | DESC
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_order_dir);
        }
      }
    ;

opt_limit_clause_init:
      /* empty */
      {
          $$= NULL;
      }
    | limit_clause
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_limit_clause_init);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

opt_limit_clause:
      /* empty */
      {
          $$= NULL;
      }
    | limit_clause
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_limit_clause);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

limit_clause:
      LIMIT limit_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_limit_clause);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_LIMIT));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

limit_options:
      limit_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_limit_options);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | limit_option ',' limit_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_limit_options);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | limit_option OFFSET_SYM limit_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_limit_options);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_OFFSET_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

limit_option:
      ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_limit_option);
        }
      }
    | param_marker
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_limit_option);
        }
      }
    | ULONGLONG_NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_limit_option);
        }
      }
    | LONG_NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_limit_option);
        }
      }
    | NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_limit_option);
        }
      }
    ;

delete_limit_clause:
      /* empty */
      {
          $$= NULL;
      }
    | LIMIT limit_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_delete_limit_clause);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_LIMIT));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

ulong_num:
      NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_ulong_num);
        }
      }
    | HEX_NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_ulong_num);
        }
      }
    | LONG_NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_ulong_num);
        }
      }
    | ULONGLONG_NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_ulong_num);
        }
      }
    | DECIMAL_NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_ulong_num);
        }
      }
    | FLOAT_NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_ulong_num);
        }
      }
    ;

real_ulong_num:
      NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_real_ulong_num);
        }
      }
    | HEX_NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_real_ulong_num);
        }
      }
    | LONG_NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_real_ulong_num);
        }
      }
    | ULONGLONG_NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_real_ulong_num);
        }
      }
    | dec_num_error
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_real_ulong_num);
        }
      }
    ;

ulonglong_num:
      NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_ulonglong_num);
        }
      }
    | ULONGLONG_NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_ulonglong_num);
        }
      }
    | LONG_NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_ulonglong_num);
        }
      }
    | DECIMAL_NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_ulonglong_num);
        }
      }
    | FLOAT_NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_ulonglong_num);
        }
      }
    ;

real_ulonglong_num:
      NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_real_ulonglong_num);
        }
      }
    | ULONGLONG_NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_real_ulonglong_num);
        }
      }
    | LONG_NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_real_ulonglong_num);
        }
      }
    | dec_num_error
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_real_ulonglong_num);
        }
      }
    ;

dec_num_error:
      dec_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_dec_num_error);
        }
      }
    ;

dec_num:
      DECIMAL_NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_dec_num);
        }
      }
    | FLOAT_NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_dec_num);
        }
      }
    ;

procedure_analyse_clause:
      /* empty */
      {
          $$= NULL;
      }
    | PROCEDURE_SYM ANALYSE_SYM '(' opt_procedure_analyse_params ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_procedure_analyse_clause);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PROCEDURE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_ANALYSE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_40));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_41));
        }
      }
    ;

opt_procedure_analyse_params:
      /* empty */
      {
          $$= NULL;
      }
    | procedure_analyse_param
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_procedure_analyse_params);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | procedure_analyse_param ',' procedure_analyse_param
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_procedure_analyse_params);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

procedure_analyse_param:
      NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_procedure_analyse_param);
        }
      }
    ;

select_var_list_init:
      select_var_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_var_list_init);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

select_var_list:
      select_var_list ',' select_var_ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_select_var_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | select_var_ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_var_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

select_var_ident:
      '@' ident_or_text
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_var_ident);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_64));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | ident_or_text
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_select_var_ident);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

into:
      INTO into_destination
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_into);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_INTO));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

into_destination:
      OUTFILE TEXT_STRING_filesystem opt_load_data_charset opt_field_term opt_line_term
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_into_destination);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_OUTFILE));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    | DUMPFILE TEXT_STRING_filesystem
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_into_destination);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DUMPFILE));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | select_var_list_init
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_into_destination);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

do:
      DO_SYM expr_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_do);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DO_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

drop:
      DROP opt_temporary table_or_tables if_exists table_list opt_restrict
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_drop);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DROP));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, $6);
        }
      }
    | DROP INDEX_SYM ident ON table_ident opt_index_lock_algorithm
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_drop);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DROP));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_INDEX_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_ON));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, $6);
        }
      }
    | DROP DATABASE if_exists ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_drop);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DROP));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_DATABASE));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | DROP FUNCTION_SYM if_exists ident '.' ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_drop);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DROP));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_FUNCTION_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_46));
          mysql_parser::add_ast_child_node($$, $6);
        }
      }
    | DROP FUNCTION_SYM if_exists ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_drop);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DROP));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_FUNCTION_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | DROP PROCEDURE_SYM if_exists sp_name
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_drop);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DROP));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_PROCEDURE_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | DROP USER clear_privileges user_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_drop);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DROP));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_USER));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | DROP VIEW_SYM if_exists table_list opt_restrict
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_drop);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DROP));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_VIEW_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    | DROP EVENT_SYM if_exists sp_name
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_drop);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DROP));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EVENT_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | DROP TRIGGER_SYM if_exists sp_name
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_drop);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DROP));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_TRIGGER_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | DROP TABLESPACE tablespace_name drop_ts_options_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_drop);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DROP));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_TABLESPACE));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | DROP LOGFILE_SYM GROUP_SYM logfile_group_name drop_ts_options_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_drop);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DROP));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_LOGFILE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_GROUP_SYM));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    | DROP SERVER_SYM if_exists ident_or_text
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_drop);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DROP));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_SERVER_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

table_list:
      table_name
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_table_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | table_list ',' table_name
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_table_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

table_name:
      table_ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_table_name);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

table_name_with_opt_use_partition:
      table_ident opt_use_partition
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_table_name_with_opt_use_partition);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

table_alias_ref_list:
      table_alias_ref
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_table_alias_ref_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | table_alias_ref_list ',' table_alias_ref
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_table_alias_ref_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

table_alias_ref:
      table_ident_opt_wild
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_table_alias_ref);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

if_exists:
      /* empty */
      {
          $$= NULL;
      }
    | IF EXISTS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_if_exists);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_IF));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EXISTS));
        }
      }
    ;

opt_temporary:
      /* empty */
      {
          $$= NULL;
      }
    | TEMPORARY
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_temporary);
        }
      }
    ;

drop_ts_options_list:
      /* empty */
      {
          $$= NULL;
      }
    | drop_ts_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_drop_ts_options_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

drop_ts_options:
      drop_ts_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_drop_ts_options);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | drop_ts_options drop_ts_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_drop_ts_options);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | drop_ts_options_list ',' drop_ts_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_drop_ts_options);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

drop_ts_option:
      opt_ts_engine
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_drop_ts_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | ts_wait
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_drop_ts_option);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

insert:
      INSERT insert_lock_option opt_ignore insert2 insert_field_spec opt_insert_update
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_insert);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_INSERT));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, $6);
        }
      }
    ;

replace:
      REPLACE replace_lock_option insert2 insert_field_spec
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_replace);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_REPLACE));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

insert_lock_option:
      /* empty */
      {
          $$= NULL;
      }
    | LOW_PRIORITY
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_insert_lock_option);
        }
      }
    | DELAYED_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_insert_lock_option);
        }
      }
    | HIGH_PRIORITY
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_insert_lock_option);
        }
      }
    ;

replace_lock_option:
      opt_low_priority
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_replace_lock_option);
        }
      }
    | DELAYED_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_replace_lock_option);
        }
      }
    ;

insert2:
      INTO insert_table
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_insert2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_INTO));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | insert_table
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_insert2);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

insert_table:
      table_name_with_opt_use_partition
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_insert_table);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

insert_field_spec:
      insert_values
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_insert_field_spec);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | '(' ')' insert_values
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_insert_field_spec);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_41));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | '(' fields ')' insert_values
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_insert_field_spec);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | SET ident_eq_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_insert_field_spec);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SET));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

fields:
      fields ',' insert_ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_fields);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | insert_ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_fields);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

insert_values:
      VALUES values_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_insert_values);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_VALUES));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | VALUE_SYM values_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_insert_values);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_VALUE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | create_select union_clause
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_insert_values);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | '(' create_select ')' union_opt
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_insert_values);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

values_list:
      values_list ',' no_braces
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_values_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | no_braces
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_values_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

ident_eq_list:
      ident_eq_list ',' ident_eq_value
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_ident_eq_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | ident_eq_value
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ident_eq_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

ident_eq_value:
      simple_ident_nospvar equal expr_or_default
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_ident_eq_value);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

equal:
      EQ
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_equal);
        }
      }
    | SET_VAR
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_equal);
        }
      }
    ;

opt_equal:
      /* empty */
      {
          $$= NULL;
      }
    | equal
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_equal);
        }
      }
    ;

no_braces:
      '(' opt_values ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_no_braces);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
        }
      }
    ;

opt_values:
      /* empty */
      {
          $$= NULL;
      }
    | values
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_values);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

values:
      values ',' expr_or_default
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_values);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | expr_or_default
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_values);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

expr_or_default:
      expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_expr_or_default);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | DEFAULT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_expr_or_default);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DEFAULT));
        }
      }
    ;

opt_insert_update:
      /* empty */
      {
          $$= NULL;
      }
    | ON DUPLICATE_SYM KEY_SYM UPDATE_SYM insert_update_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_insert_update);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ON));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_DUPLICATE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_KEY_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_UPDATE_SYM));
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    ;

update:
      UPDATE_SYM opt_low_priority opt_ignore join_table_list SET update_list where_clause opt_order_clause delete_limit_clause
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_update);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_UPDATE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_SET));
          mysql_parser::add_ast_child_node($$, $6);
          mysql_parser::add_ast_child_node($$, $7);
          mysql_parser::add_ast_child_node($$, $8);
          mysql_parser::add_ast_child_node($$, $9);
        }
      }
    ;

update_list:
      update_list ',' update_elem
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_update_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | update_elem
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_update_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

update_elem:
      simple_ident_nospvar equal expr_or_default
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_update_elem);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

insert_update_list:
      insert_update_list ',' insert_update_elem
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_insert_update_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | insert_update_elem
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_insert_update_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

insert_update_elem:
      simple_ident_nospvar equal expr_or_default
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_insert_update_elem);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

opt_low_priority:
      /* empty */
      {
          $$= NULL;
      }
    | LOW_PRIORITY
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_low_priority);
        }
      }
    ;

delete:
      DELETE_SYM opt_delete_options single_multi
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_delete);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DELETE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

single_multi:
      FROM table_ident opt_use_partition where_clause opt_order_clause delete_limit_clause
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_single_multi);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_FROM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, $6);
        }
      }
    | table_wild_list FROM join_table_list where_clause
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_single_multi);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_FROM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | FROM table_alias_ref_list USING join_table_list where_clause
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_single_multi);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_FROM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_USING));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    ;

table_wild_list:
      table_wild_one
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_table_wild_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | table_wild_list ',' table_wild_one
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_table_wild_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

table_wild_one:
      ident opt_wild
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_table_wild_one);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | ident '.' ident opt_wild
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_table_wild_one);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_46));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

opt_wild:
      /* empty */
      {
          $$= NULL;
      }
    | '.' '*'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_wild);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_46));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_42));
        }
      }
    ;

opt_delete_options:
      /* empty */
      {
          $$= NULL;
      }
    | opt_delete_option opt_delete_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_delete_options);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::merge_ast_child_nodes($$, $2);
        }
      }
    ;

opt_delete_option:
      QUICK
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_delete_option);
        }
      }
    | LOW_PRIORITY
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_delete_option);
        }
      }
    | IGNORE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_delete_option);
        }
      }
    ;

truncate:
      TRUNCATE_SYM opt_table_sym table_name
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_truncate);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TRUNCATE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

opt_table_sym:
      /* empty */
      {
          $$= NULL;
      }
    | TABLE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_table_sym);
        }
      }
    ;

opt_profile_defs:
      /* empty */
      {
          $$= NULL;
      }
    | profile_defs
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_profile_defs);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

profile_defs:
      profile_def
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_profile_defs);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | profile_defs ',' profile_def
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_profile_defs);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

profile_def:
      CPU_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_profile_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CPU_SYM));
        }
      }
    | MEMORY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_profile_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MEMORY_SYM));
        }
      }
    | BLOCK_SYM IO_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_profile_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_BLOCK_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_IO_SYM));
        }
      }
    | CONTEXT_SYM SWITCHES_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_profile_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CONTEXT_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_SWITCHES_SYM));
        }
      }
    | PAGE_SYM FAULTS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_profile_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PAGE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_FAULTS_SYM));
        }
      }
    | IPC_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_profile_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_IPC_SYM));
        }
      }
    | SWAPS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_profile_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SWAPS_SYM));
        }
      }
    | SOURCE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_profile_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SOURCE_SYM));
        }
      }
    | ALL
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_profile_def);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ALL));
        }
      }
    ;

opt_profile_args:
      /* empty */
      {
          $$= NULL;
      }
    | FOR_SYM QUERY_SYM NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_profile_args);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_FOR_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_QUERY_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_NUM));
        }
      }
    ;

show:
      SHOW show_param
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SHOW));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

show_param:
      DATABASES wild_and_where
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DATABASES));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | opt_full TABLES opt_db wild_and_where
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_TABLES));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | opt_full TRIGGERS_SYM opt_db wild_and_where
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_TRIGGERS_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | EVENTS_SYM opt_db wild_and_where
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_EVENTS_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | TABLE_SYM STATUS_SYM opt_db wild_and_where
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TABLE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_STATUS_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | OPEN_SYM TABLES opt_db wild_and_where
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_OPEN_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_TABLES));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | PLUGINS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PLUGINS_SYM));
        }
      }
    | ENGINE_SYM known_storage_engines show_engine_param
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ENGINE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | ENGINE_SYM ALL show_engine_param
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ENGINE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_ALL));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | opt_full COLUMNS from_or_in table_ident opt_db wild_and_where
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_COLUMNS));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, $6);
        }
      }
    | master_or_binary LOGS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_LOGS_SYM));
        }
      }
    | SLAVE HOSTS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SLAVE));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_HOSTS_SYM));
        }
      }
    | BINLOG_SYM EVENTS_SYM binlog_in binlog_from opt_limit_clause_init
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_BINLOG_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EVENTS_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    | RELAYLOG_SYM EVENTS_SYM binlog_in binlog_from opt_limit_clause_init
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_RELAYLOG_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EVENTS_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    | keys_or_index from_or_in table_ident opt_db where_clause
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    | opt_storage ENGINES_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_ENGINES_SYM));
        }
      }
    | PRIVILEGES
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PRIVILEGES));
        }
      }
    | COUNT_SYM '(' '*' ')' WARNINGS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_COUNT_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_42));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_WARNINGS));
        }
      }
    | COUNT_SYM '(' '*' ')' ERRORS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_COUNT_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_42));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_ERRORS));
        }
      }
    | WARNINGS opt_limit_clause_init
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_WARNINGS));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | ERRORS opt_limit_clause_init
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ERRORS));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | PROFILES_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PROFILES_SYM));
        }
      }
    | PROFILE_SYM opt_profile_defs opt_profile_args opt_limit_clause_init
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PROFILE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | opt_var_type STATUS_SYM wild_and_where
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_STATUS_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | opt_full PROCESSLIST_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_PROCESSLIST_SYM));
        }
      }
    | opt_var_type VARIABLES wild_and_where
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_VARIABLES));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | charset wild_and_where
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | COLLATION_SYM wild_and_where
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_COLLATION_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | GRANTS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_GRANTS));
        }
      }
    | GRANTS FOR_SYM user
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_GRANTS));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_FOR_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | CREATE DATABASE opt_if_not_exists ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CREATE));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_DATABASE));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | CREATE TABLE_SYM table_ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CREATE));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_TABLE_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | CREATE VIEW_SYM table_ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CREATE));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_VIEW_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | MASTER_SYM STATUS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MASTER_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_STATUS_SYM));
        }
      }
    | SLAVE STATUS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SLAVE));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_STATUS_SYM));
        }
      }
    | CREATE PROCEDURE_SYM sp_name
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CREATE));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_PROCEDURE_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | CREATE FUNCTION_SYM sp_name
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CREATE));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_FUNCTION_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | CREATE TRIGGER_SYM sp_name
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CREATE));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_TRIGGER_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | PROCEDURE_SYM STATUS_SYM wild_and_where
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PROCEDURE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_STATUS_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | FUNCTION_SYM STATUS_SYM wild_and_where
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_FUNCTION_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_STATUS_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | PROCEDURE_SYM CODE_SYM sp_name
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PROCEDURE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_CODE_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | FUNCTION_SYM CODE_SYM sp_name
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_FUNCTION_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_CODE_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | CREATE EVENT_SYM sp_name
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_show_param);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CREATE));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EVENT_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

show_engine_param:
      STATUS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_show_engine_param);
        }
      }
    | MUTEX_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_show_engine_param);
        }
      }
    | LOGS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_show_engine_param);
        }
      }
    ;

master_or_binary:
      MASTER_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_master_or_binary);
        }
      }
    | BINARY
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_master_or_binary);
        }
      }
    ;

opt_storage:
      /* empty */
      {
          $$= NULL;
      }
    | STORAGE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_storage);
        }
      }
    ;

opt_db:
      /* empty */
      {
          $$= NULL;
      }
    | from_or_in ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_db);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

opt_full:
      /* empty */
      {
          $$= NULL;
      }
    | FULL
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_full);
        }
      }
    ;

from_or_in:
      FROM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_from_or_in);
        }
      }
    | IN_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_from_or_in);
        }
      }
    ;

binlog_in:
      /* empty */
      {
          $$= NULL;
      }
    | IN_SYM TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_binlog_in);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_IN_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

binlog_from:
      /* empty */
      {
          $$= NULL;
      }
    | FROM ulonglong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_binlog_from);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_FROM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

wild_and_where:
      /* empty */
      {
          $$= NULL;
      }
    | LIKE TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_wild_and_where);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_LIKE));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | WHERE expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_wild_and_where);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_WHERE));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

describe:
      describe_command table_ident opt_describe_column
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_describe);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | describe_command opt_extended_describe explanable_command
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_describe);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

explanable_command:
      select
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_explanable_command);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | insert
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_explanable_command);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | replace
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_explanable_command);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | update
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_explanable_command);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | delete
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_explanable_command);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

describe_command:
      DESC
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_describe_command);
        }
      }
    | DESCRIBE
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_describe_command);
        }
      }
    ;

opt_extended_describe:
      /* empty */
      {
          $$= NULL;
      }
    | EXTENDED_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_extended_describe);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_EXTENDED_SYM));
        }
      }
    | PARTITIONS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_extended_describe);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PARTITIONS_SYM));
        }
      }
    | FORMAT_SYM EQ ident_or_text
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_extended_describe);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_FORMAT_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

opt_describe_column:
      /* empty */
      {
          $$= NULL;
      }
    | text_string
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_describe_column);
        }
      }
    | ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_describe_column);
        }
      }
    ;

flush:
      FLUSH_SYM opt_no_write_to_binlog flush_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_flush);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_FLUSH_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

flush_options:
      table_or_tables opt_table_list opt_flush_lock
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_flush_options);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | flush_options_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_flush_options);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

opt_flush_lock:
      /* empty */
      {
          $$= NULL;
      }
    | WITH READ_SYM LOCK_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_flush_lock);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_WITH));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_READ_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_LOCK_SYM));
        }
      }
    | FOR_SYM EXPORT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_flush_lock);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_FOR_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EXPORT_SYM));
        }
      }
    ;

flush_options_list:
      flush_options_list ',' flush_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_flush_options_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | flush_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_flush_options_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

flush_option:
      ERROR_SYM LOGS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_flush_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ERROR_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_LOGS_SYM));
        }
      }
    | ENGINE_SYM LOGS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_flush_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ENGINE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_LOGS_SYM));
        }
      }
    | GENERAL LOGS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_flush_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_GENERAL));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_LOGS_SYM));
        }
      }
    | SLOW LOGS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_flush_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SLOW));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_LOGS_SYM));
        }
      }
    | BINARY LOGS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_flush_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_BINARY));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_LOGS_SYM));
        }
      }
    | RELAY LOGS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_flush_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_RELAY));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_LOGS_SYM));
        }
      }
    | QUERY_SYM CACHE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_flush_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_QUERY_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_CACHE_SYM));
        }
      }
    | HOSTS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_flush_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_HOSTS_SYM));
        }
      }
    | PRIVILEGES
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_flush_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PRIVILEGES));
        }
      }
    | LOGS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_flush_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_LOGS_SYM));
        }
      }
    | STATUS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_flush_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_STATUS_SYM));
        }
      }
    | DES_KEY_FILE
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_flush_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DES_KEY_FILE));
        }
      }
    | RESOURCES
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_flush_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_RESOURCES));
        }
      }
    ;

opt_table_list:
      /* empty */
      {
          $$= NULL;
      }
    | table_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_table_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

reset:
      RESET_SYM reset_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_reset);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_RESET_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

reset_options:
      reset_options ',' reset_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_reset_options);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | reset_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_reset_options);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

reset_option:
      SLAVE slave_reset_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_reset_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SLAVE));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | MASTER_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_reset_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MASTER_SYM));
        }
      }
    | QUERY_SYM CACHE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_reset_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_QUERY_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_CACHE_SYM));
        }
      }
    ;

slave_reset_options:
      /* empty */
      {
          $$= NULL;
      }
    | ALL
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_slave_reset_options);
        }
      }
    ;

purge:
      PURGE purge_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_purge);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PURGE));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

purge_options:
      master_or_binary LOGS_SYM purge_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_purge_options);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_LOGS_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

purge_option:
      TO_SYM TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_purge_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TO_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | BEFORE_SYM expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_purge_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_BEFORE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

kill:
      KILL_SYM kill_option expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_kill);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_KILL_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

kill_option:
      /* empty */
      {
          $$= NULL;
      }
    | CONNECTION_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_kill_option);
        }
      }
    | QUERY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_kill_option);
        }
      }
    ;

use:
      USE_SYM ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_use);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_USE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

load:
      LOAD data_or_xml load_data_lock opt_local INFILE TEXT_STRING_filesystem opt_duplicate INTO TABLE_SYM table_ident opt_use_partition opt_load_data_charset opt_xml_rows_identified_by opt_field_term opt_line_term opt_ignore_lines opt_field_or_var_spec opt_load_data_set_spec
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_load);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_LOAD));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_INFILE));
          mysql_parser::add_ast_child_node($$, $6);
          mysql_parser::add_ast_child_node($$, $7);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($8, sql::_INTO));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($9, sql::_TABLE_SYM));
          mysql_parser::add_ast_child_node($$, $10);
          mysql_parser::add_ast_child_node($$, $11);
          mysql_parser::add_ast_child_node($$, $12);
          mysql_parser::add_ast_child_node($$, $13);
          mysql_parser::add_ast_child_node($$, $14);
          mysql_parser::add_ast_child_node($$, $15);
          mysql_parser::add_ast_child_node($$, $16);
          mysql_parser::add_ast_child_node($$, $17);
          mysql_parser::add_ast_child_node($$, $18);
        }
      }
    ;

data_or_xml:
      DATA_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_data_or_xml);
        }
      }
    | XML_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_data_or_xml);
        }
      }
    ;

opt_local:
      /* empty */
      {
          $$= NULL;
      }
    | LOCAL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_local);
        }
      }
    ;

load_data_lock:
      /* empty */
      {
          $$= NULL;
      }
    | CONCURRENT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_load_data_lock);
        }
      }
    | LOW_PRIORITY
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_load_data_lock);
        }
      }
    ;

opt_duplicate:
      /* empty */
      {
          $$= NULL;
      }
    | REPLACE
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_duplicate);
        }
      }
    | IGNORE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_duplicate);
        }
      }
    ;

opt_field_term:
      /* empty */
      {
          $$= NULL;
      }
    | COLUMNS field_term_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_field_term);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_COLUMNS));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

field_term_list:
      field_term_list field_term
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_field_term_list);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | field_term
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_field_term_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

field_term:
      TERMINATED BY text_string
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_field_term);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TERMINATED));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_BY));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | OPTIONALLY ENCLOSED BY text_string
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_field_term);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_OPTIONALLY));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_ENCLOSED));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_BY));
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | ENCLOSED BY text_string
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_field_term);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ENCLOSED));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_BY));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | ESCAPED BY text_string
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_field_term);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ESCAPED));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_BY));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

opt_line_term:
      /* empty */
      {
          $$= NULL;
      }
    | LINES line_term_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_line_term);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_LINES));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

line_term_list:
      line_term_list line_term
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_line_term_list);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | line_term
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_line_term_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

line_term:
      TERMINATED BY text_string
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_line_term);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TERMINATED));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_BY));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | STARTING BY text_string
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_line_term);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_STARTING));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_BY));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

opt_xml_rows_identified_by:
      /* empty */
      {
          $$= NULL;
      }
    | ROWS_SYM IDENTIFIED_SYM BY text_string
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_xml_rows_identified_by);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ROWS_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_IDENTIFIED_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_BY));
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

opt_ignore_lines:
      /* empty */
      {
          $$= NULL;
      }
    | IGNORE_SYM NUM lines_or_rows
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_ignore_lines);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_IGNORE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_NUM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

lines_or_rows:
      LINES
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_lines_or_rows);
        }
      }
    | ROWS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_lines_or_rows);
        }
      }
    ;

opt_field_or_var_spec:
      /* empty */
      {
          $$= NULL;
      }
    | '(' fields_or_vars ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_field_or_var_spec);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
        }
      }
    | '(' ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_field_or_var_spec);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_41));
        }
      }
    ;

fields_or_vars:
      fields_or_vars ',' field_or_var
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_fields_or_vars);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | field_or_var
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_fields_or_vars);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

field_or_var:
      simple_ident_nospvar
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_field_or_var);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | '@' ident_or_text
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_field_or_var);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_64));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

opt_load_data_set_spec:
      /* empty */
      {
          $$= NULL;
      }
    | SET load_data_set_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_load_data_set_spec);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SET));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

load_data_set_list:
      load_data_set_list ',' load_data_set_elem
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_load_data_set_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | load_data_set_elem
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_load_data_set_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

load_data_set_elem:
      simple_ident_nospvar equal remember_name expr_or_default remember_end
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_load_data_set_elem);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    ;

text_literal:
      TEXT_STRING
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_text_literal);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TEXT_STRING));
        }
      }
    | NCHAR_STRING
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_text_literal);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_NCHAR_STRING));
        }
      }
    | UNDERSCORE_CHARSET TEXT_STRING
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_text_literal);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_UNDERSCORE_CHARSET));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_TEXT_STRING));
        }
      }
    | text_literal TEXT_STRING_literal
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_text_literal);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

text_string:
      TEXT_STRING_literal
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_text_string);
        }
      }
    | HEX_NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_text_string);
        }
      }
    | BIN_NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_text_string);
        }
      }
    ;

param_marker:
      PARAM_MARKER
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_param_marker);
        }
      }
    ;

signed_literal:
      literal
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_signed_literal);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | '+' NUM_literal
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_signed_literal);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_43));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | '-' NUM_literal
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_signed_literal);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_45));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

literal:
      text_literal
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_literal);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | NUM_literal
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_literal);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | temporal_literal
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_literal);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | NULL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_literal);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_NULL_SYM));
        }
      }
    | FALSE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_literal);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_FALSE_SYM));
        }
      }
    | TRUE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_literal);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TRUE_SYM));
        }
      }
    | HEX_NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_literal);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_HEX_NUM));
        }
      }
    | BIN_NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_literal);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_BIN_NUM));
        }
      }
    | UNDERSCORE_CHARSET HEX_NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_literal);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_UNDERSCORE_CHARSET));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_HEX_NUM));
        }
      }
    | UNDERSCORE_CHARSET BIN_NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_literal);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_UNDERSCORE_CHARSET));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_BIN_NUM));
        }
      }
    ;

NUM_literal:
      NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_NUM_literal);
        }
      }
    | LONG_NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_NUM_literal);
        }
      }
    | ULONGLONG_NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_NUM_literal);
        }
      }
    | DECIMAL_NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_NUM_literal);
        }
      }
    | FLOAT_NUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_NUM_literal);
        }
      }
    ;

temporal_literal:
      DATE_SYM TEXT_STRING
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_temporal_literal);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DATE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_TEXT_STRING));
        }
      }
    | TIME_SYM TEXT_STRING
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_temporal_literal);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TIME_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_TEXT_STRING));
        }
      }
    | TIMESTAMP TEXT_STRING
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_temporal_literal);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TIMESTAMP));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_TEXT_STRING));
        }
      }
    ;

insert_ident:
      simple_ident_nospvar
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_insert_ident);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | table_wild
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_insert_ident);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

table_wild:
      ident '.' '*'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_table_wild);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_46));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_42));
        }
      }
    | ident '.' ident '.' '*'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_table_wild);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_46));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_46));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_42));
        }
      }
    ;

order_ident:
      expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_order_ident);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

simple_ident:
      ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_ident);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | simple_ident_q
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_ident);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

simple_ident_nospvar:
      ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_ident_nospvar);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | simple_ident_q
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_ident_nospvar);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

simple_ident_q:
      ident '.' ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_ident_q);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_46));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | '.' ident '.' ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_ident_q);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_46));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_46));
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | ident '.' ident '.' ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_simple_ident_q);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_46));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_46));
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    ;

field_ident:
      ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_field_ident);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | ident '.' ident '.' ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_field_ident);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_46));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_46));
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    | ident '.' ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_field_ident);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_46));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | '.' ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_field_ident);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_46));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

table_ident:
      ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_table_ident);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | ident '.' ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_table_ident);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_46));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | '.' ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_table_ident);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_46));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

table_ident_opt_wild:
      ident opt_wild
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_table_ident_opt_wild);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | ident '.' ident opt_wild
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_table_ident_opt_wild);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_46));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

table_ident_nodb:
      ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_table_ident_nodb);
        }
      }
    ;

IDENT_sys:
      IDENT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_IDENT_sys);
        }
      }
    | IDENT_QUOTED
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_IDENT_sys);
        }
      }
    ;

TEXT_STRING_sys_nonewline:
      TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_TEXT_STRING_sys_nonewline);
        }
      }
    ;

TEXT_STRING_sys:
      TEXT_STRING
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_TEXT_STRING_sys);
        }
      }
    ;

TEXT_STRING_literal:
      TEXT_STRING
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_TEXT_STRING_literal);
        }
      }
    ;

TEXT_STRING_filesystem:
      TEXT_STRING
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_TEXT_STRING_filesystem);
        }
      }
    ;

ident:
      IDENT_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_ident);
        }
      }
    | keyword
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_ident);
        }
      }
    ;

label_ident:
      IDENT_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_label_ident);
        }
      }
    | keyword_sp
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_label_ident);
        }
      }
    ;

ident_or_text:
      ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_ident_or_text);
        }
      }
    | TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_ident_or_text);
        }
      }
    | LEX_HOSTNAME
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_ident_or_text);
        }
      }
    ;

user:
      ident_or_text
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_user);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | ident_or_text '@' ident_or_text
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_user);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_64));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | CURRENT_USER optional_braces
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_user);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CURRENT_USER));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

keyword:
      keyword_sp
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | ASCII_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | BACKUP_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | BEGIN_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | BYTE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | CACHE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | CHARSET
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | CHECKSUM_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | CLOSE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | COMMENT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | COMMIT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | CONTAINS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | DEALLOCATE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | DO_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | END
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | EXECUTE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | FLUSH_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | FORMAT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | HANDLER_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | HELP_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | HOST_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | INSTALL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | LANGUAGE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | NO_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | OPEN_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | OPTIONS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | OWNER_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | PARSER_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | PORT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | PREPARE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | REMOVE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | REPAIR
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | RESET_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | RESTORE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | ROLLBACK_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | SAVEPOINT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | SECURITY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | SERVER_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | SIGNED_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | SOCKET_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | SLAVE
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | SONAME_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | START_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | STOP_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | TRUNCATE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | UNICODE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | UNINSTALL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | WRAPPER_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | XA_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    | UPGRADE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword);
        }
      }
    ;

keyword_sp:
      ACTION
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | ADDDATE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | AFTER_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | AGAINST
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | AGGREGATE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | ALGORITHM_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | ANALYSE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | ANY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | AT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | AUTO_INC
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | AUTOEXTEND_SIZE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | AVG_ROW_LENGTH
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | AVG_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | BINLOG_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | BIT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | BLOCK_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | BOOL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | BOOLEAN_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | BTREE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | CASCADED
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | CATALOG_NAME_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | CHAIN_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | CHANGED
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | CIPHER_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | CLIENT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | CLASS_ORIGIN_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | COALESCE
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | CODE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | COLLATION_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | COLUMN_NAME_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | COLUMN_FORMAT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | COLUMNS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | COMMITTED_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | COMPACT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | COMPLETION_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | COMPRESSED_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | CONCURRENT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | CONNECTION_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | CONSISTENT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | CONSTRAINT_CATALOG_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | CONSTRAINT_SCHEMA_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | CONSTRAINT_NAME_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | CONTEXT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | CPU_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | CUBE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | CURRENT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | CURSOR_NAME_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | DATA_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | DATAFILE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | DATETIME
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | DATE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | DAY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | DEFAULT_AUTH_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | DEFINER_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | DELAY_KEY_WRITE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | DES_KEY_FILE
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | DIAGNOSTICS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | DIRECTORY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | DISABLE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | DISCARD
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | DISK_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | DUMPFILE
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | DUPLICATE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | DYNAMIC_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | ENDS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | ENUM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | ENGINE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | ENGINES_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | ERROR_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | ERRORS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | ESCAPE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | EVENT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | EVENTS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | EVERY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | EXCHANGE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | EXPANSION_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | EXPIRE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | EXPORT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | EXTENDED_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | EXTENT_SIZE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | FAULTS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | FAST_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | FOUND_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | ENABLE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | FULL
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | FILE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | FIRST_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | FIXED_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | GENERAL
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | GEOMETRY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | GEOMETRYCOLLECTION
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | GET_FORMAT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | GRANTS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | GLOBAL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | HASH_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | HOSTS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | HOUR_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | IDENTIFIED_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | IGNORE_SERVER_IDS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | INVOKER_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | IMPORT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | INDEXES
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | INITIAL_SIZE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | IO_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | IPC_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | ISOLATION
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | ISSUER_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | INSERT_METHOD
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | KEY_BLOCK_SIZE
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | LAST_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | LEAVES
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | LESS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | LEVEL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | LINESTRING
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | LIST_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | LOCAL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | LOCKS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | LOGFILE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | LOGS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MAX_ROWS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MASTER_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MASTER_HEARTBEAT_PERIOD_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MASTER_HOST_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MASTER_PORT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MASTER_LOG_FILE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MASTER_LOG_POS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MASTER_USER_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MASTER_PASSWORD_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MASTER_SERVER_ID_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MASTER_CONNECT_RETRY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MASTER_RETRY_COUNT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MASTER_DELAY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MASTER_SSL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MASTER_SSL_CA_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MASTER_SSL_CAPATH_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MASTER_SSL_CERT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MASTER_SSL_CIPHER_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MASTER_SSL_CRL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MASTER_SSL_CRLPATH_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MASTER_SSL_KEY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MASTER_AUTO_POSITION_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MAX_CONNECTIONS_PER_HOUR
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MAX_QUERIES_PER_HOUR
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MAX_SIZE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MAX_UPDATES_PER_HOUR
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MAX_USER_CONNECTIONS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MEDIUM_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MEMORY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MERGE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MESSAGE_TEXT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MICROSECOND_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MIGRATE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MINUTE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MIN_ROWS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MODIFY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MODE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MONTH_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MULTILINESTRING
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MULTIPOINT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MULTIPOLYGON
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MUTEX_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | MYSQL_ERRNO_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | NAME_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | NAMES_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | NATIONAL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | NCHAR_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | NDBCLUSTER_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | NEXT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | NEW_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | NO_WAIT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | NODEGROUP_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | NONE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | NUMBER_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | NVARCHAR_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | OFFSET_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | OLD_PASSWORD
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | ONE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | ONLY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | PACK_KEYS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | PAGE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | PARTIAL
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | PARTITIONING_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | PARTITIONS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | PASSWORD
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | PHASE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | PLUGIN_DIR_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | PLUGIN_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | PLUGINS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | POINT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | POLYGON
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | PRESERVE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | PREV_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | PRIVILEGES
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | PROCESS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | PROCESSLIST_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | PROFILE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | PROFILES_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | PROXY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | QUARTER_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | QUERY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | QUICK
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | READ_ONLY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | REBUILD_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | RECOVER_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | REDO_BUFFER_SIZE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | REDOFILE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | REDUNDANT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | RELAY
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | RELAYLOG_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | RELAY_LOG_FILE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | RELAY_LOG_POS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | RELAY_THREAD
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | RELOAD
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | REORGANIZE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | REPEATABLE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | REPLICATION
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | RESOURCES
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | RESUME_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | RETURNED_SQLSTATE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | RETURNS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | REVERSE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | ROLLUP_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | ROUTINE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | ROWS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | ROW_COUNT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | ROW_FORMAT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | ROW_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | RTREE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | SCHEDULE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | SCHEMA_NAME_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | SECOND_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | SERIAL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | SERIALIZABLE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | SESSION_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | SIMPLE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | SHARE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | SHUTDOWN
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | SLOW
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | SNAPSHOT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | SOUNDS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | SOURCE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | SQL_AFTER_GTIDS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | SQL_AFTER_MTS_GAPS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | SQL_BEFORE_GTIDS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | SQL_CACHE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | SQL_BUFFER_RESULT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | SQL_NO_CACHE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | SQL_THREAD
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | STARTS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | STATS_AUTO_RECALC_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | STATS_PERSISTENT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | STATS_SAMPLE_PAGES_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | STATUS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | STORAGE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | STRING_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | SUBCLASS_ORIGIN_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | SUBDATE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | SUBJECT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | SUBPARTITION_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | SUBPARTITIONS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | SUPER_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | SUSPEND_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | SWAPS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | SWITCHES_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | TABLE_NAME_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | TABLES
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | TABLE_CHECKSUM_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | TABLESPACE
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | TEMPORARY
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | TEMPTABLE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | TEXT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | THAN_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | TRANSACTION_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | TRIGGERS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | TIMESTAMP
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | TIMESTAMP_ADD
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | TIMESTAMP_DIFF
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | TIME_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | TYPES_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | TYPE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | UDF_RETURNS_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | FUNCTION_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | UNCOMMITTED_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | UNDEFINED_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | UNDO_BUFFER_SIZE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | UNDOFILE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | UNKNOWN_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | UNTIL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | USER
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | USE_FRM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | VARIABLES
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | VIEW_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | VALUE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | WARNINGS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | WAIT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | WEEK_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | WORK_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | WEIGHT_STRING_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | X509_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | XML_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    | YEAR_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_keyword_sp);
        }
      }
    ;

set:
      SET start_option_value_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_set);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SET));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

start_option_value_list:
      option_value_no_option_type option_value_list_continued
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_start_option_value_list);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | TRANSACTION_SYM transaction_characteristics
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_start_option_value_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TRANSACTION_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | option_type start_option_value_list_following_option_type
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_start_option_value_list);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

start_option_value_list_following_option_type:
      option_value_following_option_type option_value_list_continued
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_start_option_value_list_following_option_type);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | TRANSACTION_SYM transaction_characteristics
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_start_option_value_list_following_option_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TRANSACTION_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

option_value_list_continued:
      /* empty */
      {
          $$= NULL;
      }
    | ',' option_value_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_option_value_list_continued);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_44));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

option_value_list:
      option_value
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_option_value_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | option_value_list ',' option_value
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_option_value_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

option_value:
      option_type option_value_following_option_type
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_option_value);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | option_value_no_option_type
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_option_value);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

option_type:
      GLOBAL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_option_type);
        }
      }
    | LOCAL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_option_type);
        }
      }
    | SESSION_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_option_type);
        }
      }
    ;

opt_var_type:
      /* empty */
      {
          $$= NULL;
      }
    | GLOBAL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_var_type);
        }
      }
    | LOCAL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_var_type);
        }
      }
    | SESSION_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_var_type);
        }
      }
    ;

opt_var_ident_type:
      /* empty */
      {
          $$= NULL;
      }
    | GLOBAL_SYM '.'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_var_ident_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_GLOBAL_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_46));
        }
      }
    | LOCAL_SYM '.'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_var_ident_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_LOCAL_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_46));
        }
      }
    | SESSION_SYM '.'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_var_ident_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SESSION_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_46));
        }
      }
    ;

option_value_following_option_type:
      internal_variable_name equal set_expr_or_default
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_option_value_following_option_type);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

option_value_no_option_type:
      internal_variable_name equal set_expr_or_default
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_option_value_no_option_type);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | '@' ident_or_text equal expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_option_value_no_option_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_64));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | '@' '@' opt_var_ident_type internal_variable_name equal set_expr_or_default
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_option_value_no_option_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_64));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_64));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, $6);
        }
      }
    | charset old_or_new_charset_name_or_default
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_option_value_no_option_type);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | NAMES_SYM equal expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_option_value_no_option_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_NAMES_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | NAMES_SYM charset_name_or_default opt_collate
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_option_value_no_option_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_NAMES_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | PASSWORD equal text_or_password
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_option_value_no_option_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PASSWORD));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | PASSWORD FOR_SYM user equal text_or_password
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_option_value_no_option_type);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PASSWORD));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_FOR_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    ;

internal_variable_name:
      ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_internal_variable_name);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | ident '.' ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_internal_variable_name);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_46));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | DEFAULT '.' ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_internal_variable_name);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DEFAULT));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_46));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

transaction_characteristics:
      transaction_access_mode
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_transaction_characteristics);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | isolation_level
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_transaction_characteristics);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | transaction_access_mode ',' isolation_level
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_transaction_characteristics);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | isolation_level ',' transaction_access_mode
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_transaction_characteristics);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

transaction_access_mode:
      transaction_access_mode_types
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_transaction_access_mode);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

isolation_level:
      ISOLATION LEVEL_SYM isolation_types
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_isolation_level);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ISOLATION));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_LEVEL_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

transaction_access_mode_types:
      READ_SYM ONLY_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_transaction_access_mode_types);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_READ_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_ONLY_SYM));
        }
      }
    | READ_SYM WRITE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_transaction_access_mode_types);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_READ_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_WRITE_SYM));
        }
      }
    ;

isolation_types:
      READ_SYM UNCOMMITTED_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_isolation_types);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_READ_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_UNCOMMITTED_SYM));
        }
      }
    | READ_SYM COMMITTED_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_isolation_types);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_READ_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_COMMITTED_SYM));
        }
      }
    | REPEATABLE_SYM READ_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_isolation_types);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_REPEATABLE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_READ_SYM));
        }
      }
    | SERIALIZABLE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_isolation_types);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SERIALIZABLE_SYM));
        }
      }
    ;

text_or_password:
      TEXT_STRING
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_text_or_password);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TEXT_STRING));
        }
      }
    | PASSWORD '(' TEXT_STRING ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_text_or_password);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PASSWORD));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_TEXT_STRING));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    | OLD_PASSWORD '(' TEXT_STRING ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_text_or_password);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_OLD_PASSWORD));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_TEXT_STRING));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    ;

set_expr_or_default:
      expr
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_set_expr_or_default);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | DEFAULT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_set_expr_or_default);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DEFAULT));
        }
      }
    | ON
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_set_expr_or_default);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ON));
        }
      }
    | ALL
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_set_expr_or_default);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ALL));
        }
      }
    | BINARY
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_set_expr_or_default);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_BINARY));
        }
      }
    ;

lock:
      LOCK_SYM table_or_tables table_lock_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_lock);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_LOCK_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

table_or_tables:
      TABLE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_table_or_tables);
        }
      }
    | TABLES
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_table_or_tables);
        }
      }
    ;

table_lock_list:
      table_lock
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_table_lock_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | table_lock_list ',' table_lock
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_table_lock_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

table_lock:
      table_ident opt_table_alias lock_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_table_lock);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

lock_option:
      READ_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_lock_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_READ_SYM));
        }
      }
    | WRITE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_lock_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_WRITE_SYM));
        }
      }
    | LOW_PRIORITY WRITE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_lock_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_LOW_PRIORITY));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_WRITE_SYM));
        }
      }
    | READ_SYM LOCAL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_lock_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_READ_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_LOCAL_SYM));
        }
      }
    ;

unlock:
      UNLOCK_SYM table_or_tables
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_unlock);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_UNLOCK_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

handler:
      HANDLER_SYM table_ident OPEN_SYM opt_table_alias
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_handler);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_HANDLER_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_OPEN_SYM));
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | HANDLER_SYM table_ident_nodb CLOSE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_handler);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_HANDLER_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_CLOSE_SYM));
        }
      }
    | HANDLER_SYM table_ident_nodb READ_SYM handler_read_or_scan where_clause opt_limit_clause
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_handler);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_HANDLER_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_READ_SYM));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, $6);
        }
      }
    ;

handler_read_or_scan:
      handler_scan_function
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_handler_read_or_scan);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | ident handler_rkey_function
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_handler_read_or_scan);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

handler_scan_function:
      FIRST_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_handler_scan_function);
        }
      }
    | NEXT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_handler_scan_function);
        }
      }
    ;

handler_rkey_function:
      FIRST_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_handler_rkey_function);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_FIRST_SYM));
        }
      }
    | NEXT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_handler_rkey_function);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_NEXT_SYM));
        }
      }
    | PREV_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_handler_rkey_function);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PREV_SYM));
        }
      }
    | LAST_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_handler_rkey_function);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_LAST_SYM));
        }
      }
    | handler_rkey_mode '(' values ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_handler_rkey_function);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_40));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_41));
        }
      }
    ;

handler_rkey_mode:
      EQ
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_handler_rkey_mode);
        }
      }
    | GE
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_handler_rkey_mode);
        }
      }
    | LE
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_handler_rkey_mode);
        }
      }
    | GT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_handler_rkey_mode);
        }
      }
    | LT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_handler_rkey_mode);
        }
      }
    ;

revoke:
      REVOKE clear_privileges revoke_command
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_revoke);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_REVOKE));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

revoke_command:
      grant_privileges ON opt_table grant_ident FROM grant_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_revoke_command);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_ON));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_FROM));
          mysql_parser::add_ast_child_node($$, $6);
        }
      }
    | grant_privileges ON FUNCTION_SYM grant_ident FROM grant_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_revoke_command);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_ON));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_FUNCTION_SYM));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_FROM));
          mysql_parser::add_ast_child_node($$, $6);
        }
      }
    | grant_privileges ON PROCEDURE_SYM grant_ident FROM grant_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_revoke_command);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_ON));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_PROCEDURE_SYM));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_FROM));
          mysql_parser::add_ast_child_node($$, $6);
        }
      }
    | ALL opt_privileges ',' GRANT OPTION FROM grant_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_revoke_command);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ALL));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_44));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_GRANT));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_OPTION));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_FROM));
          mysql_parser::add_ast_child_node($$, $7);
        }
      }
    | PROXY_SYM ON user FROM grant_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_revoke_command);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PROXY_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_ON));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_FROM));
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    ;

grant:
      GRANT clear_privileges grant_command
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_grant);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_GRANT));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

grant_command:
      grant_privileges ON opt_table grant_ident TO_SYM grant_list require_clause grant_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_grant_command);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_ON));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_TO_SYM));
          mysql_parser::add_ast_child_node($$, $6);
          mysql_parser::add_ast_child_node($$, $7);
          mysql_parser::add_ast_child_node($$, $8);
        }
      }
    | grant_privileges ON FUNCTION_SYM grant_ident TO_SYM grant_list require_clause grant_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_grant_command);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_ON));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_FUNCTION_SYM));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_TO_SYM));
          mysql_parser::add_ast_child_node($$, $6);
          mysql_parser::add_ast_child_node($$, $7);
          mysql_parser::add_ast_child_node($$, $8);
        }
      }
    | grant_privileges ON PROCEDURE_SYM grant_ident TO_SYM grant_list require_clause grant_options
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_grant_command);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_ON));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_PROCEDURE_SYM));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_TO_SYM));
          mysql_parser::add_ast_child_node($$, $6);
          mysql_parser::add_ast_child_node($$, $7);
          mysql_parser::add_ast_child_node($$, $8);
        }
      }
    | PROXY_SYM ON user TO_SYM grant_list opt_grant_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_grant_command);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PROXY_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_ON));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_TO_SYM));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, $6);
        }
      }
    ;

opt_table:
      /* empty */
      {
          $$= NULL;
      }
    | TABLE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_table);
        }
      }
    ;

grant_privileges:
      object_privilege_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_grant_privileges);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | ALL opt_privileges
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_grant_privileges);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ALL));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

opt_privileges:
      /* empty */
      {
          $$= NULL;
      }
    | PRIVILEGES
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_privileges);
        }
      }
    ;

object_privilege_list:
      object_privilege
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | object_privilege_list ',' object_privilege
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_object_privilege_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

object_privilege:
      SELECT_SYM opt_column_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SELECT_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | INSERT opt_column_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_INSERT));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | UPDATE_SYM opt_column_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_UPDATE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | REFERENCES opt_column_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_REFERENCES));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | DELETE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DELETE_SYM));
        }
      }
    | USAGE
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_USAGE));
        }
      }
    | INDEX_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_INDEX_SYM));
        }
      }
    | ALTER
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ALTER));
        }
      }
    | CREATE
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CREATE));
        }
      }
    | DROP
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DROP));
        }
      }
    | EXECUTE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_EXECUTE_SYM));
        }
      }
    | RELOAD
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_RELOAD));
        }
      }
    | SHUTDOWN
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SHUTDOWN));
        }
      }
    | PROCESS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PROCESS));
        }
      }
    | FILE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_FILE_SYM));
        }
      }
    | GRANT OPTION
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_GRANT));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_OPTION));
        }
      }
    | SHOW DATABASES
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SHOW));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_DATABASES));
        }
      }
    | SUPER_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SUPER_SYM));
        }
      }
    | CREATE TEMPORARY TABLES
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CREATE));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_TEMPORARY));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_TABLES));
        }
      }
    | LOCK_SYM TABLES
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_LOCK_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_TABLES));
        }
      }
    | REPLICATION SLAVE
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_REPLICATION));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_SLAVE));
        }
      }
    | REPLICATION CLIENT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_REPLICATION));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_CLIENT_SYM));
        }
      }
    | CREATE VIEW_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CREATE));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_VIEW_SYM));
        }
      }
    | SHOW VIEW_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SHOW));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_VIEW_SYM));
        }
      }
    | CREATE ROUTINE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CREATE));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_ROUTINE_SYM));
        }
      }
    | ALTER ROUTINE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ALTER));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_ROUTINE_SYM));
        }
      }
    | CREATE USER
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CREATE));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_USER));
        }
      }
    | EVENT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_EVENT_SYM));
        }
      }
    | TRIGGER_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TRIGGER_SYM));
        }
      }
    | CREATE TABLESPACE
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_object_privilege);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CREATE));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_TABLESPACE));
        }
      }
    ;

opt_and:
      /* empty */
      {
          $$= NULL;
      }
    | AND_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_and);
        }
      }
    ;

require_list:
      require_list_element opt_and require_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_require_list);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::merge_ast_child_nodes($$, $3);
        }
      }
    | require_list_element
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_require_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

require_list_element:
      SUBJECT_SYM TEXT_STRING
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_require_list_element);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SUBJECT_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_TEXT_STRING));
        }
      }
    | ISSUER_SYM TEXT_STRING
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_require_list_element);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ISSUER_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_TEXT_STRING));
        }
      }
    | CIPHER_SYM TEXT_STRING
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_require_list_element);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_CIPHER_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_TEXT_STRING));
        }
      }
    ;

grant_ident:
      '*'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_grant_ident);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_42));
        }
      }
    | ident '.' '*'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_grant_ident);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_46));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_42));
        }
      }
    | '*' '.' '*'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_grant_ident);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_42));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_46));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_42));
        }
      }
    | table_ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_grant_ident);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

user_list:
      user
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_user_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | user_list ',' user
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_user_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

grant_list:
      grant_user
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_grant_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | grant_list ',' grant_user
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_grant_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

grant_user:
      user IDENTIFIED_SYM BY TEXT_STRING
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_grant_user);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_IDENTIFIED_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_BY));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_TEXT_STRING));
        }
      }
    | user IDENTIFIED_SYM BY PASSWORD TEXT_STRING
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_grant_user);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_IDENTIFIED_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_BY));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_PASSWORD));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_TEXT_STRING));
        }
      }
    | user IDENTIFIED_SYM WITH ident_or_text
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_grant_user);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_IDENTIFIED_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_WITH));
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | user IDENTIFIED_SYM WITH ident_or_text AS TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_grant_user);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_IDENTIFIED_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_WITH));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_AS));
          mysql_parser::add_ast_child_node($$, $6);
        }
      }
    | user
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_grant_user);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

opt_column_list:
      /* empty */
      {
          $$= NULL;
      }
    | '(' column_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_column_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
        }
      }
    ;

column_list:
      column_list ',' column_list_id
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_column_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | column_list_id
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_column_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

column_list_id:
      ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_column_list_id);
        }
      }
    ;

require_clause:
      /* empty */
      {
          $$= NULL;
      }
    | REQUIRE_SYM require_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_require_clause);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_REQUIRE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | REQUIRE_SYM SSL_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_require_clause);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_REQUIRE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_SSL_SYM));
        }
      }
    | REQUIRE_SYM X509_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_require_clause);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_REQUIRE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_X509_SYM));
        }
      }
    | REQUIRE_SYM NONE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_require_clause);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_REQUIRE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_NONE_SYM));
        }
      }
    ;

grant_options:
      /* empty */
      {
          $$= NULL;
      }
    | WITH grant_option_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_grant_options);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_WITH));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

opt_grant_option:
      /* empty */
      {
          $$= NULL;
      }
    | WITH GRANT OPTION
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_grant_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_WITH));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_GRANT));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_OPTION));
        }
      }
    ;

grant_option_list:
      grant_option_list grant_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_grant_option_list);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | grant_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_grant_option_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

grant_option:
      GRANT OPTION
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_grant_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_GRANT));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_OPTION));
        }
      }
    | MAX_QUERIES_PER_HOUR ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_grant_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MAX_QUERIES_PER_HOUR));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | MAX_UPDATES_PER_HOUR ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_grant_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MAX_UPDATES_PER_HOUR));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | MAX_CONNECTIONS_PER_HOUR ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_grant_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MAX_CONNECTIONS_PER_HOUR));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | MAX_USER_CONNECTIONS_SYM ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_grant_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_MAX_USER_CONNECTIONS_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

begin:
      BEGIN_SYM opt_work
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_begin);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_BEGIN_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

opt_work:
      /* empty */
      {
          $$= NULL;
      }
    | WORK_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_work);
        }
      }
    ;

opt_chain:
      /* empty */
      {
          $$= NULL;
      }
    | AND_SYM NO_SYM CHAIN_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_chain);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_AND_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_NO_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_CHAIN_SYM));
        }
      }
    | AND_SYM CHAIN_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_chain);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_AND_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_CHAIN_SYM));
        }
      }
    ;

opt_release:
      /* empty */
      {
          $$= NULL;
      }
    | RELEASE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_release);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_RELEASE_SYM));
        }
      }
    | NO_SYM RELEASE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_release);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_NO_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_RELEASE_SYM));
        }
      }
    ;

opt_savepoint:
      /* empty */
      {
          $$= NULL;
      }
    | SAVEPOINT_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_savepoint);
        }
      }
    ;

commit:
      COMMIT_SYM opt_work opt_chain opt_release
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_commit);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_COMMIT_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

rollback:
      ROLLBACK_SYM opt_work opt_chain opt_release
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_rollback);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ROLLBACK_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | ROLLBACK_SYM opt_work TO_SYM opt_savepoint ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_rollback);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ROLLBACK_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_TO_SYM));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    ;

savepoint:
      SAVEPOINT_SYM ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_savepoint);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SAVEPOINT_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

release:
      RELEASE_SYM SAVEPOINT_SYM ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_release);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_RELEASE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_SAVEPOINT_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

union_clause:
      /* empty */
      {
          $$= NULL;
      }
    | union_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_union_clause);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

union_list:
      UNION_SYM union_option select_init
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_union_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_UNION_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

union_opt:
      /* empty */
      {
          $$= NULL;
      }
    | union_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_union_opt);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | union_order_or_limit
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_union_opt);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

opt_union_order_or_limit:
      /* empty */
      {
          $$= NULL;
      }
    | union_order_or_limit
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_union_order_or_limit);
        }
      }
    ;

union_order_or_limit:
      order_or_limit
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_union_order_or_limit);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

order_or_limit:
      order_clause opt_limit_clause_init
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_order_or_limit);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | limit_clause
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_order_or_limit);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

union_option:
      /* empty */
      {
          $$= NULL;
      }
    | DISTINCT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_union_option);
        }
      }
    | ALL
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_union_option);
        }
      }
    ;

query_specification:
      SELECT_SYM select_init2_derived
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_query_specification);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SELECT_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | '(' select_paren_derived ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_query_specification);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
        }
      }
    ;

query_expression_body:
      query_specification opt_union_order_or_limit
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_query_expression_body);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | query_expression_body UNION_SYM union_option query_specification opt_union_order_or_limit
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_query_expression_body);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_UNION_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    ;

subselect:
      subselect_start query_expression_body subselect_end
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_subselect);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

subselect_start:
      /* empty */
      {
          $$= NULL;
      }
    ;

subselect_end:
      /* empty */
      {
          $$= NULL;
      }
    ;

opt_query_expression_options:
      /* empty */
      {
          $$= NULL;
      }
    | query_expression_option_list
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_query_expression_options);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

query_expression_option_list:
      query_expression_option_list query_expression_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_query_expression_option_list);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | query_expression_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_query_expression_option_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

query_expression_option:
      STRAIGHT_JOIN
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_query_expression_option);
        }
      }
    | HIGH_PRIORITY
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_query_expression_option);
        }
      }
    | DISTINCT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_query_expression_option);
        }
      }
    | SQL_SMALL_RESULT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_query_expression_option);
        }
      }
    | SQL_BIG_RESULT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_query_expression_option);
        }
      }
    | SQL_BUFFER_RESULT
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_query_expression_option);
        }
      }
    | SQL_CALC_FOUND_ROWS
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_query_expression_option);
        }
      }
    | ALL
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_query_expression_option);
        }
      }
    ;

view_or_trigger_or_sp_or_event:
      definer definer_tail
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_view_or_trigger_or_sp_or_event);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | no_definer no_definer_tail
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_view_or_trigger_or_sp_or_event);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | view_replace_or_algorithm definer_opt view_tail
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_view_or_trigger_or_sp_or_event);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

definer_tail:
      view_tail
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_definer_tail);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | trigger_tail
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_definer_tail);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sp_tail
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_definer_tail);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sf_tail
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_definer_tail);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | event_tail
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_definer_tail);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

no_definer_tail:
      view_tail
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_no_definer_tail);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | trigger_tail
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_no_definer_tail);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sp_tail
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_no_definer_tail);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | sf_tail
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_no_definer_tail);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | udf_tail
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_no_definer_tail);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | event_tail
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_no_definer_tail);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

definer_opt:
      no_definer
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_definer_opt);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | definer
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_definer_opt);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

no_definer:
      /* empty */
      {
          $$= NULL;
      }
    ;

definer:
      DEFINER_SYM EQ user
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_definer);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_DEFINER_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

view_replace_or_algorithm:
      view_replace
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_view_replace_or_algorithm);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | view_replace view_algorithm
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_view_replace_or_algorithm);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | view_algorithm
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_view_replace_or_algorithm);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    ;

view_replace:
      OR_SYM REPLACE
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_view_replace);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_OR_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_REPLACE));
        }
      }
    ;

view_algorithm:
      ALGORITHM_SYM EQ UNDEFINED_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_view_algorithm);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ALGORITHM_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_UNDEFINED_SYM));
        }
      }
    | ALGORITHM_SYM EQ MERGE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_view_algorithm);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ALGORITHM_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_MERGE_SYM));
        }
      }
    | ALGORITHM_SYM EQ TEMPTABLE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_view_algorithm);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ALGORITHM_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_EQ));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_TEMPTABLE_SYM));
        }
      }
    ;

view_suid:
      /* empty */
      {
          $$= NULL;
      }
    | SQL_SYM SECURITY_SYM DEFINER_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_view_suid);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SQL_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_SECURITY_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_DEFINER_SYM));
        }
      }
    | SQL_SYM SECURITY_SYM INVOKER_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_view_suid);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SQL_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_SECURITY_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_INVOKER_SYM));
        }
      }
    ;

view_tail:
      view_suid VIEW_SYM table_ident view_list_opt AS view_select
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_view_tail);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_VIEW_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_AS));
          mysql_parser::add_ast_child_node($$, $6);
        }
      }
    ;

view_list_opt:
      /* empty */
      {
          $$= NULL;
      }
    | '(' view_list ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_view_list_opt);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
        }
      }
    ;

view_list:
      ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_view_list);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | view_list ',' ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::reuse_ast_node($1, sql::_view_list);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

view_select:
      view_select_aux view_check_option
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_view_select);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

view_select_aux:
      create_view_select union_clause
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_view_select_aux);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    | '(' create_view_select_paren ')' union_opt
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_view_select_aux);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    ;

create_view_select_paren:
      create_view_select
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_view_select_paren);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | '(' create_view_select_paren ')'
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_view_select_paren);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_40));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_41));
        }
      }
    ;

create_view_select:
      SELECT_SYM select_part2
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_create_view_select);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SELECT_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

view_check_option:
      /* empty */
      {
          $$= NULL;
      }
    | WITH CHECK_SYM OPTION
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_view_check_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_WITH));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_CHECK_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_OPTION));
        }
      }
    | WITH CASCADED CHECK_SYM OPTION
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_view_check_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_WITH));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_CASCADED));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_CHECK_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_OPTION));
        }
      }
    | WITH LOCAL_SYM CHECK_SYM OPTION
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_view_check_option);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_WITH));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_LOCAL_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_CHECK_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_OPTION));
        }
      }
    ;

trigger_tail:
      TRIGGER_SYM remember_name sp_name trg_action_time trg_event ON remember_name table_ident FOR_SYM remember_name EACH_SYM ROW_SYM sp_proc_stmt
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_trigger_tail);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_TRIGGER_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_ON));
          mysql_parser::add_ast_child_node($$, $7);
          mysql_parser::add_ast_child_node($$, $8);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($9, sql::_FOR_SYM));
          mysql_parser::add_ast_child_node($$, $10);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($11, sql::_EACH_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($12, sql::_ROW_SYM));
          mysql_parser::add_ast_child_node($$, $13);
        }
      }
    ;

udf_tail:
      AGGREGATE_SYM remember_name FUNCTION_SYM ident RETURNS_SYM udf_type SONAME_SYM TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_udf_tail);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_AGGREGATE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($3, sql::_FUNCTION_SYM));
          mysql_parser::add_ast_child_node($$, $4);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($5, sql::_RETURNS_SYM));
          mysql_parser::add_ast_child_node($$, $6);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($7, sql::_SONAME_SYM));
          mysql_parser::add_ast_child_node($$, $8);
        }
      }
    | remember_name FUNCTION_SYM ident RETURNS_SYM udf_type SONAME_SYM TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_udf_tail);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_FUNCTION_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_RETURNS_SYM));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_SONAME_SYM));
          mysql_parser::add_ast_child_node($$, $7);
        }
      }
    ;

sf_tail:
      remember_name FUNCTION_SYM sp_name '(' sp_fdparam_list ')' RETURNS_SYM type_with_opt_collate sp_c_chistics sp_proc_stmt
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sf_tail);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_FUNCTION_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_40));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($7, sql::_RETURNS_SYM));
          mysql_parser::add_ast_child_node($$, $8);
          mysql_parser::add_ast_child_node($$, $9);
          mysql_parser::add_ast_child_node($$, $10);
        }
      }
    ;

sp_tail:
      PROCEDURE_SYM remember_name sp_name '(' sp_pdparam_list ')' sp_c_chistics sp_proc_stmt
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_sp_tail);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_PROCEDURE_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_40));
          mysql_parser::add_ast_child_node($$, $5);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($6, sql::_41));
          mysql_parser::add_ast_child_node($$, $7);
          mysql_parser::add_ast_child_node($$, $8);
        }
      }
    ;

xa:
      XA_SYM begin_or_start xid opt_join_or_resume
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_xa);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_XA_SYM));
          mysql_parser::add_ast_child_node($$, $2);
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | XA_SYM END xid opt_suspend
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_xa);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_XA_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_END));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | XA_SYM PREPARE_SYM xid
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_xa);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_XA_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_PREPARE_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | XA_SYM COMMIT_SYM xid opt_one_phase
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_xa);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_XA_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_COMMIT_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, $4);
        }
      }
    | XA_SYM ROLLBACK_SYM xid
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_xa);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_XA_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_ROLLBACK_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | XA_SYM RECOVER_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_xa);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_XA_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_RECOVER_SYM));
        }
      }
    ;

xid:
      text_string
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_xid);
          mysql_parser::add_ast_child_node($$, $1);
        }
      }
    | text_string ',' text_string
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_xid);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    | text_string ',' text_string ',' ulong_num
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_xid);
          mysql_parser::add_ast_child_node($$, $1);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_44));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_44));
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    ;

begin_or_start:
      BEGIN_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_begin_or_start);
        }
      }
    | START_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_begin_or_start);
        }
      }
    ;

opt_join_or_resume:
      /* empty */
      {
          $$= NULL;
      }
    | JOIN_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_join_or_resume);
        }
      }
    | RESUME_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::set_ast_node_name($1, sql::_opt_join_or_resume);
        }
      }
    ;

opt_one_phase:
      /* empty */
      {
          $$= NULL;
      }
    | ONE_SYM PHASE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_one_phase);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_ONE_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_PHASE_SYM));
        }
      }
    ;

opt_suspend:
      /* empty */
      {
          $$= NULL;
      }
    | SUSPEND_SYM opt_migrate
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_suspend);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_SUSPEND_SYM));
          mysql_parser::add_ast_child_node($$, $2);
        }
      }
    ;

opt_migrate:
      /* empty */
      {
          $$= NULL;
      }
    | FOR_SYM MIGRATE_SYM
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_opt_migrate);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_FOR_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_MIGRATE_SYM));
        }
      }
    ;

install:
      INSTALL_SYM PLUGIN_SYM ident SONAME_SYM TEXT_STRING_sys
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_install);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_INSTALL_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_PLUGIN_SYM));
          mysql_parser::add_ast_child_node($$, $3);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($4, sql::_SONAME_SYM));
          mysql_parser::add_ast_child_node($$, $5);
        }
      }
    ;

uninstall:
      UNINSTALL_SYM PLUGIN_SYM ident
      {
        if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)
        {
          $$= mysql_parser::new_ast_node(sql::_uninstall);
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($1, sql::_UNINSTALL_SYM));
          mysql_parser::add_ast_child_node($$, mysql_parser::set_ast_node_name($2, sql::_PLUGIN_SYM));
          mysql_parser::add_ast_child_node($$, $3);
        }
      }
    ;

