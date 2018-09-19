/*
 * Copyright (c) 2013, 2018, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#if defined(_MSC_VER)
  #ifdef MYSQL_PARSER_EXPORTS
    #define MYSQL_PARSER_PUBLIC __declspec(dllexport)
  #else
    #define MYSQL_PARSER_PUBLIC __declspec(dllimport)
  #endif
#else
  #define MYSQL_PARSER_PUBLIC
#endif

#include "grtpp_module_cpp.h"
#include "grtsqlparser/mysql_parser_services.h"

#include "grts/structs.db.mysql.h"
#include "grts/structs.wrapper.h"

#define DOC_MYSQLPARSERSERVICESIMPL                                            \
  "Parsing services for MySQL.\n"                                              \
  "\n"                                                                         \
  "This module provides various functions for processing of MySQL sql code.\n" \
  "It is the second generation module using a fast ANTLR-based recognizer infrastructure."

//--------------------------------------------------------------------------------------------------

class MYSQL_PARSER_PUBLIC MySQLParserServicesImpl : public parsers::MySQLParserServices, public grt::ModuleImplBase {
public:
  MySQLParserServicesImpl(grt::CPPModuleLoader *loader) : grt::ModuleImplBase(loader) {
  }

  DEFINE_INIT_MODULE_DOC(
    "1.0", "Oracle Corporation", DOC_MYSQLPARSERSERVICESIMPL, grt::ModuleImplBase,
    DECLARE_MODULE_FUNCTION_DOC(
      MySQLParserServicesImpl::createNewParserContext,
      "Creates a new parser context which is needed for most calls to parse or syntax check something.",
      "charsets a list of character sets (as stored in db_catalog or db_rdbms)\n"
      "version the server version that guides the parsing process\n"
      "sql_mode the sql mode to be used for parsing\n"
      "case_sensitive a flag telling whether object names must be compared case sensitively (only used for schemas, "
      "tables and views)"),

    DECLARE_MODULE_FUNCTION_DOC(MySQLParserServicesImpl::parseTriggerSql,
                                "Parses a trigger from the SQL script and applies it to the given view object.",
                                "context_ref a previously created parser context reference\n"
                                "trigger an instantiated trigger object to fill\n"
                                "sql the SQL script to be parsed"),

    DECLARE_MODULE_FUNCTION_DOC(MySQLParserServicesImpl::parseViewSql,
                                "Parses a view from the SQL script and applies it to the given view object.",
                                "context_ref a previously created parser context reference\n"
                                "view an instantiated view object to fill\n"
                                "sql the SQL script to be parsed"),

    DECLARE_MODULE_FUNCTION_DOC(
      MySQLParserServicesImpl::parseRoutineSql,
      "Parses a procedure or function (including UDF) from the given sql and fills the object with details.",
      "context_ref a previously created parser context reference\n"
      "routine an instatiated routine object to fill in details\n"
      "sql the SQL script to be parsed"),

    DECLARE_MODULE_FUNCTION_DOC(MySQLParserServicesImpl::parseRoutinesSql,
                                "Parses a list of procedures and functions (including UDF) from the given sql and "
                                "fills the object in the routine group with details.",
                                "context_ref a previously created parser context reference\n"
                                "group an instantiated routine group object to fill with routine objects\n"
                                "sql the SQL script to be parsed"),

    DECLARE_MODULE_FUNCTION_DOC(MySQLParserServicesImpl::parseSQLIntoCatalogSql,
                                "Parses an SQL script into a grt catalog structure.",
                                "context_ref a previously created parser context reference\n"
                                "catalog the Catalog where processed sql will be stored\n"
                                "sql the SQL script to be parsed\n"
                                "options Options for processing"),

    DECLARE_MODULE_FUNCTION_DOC(MySQLParserServicesImpl::doSyntaxCheck,
                                "Parses the given sql to see if there's any syntax error.",
                                "context_ref a previously created parser context reference\n"
                                "sql the SQL script to be parsed\n"
                                "type the type of the sql (can be 'full', 'view', 'routine', 'trigger' or 'event')"),

    DECLARE_MODULE_FUNCTION_DOC(MySQLParserServicesImpl::doSchemaRefRename,
                                "Renames all schema references in the catalog from the old to new name.",
                                "context_ref a previously created parser context reference\n"
                                "catalog the catalog whose schemas are processed\n"
                                "old_name the existing schema name\n"
                                "new_name the new schema name"),

    DECLARE_MODULE_FUNCTION_DOC(
      MySQLParserServicesImpl::getSqlStatementRanges,
      "Scans the sql code to find start and stop positions of each contained statement. An initial "
      "delimiter must be provided to find a statement's end. Embedded delimiter commands will be taken "
      "into account properly. The found ranges are returned as grt list.",
      "sql the sql script to process\n"),

    DECLARE_MODULE_FUNCTION_DOC(
      MySQLParserServicesImpl::parseStatementDetails,
      "Parses the given statement and extracts various details into a dict. The values returned depend "
      "on what statement is parsed. This routine only parses single statments.",
      "context_ref a previously created parser context reference\n"
      "sql the SQL code to parse"),

    NULL);

  // Certain module functions taking a parser context have 2 implementations. One for
  // the module interface (with a grt wrapper) and one for direct access.
  // Ultimately, the grt wrapper version uses the direct access version.
  virtual parsers::MySQLParserContext::Ref createParserContext(GrtCharacterSetsRef charsets, GrtVersionRef version,
                                                               const std::string &sqlMode, bool caseSensitive) override;
  parser_ContextReferenceRef createNewParserContext(GrtCharacterSetsRef charsets, GrtVersionRef version,
                                                    const std::string &sqlMode, int case_sensitive);

  virtual size_t tokenFromString(parsers::MySQLParserContext::Ref context, const std::string &token) override;
  virtual MySQLQueryType determineQueryType(parsers::MySQLParserContext::Ref context, const std::string &text) override;

  // DB objects.
  virtual size_t parseTable(parsers::MySQLParserContext::Ref context, db_mysql_TableRef table,
                            const std::string &sql) override;

  size_t parseTriggerSql(parser_ContextReferenceRef context_ref, db_mysql_TriggerRef trigger, const std::string &sql);
  virtual size_t parseTrigger(parsers::MySQLParserContext::Ref context, db_mysql_TriggerRef trigger,
                              const std::string &sql) override;

  size_t parseViewSql(parser_ContextReferenceRef context_ref, db_mysql_ViewRef view, const std::string &sql);
  virtual size_t parseView(parsers::MySQLParserContext::Ref context, db_mysql_ViewRef view,
                           const std::string &sql) override;

  size_t parseRoutineSql(parser_ContextReferenceRef context_ref, db_mysql_RoutineRef routine, const std::string &sql);
  virtual size_t parseRoutine(parsers::MySQLParserContext::Ref context, db_mysql_RoutineRef routine,
                              const std::string &sql) override;

  size_t parseRoutinesSql(parser_ContextReferenceRef context_ref, db_mysql_RoutineGroupRef group,
                          const std::string &sql);
  virtual size_t parseRoutines(parsers::MySQLParserContext::Ref context, db_mysql_RoutineGroupRef group,
                               const std::string &sql) override;

  virtual size_t parseSchema(parsers::MySQLParserContext::Ref context, db_mysql_SchemaRef schema,
                             const std::string &sql) override;
  virtual size_t parseIndex(parsers::MySQLParserContext::Ref context, db_mysql_IndexRef index,
                            const std::string &sql) override;
  virtual size_t parseEvent(parsers::MySQLParserContext::Ref context, db_mysql_EventRef event,
                            const std::string &sql) override;
  virtual size_t parseLogfileGroup(parsers::MySQLParserContext::Ref context, db_mysql_LogFileGroupRef group,
                                   const std::string &sql) override;
  virtual size_t parseServer(parsers::MySQLParserContext::Ref context, db_mysql_ServerLinkRef server,
                             const std::string &sql) override;
  virtual size_t parseTablespace(parsers::MySQLParserContext::Ref context, db_mysql_TablespaceRef tablespace,
                                 const std::string &sql) override;

  size_t parseSQLIntoCatalogSql(parser_ContextReferenceRef context_ref, db_mysql_CatalogRef catalog,
                                const std::string &sql, grt::DictRef options);
  virtual size_t parseSQLIntoCatalog(parsers::MySQLParserContext::Ref context, db_mysql_CatalogRef catalog,
                                     const std::string &sql, grt::DictRef options) override;

  size_t doSyntaxCheck(parser_ContextReferenceRef context_ref, const std::string &sql, const std::string &type);
  virtual size_t checkSqlSyntax(parsers::MySQLParserContext::Ref context, const char *sql, size_t length,
                                MySQLParseUnit type) override;

  size_t doSchemaRefRename(parser_ContextReferenceRef context_ref, db_mysql_CatalogRef catalog,
                           const std::string old_name, const std::string new_name);
  virtual size_t renameSchemaReferences(parsers::MySQLParserContext::Ref context, db_mysql_CatalogRef catalog,
                                        const std::string old_name, const std::string new_name) override;

  grt::BaseListRef getSqlStatementRanges(const std::string &sql);
  virtual size_t determineStatementRanges(const char *sql, size_t length, const std::string &initialDelimiter,
    std::vector<parsers::StatementRange> &ranges, const std::string &lineBreak = "\n") override;

  grt::DictRef parseStatementDetails(parser_ContextReferenceRef context_ref, const std::string &sql);
  virtual grt::DictRef parseStatement(parsers::MySQLParserContext::Ref context, const std::string &sql) override;

  // Data types.
  virtual bool parseTypeDefinition(const std::string &typeDefinition, GrtVersionRef targetVersion,
                                   SimpleDatatypeListRef typeList, UserDatatypeListRef userTypes,
                                   SimpleDatatypeListRef defaultTypeList, db_SimpleDatatypeRef &simpleType,
                                   db_UserDatatypeRef &userType, int &precision, int &scale, int &length,
                                   std::string &datatypeExplicitParams) override;

  // Others.
  virtual std::vector<std::pair<int, std::string>> getCodeCompletionCandidates(
    parsers::MySQLParserContext::Ref context, std::pair<size_t, size_t> caret, std::string const &sql,
    std::string const &defaultSchema, bool uppercaseKeywords, parsers::SymbolTable &symbolTable) override;
};
