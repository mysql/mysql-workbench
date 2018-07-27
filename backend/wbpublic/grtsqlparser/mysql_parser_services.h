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

#include "wbpublic_public_interface.h"

#include "mysql/mysql-recognition-types.h"
#include "parsers-common.h"

#include "grtdb/db_helpers.h"

#include "grts/structs.db.mysql.h"

namespace parsers {

  class MySQLParser;
  class SymbolTable;

  // Describes a single statement out of a list in a string.
  struct WBPUBLICBACKEND_PUBLIC_FUNC StatementRange {
    size_t line;   // The line number of the statement.
    size_t start;  // The byte start offset of the statement.
    size_t length; // The length of the statements in bytes.
  };

  struct WBPUBLICBACKEND_PUBLIC_FUNC MySQLParserContext {
    typedef std::shared_ptr<MySQLParserContext> Ref;

    virtual ~MySQLParserContext() {};

    virtual bool isCaseSensitive() = 0;
    virtual void updateServerVersion(GrtVersionRef newVersion) = 0;
    virtual void updateSqlMode(const std::string &mode) = 0;

    virtual GrtVersionRef serverVersion() const = 0;
    virtual std::string sqlMode() const = 0;
    virtual std::vector<ParserErrorInfo> errorsWithOffset(size_t offset) const = 0;

    virtual Scanner createScanner() = 0;

    // Identifier determination depends on e.g the sql mode, hence we need extra handling.
    virtual bool isIdentifier(size_t type) const = 0;
  };

  /**
   * Defines an abstract interface for parser services. The actual implementation is done in a module
   * (and hence a singleton).
   * The API is thread safe if no parser context is shared between threads.
   */
  class WBPUBLICBACKEND_PUBLIC_FUNC MySQLParserServices {
  public:
    virtual ~MySQLParserServices() {};
    typedef MySQLParserServices *Ref; // We only have a singleton, so define Ref only to keep the pattern.

    static MySQLParserServices::Ref get();
    virtual MySQLParserContext::Ref createParserContext(GrtCharacterSetsRef charsets, GrtVersionRef version,
                                                        const std::string &sqlMode, bool caseSensitive) = 0;

    // Info services.
    virtual size_t tokenFromString(MySQLParserContext::Ref context, const std::string &token) = 0;
    virtual MySQLQueryType determineQueryType(MySQLParserContext::Ref context, const std::string &text) = 0;

    // DB objects.
    virtual size_t parseTable(MySQLParserContext::Ref context, db_mysql_TableRef table, const std::string &sql) = 0;
    virtual size_t parseRoutine(MySQLParserContext::Ref context, db_mysql_RoutineRef routine,
                                const std::string &sql) = 0;
    virtual size_t parseRoutines(MySQLParserContext::Ref context, db_mysql_RoutineGroupRef group,
                                 const std::string &sql) = 0;
    virtual size_t parseTrigger(MySQLParserContext::Ref context, db_mysql_TriggerRef trigger,
                                const std::string &sql) = 0;
    virtual size_t parseView(MySQLParserContext::Ref context, db_mysql_ViewRef view, const std::string &sql) = 0;
    virtual size_t parseSchema(MySQLParserContext::Ref context, db_mysql_SchemaRef schema, const std::string &sql) = 0;
    virtual size_t parseIndex(MySQLParserContext::Ref context, db_mysql_IndexRef index, const std::string &sql) = 0;
    virtual size_t parseEvent(MySQLParserContext::Ref context, db_mysql_EventRef event, const std::string &sql) = 0;
    virtual size_t parseLogfileGroup(MySQLParserContext::Ref context, db_mysql_LogFileGroupRef group,
                                     const std::string &sql) = 0;
    virtual size_t parseServer(MySQLParserContext::Ref context, db_mysql_ServerLinkRef server,
                               const std::string &sql) = 0;
    virtual size_t parseTablespace(MySQLParserContext::Ref context, db_mysql_TablespaceRef tablespace,
                                   const std::string &sql) = 0;

    virtual size_t parseSQLIntoCatalog(MySQLParserContext::Ref context, db_mysql_CatalogRef catalog,
                                       const std::string &sql, grt::DictRef options) = 0;

    virtual size_t checkSqlSyntax(MySQLParserContext::Ref context, const char *sql, size_t length,
                                  MySQLParseUnit unitType) = 0;
    virtual size_t renameSchemaReferences(MySQLParserContext::Ref context, db_mysql_CatalogRef catalog,
                                          const std::string old_name, const std::string new_name) = 0;

    virtual size_t determineStatementRanges(const char *sql, size_t length,
                                            const std::string &initialDelimiter,
                                            std::vector<StatementRange> &ranges,
                                            const std::string &lineBreak = "\n") = 0;

    virtual grt::DictRef parseStatement(MySQLParserContext::Ref context, const std::string &sql) = 0;

    // Data types.
    static db_SimpleDatatypeRef findDataType(SimpleDatatypeListRef types, GrtVersionRef version,
                                             const std::string &name);

    virtual bool parseTypeDefinition(const std::string &typeDefinition, GrtVersionRef targetVersion,
                                     SimpleDatatypeListRef typeList, UserDatatypeListRef userTypes,
                                     SimpleDatatypeListRef defaultTypeList, db_SimpleDatatypeRef &simpleType,
                                     db_UserDatatypeRef &userType, int &precision, int &scale, int &length,
                                     std::string &datatypeExplicitParams) = 0;

    // Others.
    virtual std::vector<std::pair<int, std::string>> getCodeCompletionCandidates(
      MySQLParserContext::Ref context, std::pair<size_t, size_t> caret, std::string const &sql,
      std::string const &defaultSchema, bool uppercaseKeywords, parsers::SymbolTable &symbolTable) = 0;
  };

} // namespace parsers
