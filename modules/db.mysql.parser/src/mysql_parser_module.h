/*
 * Copyright (c) 2013, 2017, Oracle and/or its affiliates. All rights reserved.
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

#if defined(_WIN32)
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

#ifndef HAVE_PRECOMPILED_HEADERS
#include "grts/structs.db.mysql.h"
#include "grts/structs.wrapper.h"
#endif

#define DOC_MYSQLPARSERSERVICESIMPL                                            \
  "Parsing services for MySQL.\n"                                              \
  "\n"                                                                         \
  "This module provides various functions for processing of MySQL sql code.\n" \
  "It is the second generation module using a fast ANTLR-based recognizer infrastructure."

//--------------------------------------------------------------------------------------------------

class MYSQL_PARSER_PUBLIC MySQLParserServicesImpl : public parser::MySQLParserServices, public grt::ModuleImplBase {
public:
  MySQLParserServicesImpl(grt::CPPModuleLoader *loader) : grt::ModuleImplBase(loader), _stop(false) {
  }
  DEFINE_INIT_MODULE_DOC(
    "1.0", "Oracle Corporation", DOC_MYSQLPARSERSERVICESIMPL, grt::ModuleImplBase,
    DECLARE_MODULE_FUNCTION_DOC(
      MySQLParserServicesImpl::createParserContext,
      "Creates a new parser context which is needed for most calls to parse or syntax check something.",
      "charsets a list of character sets (as stored in db_catalog or db_rdbms)\n"
      "version the server version that guides the parsing process\n"
      "sql_mode the sql mode to be used for parsing\n"
      "case_sensitive a flag telling whether object names must be compared case sensitively (only used for schemas, "
      "tables and views)"),

    DECLARE_MODULE_FUNCTION_DOC(
      MySQLParserServicesImpl::stopProcessing,
      "Tells the module to stop any ongoing processing as soon as possible (mostly statement range determination). Can "
      "be called from any thread.\n"
      "Calling any other module function will reset this flag, so make sure any running task returned"
      "before starting a new one.",
      ""),

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
      "routine an instanatiated routine object to fill in details\n"
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
                                "Renames all schema references in the catalog from the old to the name.",
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
      MySQLParserServicesImpl::replaceTokenSequence,
      "Parses the given SQL code and searches for the given token. If found this and the following "
      "tokens (determined by the count) is replaced by the text in the string array (one string "
      "entry per replaced token). It is attempted to maintain the current text casing.",
      "sql the sql script to process\n"
      "start_token the token to search for. The first found instance is used.\n"
      "count the number of tokens to replace (including the start token).\n"
      "replacements the strings to use instead of the text for the found tokens.\n"
      "             If more than count the rest is simply appended.\n"
      "             If less than count then entries are removed."),

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
  parser_ContextReferenceRef createParserContext(GrtCharacterSetsRef charsets, GrtVersionRef version,
                                                 const std::string &sql_mode, int case_sensitive);

  virtual size_t stopProcessing();

  virtual size_t parseTable(parser::MySQLParserContext::Ref context, db_mysql_TableRef table, const std::string &sql);

  size_t parseTriggerSql(parser_ContextReferenceRef context_ref, db_mysql_TriggerRef trigger, const std::string &sql);
  virtual size_t parseTrigger(parser::MySQLParserContext::Ref context, db_mysql_TriggerRef trigger,
                              const std::string &sql);

  size_t parseViewSql(parser_ContextReferenceRef context_ref, db_mysql_ViewRef view, const std::string &sql);
  virtual size_t parseView(parser::MySQLParserContext::Ref context, db_mysql_ViewRef view, const std::string &sql);

  size_t parseRoutineSql(parser_ContextReferenceRef context_ref, db_mysql_RoutineRef routine, const std::string &sql);
  virtual size_t parseRoutine(parser::MySQLParserContext::Ref context, db_mysql_RoutineRef routine,
                              const std::string &sql);

  size_t parseRoutinesSql(parser_ContextReferenceRef context_ref, db_mysql_RoutineGroupRef group,
                          const std::string &sql);
  virtual size_t parseRoutines(parser::MySQLParserContext::Ref context, db_mysql_RoutineGroupRef group,
                               const std::string &sql);

  virtual size_t parseSchema(parser::MySQLParserContext::Ref context, db_mysql_SchemaRef schema,
                             const std::string &sql);

  virtual size_t parseIndex(parser::MySQLParserContext::Ref context, db_mysql_IndexRef index, const std::string &sql);

  virtual size_t parseEvent(parser::MySQLParserContext::Ref context, db_mysql_EventRef event, const std::string &sql);

  virtual size_t parseLogfileGroup(parser::MySQLParserContext::Ref context, db_mysql_LogFileGroupRef group,
                                   const std::string &sql);

  virtual size_t parseServer(parser::MySQLParserContext::Ref context, db_mysql_ServerLinkRef server,
                             const std::string &sql);

  virtual size_t parseTablespace(parser::MySQLParserContext::Ref context, db_mysql_TablespaceRef tablespace,
                                 const std::string &sql);

  size_t parseSQLIntoCatalogSql(parser_ContextReferenceRef context_ref, db_mysql_CatalogRef catalog,
                                const std::string &sql, grt::DictRef options);
  virtual size_t parseSQLIntoCatalog(parser::MySQLParserContext::Ref context, db_mysql_CatalogRef catalog,
                                     const std::string &sql, grt::DictRef options);

  size_t doSyntaxCheck(parser_ContextReferenceRef context_ref, const std::string &sql, const std::string &type);
  virtual size_t checkSqlSyntax(parser::MySQLParserContext::Ref context, const char *sql, size_t length,
                                MySQLParseUnit type);

  size_t doSchemaRefRename(parser_ContextReferenceRef context_ref, db_mysql_CatalogRef catalog,
                           const std::string old_name, const std::string new_name);
  virtual size_t renameSchemaReferences(parser::MySQLParserContext::Ref context, db_mysql_CatalogRef catalog,
                                        const std::string old_name, const std::string new_name);

  grt::BaseListRef getSqlStatementRanges(const std::string &sql);
  virtual size_t determineStatementRanges(const char *sql, size_t length, const std::string &initial_delimiter,
                                          std::vector<std::pair<size_t, size_t> > &ranges,
                                          const std::string &line_break = "\n");

  grt::DictRef parseStatementDetails(parser_ContextReferenceRef context_ref, const std::string &sql);
  virtual grt::DictRef parseStatement(parser::MySQLParserContext::Ref context, const std::string &sql);

  // Query manipulation.
  std::string replaceTokenSequence(parser_ContextReferenceRef context_ref, const std::string &sql, size_t start_token,
                                   size_t count, grt::StringListRef replacements);
  virtual std::string replaceTokenSequenceWithText(parser::MySQLParserContext::Ref context, const std::string &sql,
                                                   size_t start_token, size_t count,
                                                   const std::vector<std::string> replacements);

private:
  bool _stop;
};
