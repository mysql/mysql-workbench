/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#pragma once

#include "mysql_sql_parser_public_interface.h"
#include "grtsqlparser/sql_facade.h"
#include "mysql_sql_parser_base.h"
#include "grts/structs.db.mysql.h"
#include "grtpp_module_cpp.h"

#include "mysql-parser.h"

#define MysqlSqlFacade_VERSION "2.0"

#define DOC_MysqlSqlFacadeImpl \
  "MySQL parser routines. Allows parsing SQL scripts into GRT objects, checking syntax etc."

/** Implements DBMS specifics.
 *
 * @ingroup sqlparser
 */
class MYSQL_SQL_PARSER_PUBLIC_FUNC MysqlSqlFacadeImpl : public SqlFacade, public grt::ModuleImplBase {
public:
  MysqlSqlFacadeImpl(grt::CPPModuleLoader *ldr) : grt::ModuleImplBase(ldr) {
  }

  DEFINE_INIT_MODULE_DOC(
    MysqlSqlFacade_VERSION, "Oracle and/or its affiliates", DOC_MysqlSqlFacadeImpl, grt::ModuleImplBase,
    DECLARE_MODULE_FUNCTION_DOC(
      MysqlSqlFacadeImpl::parseSqlScriptString,
      "Parses a SQL script containing CREATE statements from a string, "
      "filling the given catalog object with the encountered tables, views and other objects.",
      "catalog an initialized catalog object where the parsed objects should be added\n"
      "sql the SQL script to be parsed"),
    DECLARE_MODULE_FUNCTION_DOC(
      MysqlSqlFacadeImpl::parseSqlScriptStringEx,
      "Parses a SQL script containing CREATE statements from a string, "
      "filling the given catalog object with the encountered tables, views and other objects.\n"
      "The following options are recognized:\n"
      "sql_script_codeset, created_objects, gen_fk_names_when_empty, case_sensitive_identifiers,"
      "processing_create_statements, processing_alter_statements, processing_drop_statements, reuse_existing_objects",
      "catalog an initialized catalog object where the parsed objects should be added\n"
      "sql the SQL script to be parsed\n"
      "options a dictionary containing various options for the parser routine"),
    DECLARE_MODULE_FUNCTION_DOC(
      MysqlSqlFacadeImpl::parseSqlScriptFile,
      "Parses a SQL script containing CREATE statements from a file, "
      "filling the given catalog object with the encountered tables, views and other objects.",
      "catalog an instantiated catalog object where the parsed objects should be added\n"
      "filename the SQL script file to be parsed"),
    DECLARE_MODULE_FUNCTION_DOC(
      MysqlSqlFacadeImpl::parseSqlScriptFileEx,
      "Parses a SQL script containing CREATE statements from a file, "
      "filling the given catalog object with the encountered tables, views and other objects.",
      "catalog an initialized catalog object where the parsed objects should be added\n"
      "filename the SQL script file to be parsed\n"
      "options a dictionary containing various options for the parser routine"),
    DECLARE_MODULE_FUNCTION(MysqlSqlFacadeImpl::parseInserts),

    DECLARE_MODULE_FUNCTION_DOC(MysqlSqlFacadeImpl::parseTrigger,
                                "Parses a trigger from the SQL script and applies it to the given view object.",
                                "trigger an instantiated trigger object to fill\n"
                                "sql the SQL script to be parsed"),

    DECLARE_MODULE_FUNCTION_DOC(
      MysqlSqlFacadeImpl::parseRoutine,
      "Parses a stored procedure or function from the SQL script and applies it to the given routine object.",
      "routine an instantiated routine object to fill\n"
      "sql the SQL script to be parsed"),
    DECLARE_MODULE_FUNCTION_DOC(MysqlSqlFacadeImpl::parseRoutines,
                                "Parses a set of stored procedure or function from the SQL script and adds them to the "
                                "given routine group object.",
                                "routineGroup an instantiated routine group object to fill\n"
                                "sql the SQL script to be parsed"),
    DECLARE_MODULE_FUNCTION_DOC(MysqlSqlFacadeImpl::parseView,
                                "Parses a view from the SQL script and applies it to the given view object.",
                                "view an instantiated view object to fill\n"
                                "sql the SQL script to be parsed"),
    DECLARE_MODULE_FUNCTION(MysqlSqlFacadeImpl::checkSqlSyntax),
    DECLARE_MODULE_FUNCTION(MysqlSqlFacadeImpl::checkTriggerSyntax),
    DECLARE_MODULE_FUNCTION(MysqlSqlFacadeImpl::checkViewSyntax),
    DECLARE_MODULE_FUNCTION(MysqlSqlFacadeImpl::checkRoutineSyntax),
    DECLARE_MODULE_FUNCTION(MysqlSqlFacadeImpl::renameSchemaReferences),
    DECLARE_MODULE_FUNCTION_DOC(MysqlSqlFacadeImpl::splitSqlStatements,
                                "Splits the given SQL script into separate statements, returning a list of strings. "
                                "For large scripts, getSqlStatementRanges() is preferred, as it will not create a copy "
                                "of each statement.",
                                "sql a SQL script, with one or more statements, separated by ;"),
    DECLARE_MODULE_FUNCTION_DOC(
      MysqlSqlFacadeImpl::getSqlStatementRanges,
      "Splits the given SQL script into separate statement ranges, returning a list of offset,length pairs.",
      "sql a SQL script, with one or more statements, separated by ;"),
    DECLARE_MODULE_FUNCTION_DOC(
      MysqlSqlFacadeImpl::parseAstFromSqlScript,
      "Parses the given SQL code, splitting into statements and building an AST out of it.\n"
      "The return value is a list of ASTs - one for each statement- which are in turn, a tree composed "
      "of lists denoting a tuple in the form (symbol-name, value, [child-nodes], base_offset, begin_offset, "
      "end_offset).\n"
      "Where:\n"
      "symbol-name is the name of the MySQL grammar symbol (see the MySQL grammar in the MySQL server source code)\n"
      "value is a string with the value of the token in the node, or empty if this is not a terminal node\n"
      "[child-nodes] is a list of child nodes, with the same format\n"
      "If there's an error parsing the statement, a string containing the error text will be added in place of the "
      "tree.",
      "sql a SQL script, with one or more statements"),
    NULL);

  int splitSqlScript(const std::string &sql, std::list<std::string> &statements);
  int splitSqlScript(const char *sql, std::size_t length, const std::string &initial_delimiter,
                     std::vector<std::pair<std::size_t, std::size_t> > &ranges, const std::string &line_break = "\n");

  Sql_parser::Ref sqlParser();
  int parseSqlScriptString(db_CatalogRef catalog, const std::string sql);
  int parseSqlScriptStringEx(db_CatalogRef catalog, const std::string sql, const grt::DictRef options);
  int parseSqlScriptFile(db_CatalogRef catalog, const std::string filename);
  int parseSqlScriptFileEx(db_CatalogRef catalog, const std::string filename, const grt::DictRef options);

  Invalid_sql_parser::Ref invalidSqlParser();
  int parseInserts(db_TableRef table, const std::string &sql);
  int parseTrigger(db_TriggerRef trigger, const std::string &sql);
  int parseRoutine(db_RoutineRef routine, const std::string &sql);
  int parseRoutines(db_RoutineGroupRef routineGroup, const std::string &sql);
  int parseView(db_ViewRef view, const std::string &sql);

  Sql_syntax_check::Ref sqlSyntaxCheck();
  int checkSqlSyntax(const std::string &sql);
  int checkTriggerSyntax(const std::string &sql);
  int checkViewSyntax(const std::string &sql);
  int checkRoutineSyntax(const std::string &sql);

  Sql_semantic_check::Ref sqlSemanticCheck();

  Sql_specifics::Ref sqlSpecifics();

  Sql_normalizer::Ref sqlNormalizer();
  std::string normalizeSqlStatement(const std::string sql, const std::string schema_name);
  virtual std::string removeInterTokenSpaces(const std::string sql);

  Sql_inserts_loader::Ref sqlInsertsLoader();

  Sql_schema_rename::Ref sqlSchemaRenamer();
  int renameSchemaReferences(db_CatalogRef catalog, const std::string old_schema_name,
                             const std::string new_schema_name);

  grt::StringListRef splitSqlStatements(const std::string &sql);
  grt::BaseListRef getSqlStatementRanges(const std::string &sql);

  // AST is returned as a list of tree of lists in the format [[symbol-name, value, [child-nodes], base_offset,
  // begin_offs, end_offs], ...],
  // one list item per statement in the script. If there is a syntax error in the statement, a string with the error
  // message will be there instead of the AST
  grt::BaseListRef parseAstFromSqlScript(const std::string &sql);

  Sql_statement_decomposer::Ref sqlStatementDecomposer(grt::DictRef db_opts = grt::DictRef());

  grt::BaseListRef getItemFromPath(const std::string &path, const grt::BaseListRef source);
  std::string getTypeDescription(grt::BaseListRef type_node,
                                 std::vector<std::string> *additional_type_data_paths = NULL);
  virtual bool parseSelectStatementForEdit(const std::string &sql, std::string &schema_name, std::string &table_name,
                                           String_tuple_list &column_names);
  virtual bool parseRoutineDetails(const std::string &sql, std::string &type, std::string &name,
                                   String_tuple_list &parameters, std::string &return_value, std::string &comments);
  virtual bool parseDropStatement(const std::string &sql, std::string &object_type,
                                  std::vector<std::pair<std::string, std::string> > &object_names);

  void stop_processing();

private:
  bool _stop;
};
