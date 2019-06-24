/*
 * Copyright (c) 2012, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "wb_db_schema.h"
#include "base/sqlstring.h"
#include "base/string_utilities.h"
#include "base/log.h"

using namespace wb;
using namespace base;

DEFAULT_LOG_DOMAIN("WbDbSchema");

InternalSchema::InternalSchema(const std::string &schema_name, sql::Dbc_connection_handler::Ref &conn)
  : _connection(conn), _schema_name(schema_name) {
}

InternalSchema::~InternalSchema(void) {
}

bool InternalSchema::check_schema_exist() {
  bool ret_val = false;
  try {
    std::unique_ptr<sql::Statement> stmt(_connection->ref->createStatement());
    std::unique_ptr<sql::ResultSet> rs(
      stmt->executeQuery(std::string(base::sqlstring("SHOW DATABASES LIKE ?", 0) << _schema_name)));

    ret_val = rs->next();
  } catch (const sql::SQLException &exc) {
    logWarning("Error verifying existence of wb schema '%s': %s", _schema_name.c_str(), exc.what());
    ret_val = false;
  }

  return ret_val;
}

bool InternalSchema::check_function_exists(const std::string &function_name) {
  return check_function_or_sp_exists(function_name, true);
}

bool InternalSchema::check_stored_procedure_exists(const std::string &spname) {
  return check_function_or_sp_exists(spname, false);
}

bool InternalSchema::check_function_or_sp_exists(const std::string object_name, bool check_function) {
  bool ret_val = false;
  std::string what = check_function ? "FUNCTION" : "PROCEDURE";
  std::string statement = "SHOW " + what + " STATUS LIKE ?";
  try {
    std::unique_ptr<sql::Statement> stmt(_connection->ref->createStatement());
    std::unique_ptr<sql::ResultSet> rs(
      stmt->executeQuery(std::string(base::sqlstring(statement.c_str(), 0) << object_name)));

    while (!ret_val && rs->next()) {
      std::string schema = rs->getString(1);

      ret_val = (schema == _schema_name);
    }
  } catch (const sql::SQLException &exc) {
    logWarning("Error verifying existence of %s '%s'.'%s' : %s", what.c_str(), _schema_name.c_str(),
               object_name.c_str(), exc.what());
    ret_val = false;
  }

  return ret_val;
}

bool InternalSchema::check_table_exists(const std::string &table_name) {
  return check_table_or_view_exists(table_name, false);
}

bool InternalSchema::check_view_exists(const std::string &view_name) {
  return check_table_or_view_exists(view_name, true);
}

bool InternalSchema::check_table_or_view_exists(const std::string object_name, bool check_view) {
  std::string what = check_view ? "view" : "table";
  bool ret_val = false;
  try {
    std::unique_ptr<sql::Statement> stmt(_connection->ref->createStatement());
    std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery(
      std::string(base::sqlstring("SHOW FULL TABLES FROM ! LIKE ?", 0) << _schema_name << object_name)));

    while (!ret_val && rs->next()) {
      std::string type = rs->getString(1);
      bool type_is_view = type == "VIEW";

      ret_val = !(type_is_view ^ check_view);
    }
  } catch (const sql::SQLException &exc) {
    logWarning("Error verifying existence of %s '%s'.'%s' : %s", what.c_str(), _schema_name.c_str(),
               object_name.c_str(), exc.what());
    ret_val = false;
  }

  return ret_val;
}

std::string InternalSchema::create_schema() {
  std::string statement(base::sqlstring("CREATE SCHEMA !", 0) << _schema_name);

  return execute_sql(statement);
}

bool InternalSchema::is_remote_search_deployed() {
  bool ret_val = check_schema_exist() && check_stored_procedure_exists("SEARCH_OBJECTS") &&
                 check_stored_procedure_exists("SEARCH_TABLES_AND_VIEWS") &&
                 check_stored_procedure_exists("SEARCH_ROUTINES");

  return ret_val;
}

std::string InternalSchema::execute_sql(const std::string &statement) {
  std::string ret_val("");
  try {
    std::unique_ptr<sql::Statement> stmt(_connection->ref->createStatement());
    stmt->execute(statement);
  } catch (const sql::SQLException &exc) {
    ret_val = base::strfmt("MySQL Error : %s (code %d)", exc.what(), exc.getErrorCode());
    logWarning("Error executing sql :\n '%s'\n Error %d : %s", statement.c_str(), exc.getErrorCode(), exc.what());
  }

  return ret_val;
}

std::string InternalSchema::deploy_remote_search() {
  std::string ret_val("");

  if (!check_schema_exist())
    ret_val = create_schema();

  if (!ret_val.length() && !check_stored_procedure_exists("SEARCH_TABLES_AND_VIEWS"))
    ret_val = deploy_get_tables_and_views_sp();

  if (!ret_val.length() && !check_stored_procedure_exists("SEARCH_OBJECTS"))
    ret_val = deploy_get_objects_sp();

  if (!ret_val.length() && !check_stored_procedure_exists("SEARCH_ROUTINES"))
    ret_val = deploy_get_routines();

  return ret_val;
}

std::string InternalSchema::deploy_get_objects_sp() {
  std::string statement =
    "CREATE PROCEDURE `" + _schema_name +
    "`.`SEARCH_OBJECTS`(IN schema_filter VARCHAR(255), IN object_filter VARCHAR(255), IN matching_type INT)\n"
    "BEGIN\n"
    "    DECLARE sch_name VARCHAR(255);\n"
    "    DECLARE start_index INT;\n"
    "    DECLARE end_index INT;\n"
    "    DECLARE sch_delimiter CHAR(1);\n"
    "    DECLARE sch_length INT;\n"

    "    SET @databases := '';\n"
    "    SHOW DATABASES WHERE (@databases := CONCAT(@databases, `Database`, ',')) IS NULL;\n"

    "    DROP TABLE IF EXISTS MATCHING_OBJECTS;"

    "    CREATE TEMPORARY TABLE MATCHING_OBJECTS(\n"
    "        SCHEMA_NAME VARCHAR(100),\n"
    "        OBJECT_NAME VARCHAR(100),\n"
    "        OBJECT_TYPE VARCHAR(1)) ENGINE InnoDB DEFAULT CHARSET=utf8;\n"

    "    SET sch_length = LENGTH(@databases);\n"
    "    SET sch_delimiter = ',';\n"
    "    SET start_index = 1;\n"

    "    REPEAT\n"

    "        SET end_index = LOCATE(sch_delimiter, @databases, start_index);\n"

    "        IF end_index > 0 THEN \n"
    "            SET sch_name = MID(@databases, start_index, end_index - start_index);\n"
    "            SET start_index = end_index + 1;\n"

    "            SET @matched = 0;\n"
    "            IF matching_type = 0 THEN\n"
    "               SELECT sch_name LIKE schema_filter INTO @matched;\n"
    "            ELSE\n"
    "               SELECT sch_name REGEXP schema_filter INTO @matched;\n"
    "            END IF;\n"

    "            IF @matched = 1 THEN\n"
    "                CALL SEARCH_TABLES_AND_VIEWS(sch_name, object_filter, matching_type);\n"
    "            END IF;\n"
    "        END IF;\n"

    "    UNTIL start_index > sch_length\n"
    "    END REPEAT;\n"

    "    CALL SEARCH_ROUTINES(schema_filter, object_filter, matching_type, 0);\n"
    "    CALL SEARCH_ROUTINES(schema_filter, object_filter, matching_type, 1);\n"

    "    SELECT * FROM MATCHING_OBJECTS;\n"
    "END";

  return execute_sql(statement);
}

std::string InternalSchema::deploy_get_tables_and_views_sp() {
  std::string statement =
    "CREATE PROCEDURE `" + _schema_name +
    "`.`SEARCH_TABLES_AND_VIEWS`( IN schema_name VARCHAR(255), IN object_filter VARCHAR(255), IN matching_type INT)\n"
    "BEGIN\n"
    "    DECLARE table_def VARCHAR(255);\n"
    "    DECLARE table_name VARCHAR(255);\n"
    "    DECLARE table_type VARCHAR(255);\n"
    "    DECLARE type VARCHAR(1);\n"
    "    DECLARE start_index INT;\n"
    "    DECLARE end_index INT;\n"
    "    DECLARE type_index INT;\n"
    "    DECLARE tbl_delimiter CHAR(1);\n"
    "    DECLARE type_delimiter CHAR(1);\n"
    "    DECLARE tbl_length INT;\n"

    "    SET @tables := '';\n"

    "    SET @sql = CONCAT(\"SHOW FULL TABLES FROM `\", schema_name, \"` WHERE (@tables:=CONCAT(@tables, "
    "`Tables_in_\", schema_name, \"`, ':', `Table_type`, ';')) IS NULL;\");\n"
    "    PREPARE stmt FROM @sql;\n"
    "    EXECUTE stmt;\n"
    "    DEALLOCATE prepare stmt;\n"

    "    SET tbl_length = LENGTH(@tables);\n"
    "    SET tbl_delimiter = ';';\n"
    "    SET type_delimiter = ':';\n"
    "    SET start_index = 1;\n"

    "    REPEAT\n"

    "        SET end_index = LOCATE(tbl_delimiter, @tables, start_index);\n"

    "        IF end_index > 0 THEN \n"
    "            SET table_def = MID(@tables, start_index, end_index - start_index);\n"
    "            SET start_index = end_index + 1;\n"

    "            SET type_index = LOCATE(type_delimiter, table_def, 1);\n"
    "            SET table_name = MID(table_def, 1, type_index - 1);\n"
    "            SET table_type = MID(table_def, type_index + 1, LENGTH(table_def) - type_index);\n"

    "            IF table_type = 'VIEW' THEN\n"
    "                SET type = 'V';\n"
    "            ELSE\n"
    "                SET type = 'T';\n"
    "            END IF;\n"

    "            SET @matched = 0;\n"
    "            IF matching_type = 0 THEN \n"
    "               SELECT table_name LIKE object_filter INTO @matched;\n"
    "            ELSE\n"
    "               SELECT table_name REGEXP object_filter INTO @matched;\n"
    "            END IF;\n"

    "            IF @matched = 1 THEN\n"
    "                INSERT INTO MATCHING_OBJECTS VALUES(schema_name, table_name, type);\n"
    "            END IF;\n"

    "            SET start_index = end_index + 1;\n"
    "        END IF;\n"

    "    UNTIL start_index > tbl_length\n"
    "    END REPEAT;\n"
    "END";

  return execute_sql(statement);
}

std::string InternalSchema::deploy_get_routines() {
  std::string statement =
    "CREATE PROCEDURE `" + _schema_name +
    "`.`SEARCH_ROUTINES`(IN schema_filter VARCHAR(255), IN object_filter VARCHAR(255), IN matching_type INT, IN "
    "functions INT)\n"
    "BEGIN\n"
    "    DECLARE routine_def VARCHAR(255);\n"
    "    DECLARE routine_name VARCHAR(255);\n"
    "    DECLARE routine_type VARCHAR(1);\n"
    "    DECLARE sch_name VARCHAR(255);\n"
    "    DECLARE start_index INT;\n"
    "    DECLARE end_index INT;\n"
    "    DECLARE sch_delimiter CHAR(1);\n"
    "    DECLARE routine_delimiter CHAR(1);\n"
    "    DECLARE sch_length INT;\n"
    "    DECLARE routine_length INT;\n"
    "    DECLARE sch_index INT;\n"
    "    SET @routines := '';\n"
    "    IF functions = 1 THEN \n"
    "       SHOW FUNCTION STATUS WHERE (@routines:=CONCAT(@routines, Db, ':', Name, ';')) IS NULL;\n"
    "       SET routine_type = 'F';\n"
    "    ELSE \n"
    "       SHOW PROCEDURE STATUS WHERE (@routines:=CONCAT(@routines, Db, ':', Name, ';')) IS NULL;\n"
    "       SET routine_type = 'P';\n"
    "    END IF;\n"
    "    SET routine_length = LENGTH(@routines);\n"
    "    SET sch_delimiter = ':';\n"
    "    SET routine_delimiter = ';';\n"
    "    SET start_index = 1;\n"
    "    REPEAT\n"
    "        SET end_index = LOCATE(routine_delimiter, @routines, start_index);\n"
    "        IF end_index > 0 THEN \n"
    "            SET routine_def = MID(@routines, start_index, end_index - start_index);\n"
    "            SET start_index = end_index + 1;\n"
    "            SET sch_index = LOCATE(sch_delimiter, routine_def, 1);\n"
    "            SET sch_name = MID(routine_def, 1, sch_index - 1);\n"
    "            SET routine_name = MID(routine_def, sch_index + 1, LENGTH(routine_def) - sch_index);\n"

    "            SET @matched_schema = 0;\n"
    "            IF matching_type = 0 THEN\n"
    "               SELECT sch_name LIKE schema_filter INTO @matched_schema;\n"
    "            ELSE\n"
    "               SELECT sch_name REGEXP schema_filter INTO @matched_schema;\n"
    "            END IF;\n"

    "            SET @matched_routine = 0;\n"
    "            IF matching_type = 0 THEN\n"
    "               SELECT routine_name LIKE object_filter INTO @matched_object;\n"
    "            ELSE\n"
    "               SELECT routine_name REGEXP object_filter INTO @matched_object;\n"
    "            END IF;\n"

    "            IF @matched_schema = 1 AND @matched_object = 1 THEN\n"
    "                INSERT INTO MATCHING_OBJECTS VALUES(sch_name, routine_name, routine_type);\n"
    "            END IF;\n"
    "            SET start_index = end_index + 1;\n"
    "        END IF;\n"
    "    UNTIL start_index > routine_length\n"
    "    END REPEAT;\n"
    "END";

  return execute_sql(statement);
}

// SQL Editor snippets

bool InternalSchema::check_snippets_table_exist() {
  return check_schema_exist() && check_table_exists("snippet");
}

std::string InternalSchema::create_snippets_table_exist() {
  if (!check_table_exists("snippet")) {
    if (!check_schema_exist()) {
      std::string error = create_schema();
      if (!error.empty())
        return error;
    }

    std::string statement(
      base::sqlstring("CREATE TABLE !.snippet (id INT PRIMARY KEY auto_increment, title varchar(128), code TEXT)", 0)
      << _schema_name);

    return execute_sql(statement);
  }
  return "";
}

int InternalSchema::insert_snippet(const std::string &title, const std::string &code) {
  std::string statement(base::sqlstring("INSERT INTO !.snippet (title, code) VALUES (?, ?)", 0) << _schema_name << title
                                                                                                << code);

  std::unique_ptr<sql::Statement> stmt(_connection->ref->createStatement());
  stmt->execute(statement);

  std::unique_ptr<sql::ResultSet> result(stmt->executeQuery("SELECT LAST_INSERT_ID()"));
  if (result->next())
    return result->getInt(1);
  return 0;
}

void InternalSchema::delete_snippet(int snippet_id) {
  std::string statement(base::sqlstring("DELETE FROM !.snippet WHERE id = ?", 0) << _schema_name << snippet_id);

  std::unique_ptr<sql::Statement> stmt(_connection->ref->createStatement());
  stmt->execute(statement);
}

void InternalSchema::set_snippet_title(int snippet_id, const std::string &title) {
  std::string statement(base::sqlstring("UPDATE !.snippet SET title = ? WHERE id = ?", 0) << _schema_name << title
                                                                                          << snippet_id);

  std::unique_ptr<sql::Statement> stmt(_connection->ref->createStatement());
  stmt->execute(statement);
}

void InternalSchema::set_snippet_code(int snippet_id, const std::string &code) {
  std::string statement(base::sqlstring("UPDATE !.snippet SET code = ? WHERE id = ?", 0) << _schema_name << code
                                                                                         << snippet_id);

  std::unique_ptr<sql::Statement> stmt(_connection->ref->createStatement());
  stmt->execute(statement);
}
