/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */


#ifndef _REPORTING_TEMPLATE_VARIABLES_H_
#define _REPORTING_TEMPLATE_VARIABLES_H_

// General variables.
const char* const REPORT_GENERATED = "GENERATED";
const char* const REPORT_PROJECT_AUTHOR = "PROJECT_AUTHOR";
const char* const REPORT_PROJECT_CREATED = "PROJECT_CREATED";
const char* const REPORT_PROJECT_CHANGED = "PROJECT_CHANGED";
const char* const REPORT_PROJECT_DESCRIPTION = "PROJECT_DESCRIPTION";
const char* const REPORT_PROJECT_NAME = "PROJECT_NAME";
const char* const REPORT_PROJECT_TITLE = "PROJECT_TITLE";
const char* const REPORT_PROJECT_VERSION = "PROJECT_VERSION";
const char* const REPORT_STYLE_NAME = "STYLE_NAME";
const char* const REPORT_TITLE = "TITLE";

// Schema variables.
const char* const REPORT_SCHEMATA = "SCHEMATA";
const char* const REPORT_SCHEMA_ID = "SCHEMA_ID";         // Unique id.
const char* const REPORT_SCHEMA_NUMBER = "SCHEMA_NUMBER"; // One based index (for schemata simply ID + 1).
const char* const REPORT_SCHEMA_NAME = "SCHEMA_NAME";
const char* const REPORT_SCHEMA_COUNT = "SCHEMA_COUNT";

// Table variables.
const char* const REPORT_TABLES = "TABLES";
const char* const REPORT_TABLE_NAME = "TABLE_NAME";
const char* const REPORT_TOTAL_TABLE_COUNT = "TOTAL_TABLE_COUNT";
const char* const REPORT_TABLE_COUNT = "TABLE_COUNT";
const char* const REPORT_TABLE_ID = "TABLE_ID";         // Unique id over all schemata.
const char* const REPORT_TABLE_NUMBER = "TABLE_NUMBER"; // One based index in the schema.
const char* const REPORT_TABLE_COMMENT_LISTING = "TABLE_COMMENT_LISTING";
const char* const REPORT_TABLE_COMMENT = "TABLE_COMMENT";
const char* const REPORT_TABLE_COLLATION = "TABLE_COLLATION";
const char* const REPORT_TABLE_CHARSET = "TABLE_CHARSET";
const char* const REPORT_TABLE_ENGINE = "TABLE_ENGINE";
const char* const REPORT_TABLE_PACK_KEYS = "TABLE_PACK_KEYS";
const char* const REPORT_TABLE_AUTO_INCREMENT = "TABLE_AUTO_INCREMENT";
const char* const REPORT_TABLE_DELAY_KEY_UPDATES = "TABLE_DELAY_KEY_UPDATES";
const char* const REPORT_TABLE_ROW_FORMAT = "TABLE_ROW_FORMAT";
const char* const REPORT_TABLE_KEY_BLOCK_SIZE = "TABLE_KEY_BLOCK_SIZE";
const char* const REPORT_TABLE_AVG_ROW_LENGTH = "TABLE_AVG_ROW_LENGTH";
const char* const REPORT_TABLE_MIN_ROW_COUNT = "TABLE_MIN_ROW_COUNT";
const char* const REPORT_TABLE_MAX_ROW_COUNT = "TABLE_MAX_ROW_COUNT";
const char* const REPORT_TABLE_USE_CHECKSUM = "TABLE_USE_CHECKSUM";
const char* const REPORT_TABLE_HAS_PASSWORD = "TABLE_HAS_PASSWORD";
const char* const REPORT_TABLE_DATA_DIR = "TABLE_DATA_DIR";
const char* const REPORT_TABLE_INDEX_DIR = "TABLE_INDEX_DIR";
const char* const REPORT_TABLE_UNION_TABLES = "TABLE_UNION_TABLES";
const char* const REPORT_TABLE_MERGE_METHOD = "TABLE_MERGE_METHOD";
const char* const REPORT_TABLE_CONNECTION_STRING = "TABLE_CONNECTION_STRING";

// Index variables.
const char* const REPORT_INDICES = "INDICES";
const char* const REPORT_INDICES_LISTING = "INDICES_LISTING";
const char* const REPORT_INDICES_COLUMNS = "INDICES_COLUMNS";

const char* const REPORT_INDEX_COUNT = "INDEX_COUNT";
const char* const REPORT_TOTAL_INDEX_COUNT = "TOTAL_INDEX_COUNT";
const char* const REPORT_INDEX_ID = "INDEX_ID";
const char* const REPORT_INDEX_NUMBER = "INDEX_NUMBER";
const char* const REPORT_INDEX_NAME = "INDEX_NAME";
const char* const REPORT_INDEX_PRIMARY = "INDEX_PRIMARY";
const char* const REPORT_INDEX_UNIQUE = "INDEX_UNIQUE";
const char* const REPORT_INDEX_TYPE = "INDEX_TYPE";
const char* const REPORT_INDEX_KIND = "INDEX_KIND";
const char* const REPORT_INDEX_KEY_BLOCK_SIZE = "INDEX_KEY_BLOCK_SIZE";
const char* const REPORT_INDEX_COMMENT = "INDEX_COMMENT";
const char* const REPORT_INDEX_COLUMNS = "INDEX_COLUMNS";
const char* const REPORT_INDEX_COLUMN_NAME = "INDEX_COLUMN_NAME";
const char* const REPORT_INDEX_COLUMN_ORDER = "INDEX_COLUMN_ORDER";
const char* const REPORT_INDEX_COLUMN_COMMENT = "INDEX_COLUMN_COMMENT";

// Variables for primary key and foreign key + relationship.
const char* const REPORT_FOREIGN_KEYS = "FOREIGN_KEYS";
const char* const REPORT_FOREIGN_KEY_ID = "FOREIGN_KEY_ID";
const char* const REPORT_FOREIGN_KEY_NUMBER = "FOREIGN_KEY_NUMBER";
const char* const REPORT_FK_DELETE_RULE = "FK_DELETE_RULE";
const char* const REPORT_FK_UPDATE_RULE = "FK_UPDATE_RULE";
const char* const REPORT_FK_MANDATORY = "FK_MANDATORY";
const char* const REPORT_FOREIGN_KEY_COUNT = "FOREIGN_KEY_COUNT";
const char* const REPORT_TOTAL_FK_COUNT = "TOTAL_FK_COUNT";

const char* const REPORT_REL_NAME = "REL_NAME";
const char* const REPORT_REL_LISTING = "REL_LISTING";
const char* const REPORT_REL_PARENTTABLE = "REL_PARENTTABLE";
const char* const REPORT_REL_CHILDTABLE = "REL_CHILDTABLE";
const char* const REPORT_REL_CARD = "REL_CARD";
const char* const REPORT_REL = "REL";
const char* const REPORT_REL_TYPE = "REL_TYPE";

// Column variables.
const char* const REPORT_COLUMNS = "COLUMNS";
const char* const REPORT_COLUMN_ID = "COLUMN_ID";
const char* const REPORT_COLUMN_NUMBER = "COLUMN_NUMBER";
const char* const REPORT_COLUMN_NAME = "COLUMN_NAME";
const char* const REPORT_COLUMN_KEY_PART = "COLUMN_KEY_PART";
const char* const REPORT_COLUMN_NULLABLE = "COLUMN_NULLABLE";
const char* const REPORT_COLUMN_DEFAULTVALUE = "COLUMN_DEFAULTVALUE";
const char* const REPORT_COLUMN_AUTO_INC = "COLUMN_AUTO_INC";
const char* const REPORT_COLUMN_CHARSET = "COLUMN_CHARSET";
const char* const REPORT_COLUMN_COLLATION = "COLUMN_COLLATION";
const char* const REPORT_COLUMN_COMMENT = "COLUMN_COMMENT";
const char* const REPORT_COLUMN_DATATYPE = "COLUMN_DATATYPE";
const char* const REPORT_COLUMN_IS_USERTYPE = "COLUMN_IS_USERTYPE";
const char* const REPORT_COLUMN_COUNT = "COLUMN_COUNT";
const char* const REPORT_TOTAL_COLUMN_COUNT = "TOTAL_COLUMN_COUNT";
const char* const REPORT_COLUMNS_LISTING = "COLUMNS_LISTING";
const char* const REPORT_COLUMN_KEY = "COLUMN_KEY";
const char* const REPORT_COLUMN_NOTNULL = "COLUMN_NOTNULL";

// DDL/script variables (for tables).
const char* const REPORT_DDL_LISTING = "DDL_LISTING";
const char* const REPORT_DDL_SCRIPT = "DDL_SCRIPT";

// Trigger variables.
const char* const REPORT_TRIGGERS = "TRIGGERS";
const char* const REPORT_TRIGGERS_LISTING = "TRIGGER_LISTING";
const char* const REPORT_TOTAL_TRIGGER_COUNT = "TOTAL_TRIGGER_COUNT";
const char* const REPORT_TRIGGER_COUNT = "TRIGGER_COUNT";
const char* const REPORT_TRIGGER_NAME = "TRIGGER_NAME";
const char* const REPORT_TRIGGER_ID = "TRIGGER_ID";
const char* const REPORT_TRIGGER_NUMBER = "TRIGGER_NUMBER";
const char* const REPORT_TRIGGER_DEFINER = "TRIGGER_DEFINER";
const char* const REPORT_TRIGGER_ENABLED = "TRIGGER_ENABLED";
const char* const REPORT_TRIGGER_EVENT = "TRIGGER_EVENT";
const char* const REPORT_TRIGGER_ORDER = "TRIGGER_ORDER";
const char* const REPORT_TRIGGER_OTHER_TRIGGER = "TRIGGER_OTHER_TRIGGER";
const char* const REPORT_TRIGGER_TIMING = "TRIGGER_TIMING";

// Partitioning + sub partitioning variables.
const char* const REPORT_PARTITION_LISTING = "PARTITION_LISTING";
const char* const REPORT_PARTITION_COUNT = "PARTITION_COUNT";
const char* const REPORT_PARTITION_VALUE = "PARTITION_VALUE";
const char* const REPORT_PARTITION_NAME = "PARTITION_NAME";
const char* const REPORT_PARTITION_TYPE = "PARTITION_TYPE";
const char* const REPORT_PARTITION_EXPRESSION = "PARTITION_EXPRESSION";
const char* const REPORT_PARTITIONS = "PARTITIONS";
const char* const REPORT_PARTITION_COMMENT = "PARTITION_COMMENT";
const char* const REPORT_PARTITION_MIN_ROW_COUNT = "PARTITION_MIN_ROW_COUNT";
const char* const REPORT_PARTITION_MAX_ROW_COUNT = "PARTITION_MAX_ROW_COUNT";
const char* const REPORT_PARTITION_DATA_DIR = "PARTITION_DATA_DIR";
const char* const REPORT_PARTITION_INDEX_DIR = "PARTITION_INDEX_DIR";

const char* const REPORT_PARTITION_SUB_PARTITIONS = "PARTITION_SUB_PARTITIONS";
const char* const REPORT_PARTITION_SUB_COUNT = "PARTITION_SUB_COUNT";
const char* const REPORT_PARTITION_SUB_TYPE = "PARTITION_SUB_TYPE";
const char* const REPORT_PARTITION_SUB_EXPRESSION = "PARTITION_SUB_EXPRESSION";
const char* const REPORT_PARTITION_SUB_VALUE = "PARTITION_SUB_VALUE";
const char* const REPORT_PARTITION_SUB_NAME = "PARTITION_SUB_NAME";
const char* const REPORT_PARTITION_SUB_MIN_ROW_COUNT = "PARTITION_SUB_MIN_ROW_COUNT";
const char* const REPORT_PARTITION_SUB_MAX_ROW_COUNT = "PARTITION_SUB_MAX_ROW_COUNT";
const char* const REPORT_PARTITION_SUB_DATA_DIR = "PARTITION_SUB_DATA_DIR";
const char* const REPORT_PARTITION_SUB_INDEX_DIR = "PARTITION_SUB_INDEX_DIR";
const char* const REPORT_PARTITION_SUB_COMMENT = "PARTITION_SUB_COMMENT";

// View variables.
const char* const REPORT_VIEWS = "VIEWS";
const char* const REPORT_VIEW_LISTING = "VIEW_LISTING";
const char* const REPORT_VIEW_COUNT = "VIEW_COUNT";
const char* const REPORT_TOTAL_VIEW_COUNT = "TOTAL_VIEW_COUNT";
const char* const REPORT_VIEW_ID = "VIEW_ID";
const char* const REPORT_VIEW_NUMBER = "VIEW_NUMBER";
const char* const REPORT_VIEW_NAME = "VIEW_NAME";
const char* const REPORT_VIEW_DDL = "VIEW_DDL";
const char* const REPORT_VIEW_COMMENT = "VIEW_COMMENT";
const char* const REPORT_VIEW_COMMENT_LISTING = "VIEW_COMMENT_LISTING";
const char* const REPORT_VIEW_COLUMNS = "VIEW_COLUMNS";
const char* const REPORT_VIEW_READ_ONLY = "VIEW_READ_ONLY";
const char* const REPORT_VIEW_WITH_CHECK = "VIEW_WITH_CHECK";

// Stored procedure/function variables.
const char* const REPORT_ROUTINES = "ROUTINES";
const char* const REPORT_ROUTINE_NAME = "ROUTINE_NAME";
const char* const REPORT_ROUTINE_LISTING = "ROUTINE_LISTING";
const char* const REPORT_ROUTINE_COUNT = "ROUTINE_COUNT";
const char* const REPORT_TOTAL_ROUTINE_COUNT = "TOTAL_ROUTINE_COUNT";
const char* const REPORT_ROUTINE_ID = "ROUTINE_ID";
const char* const REPORT_ROUTINE_NUMBER = "ROUTINE_NUMBER";
const char* const REPORT_ROUTINE_DDL = "ROUTINE_DDL";
const char* const REPORT_ROUTINE_TYPE = "ROUTINE_TYPE";
const char* const REPORT_ROUTINE_PARAMETERS = "ROUTINE_PARAMETERS";
const char* const REPORT_ROUTINE_PARAMETER_NAME = "ROUTINE_PARAMETER_NAME";
const char* const REPORT_ROUTINE_PARAMETER_TYPE = "ROUTINE_PARAMETER_TYPE";
const char* const REPORT_ROUTINE_PARAMETER_DATA_TYPE = "ROUTINE_PARAMETER_DATA_TYPE";
const char* const REPORT_ROUTINE_PARAMETER_COUNT = "ROUTINE_PARAMETER_COUNT";
const char* const REPORT_ROUTINE_RETURN_TYPE = "ROUTINE_RETURN_TYPE";
const char* const REPORT_ROUTINE_SECURITY = "ROUTINE_SECURITY";

#endif // _REPORTING_TEMPLATE_VARIABLES_H_