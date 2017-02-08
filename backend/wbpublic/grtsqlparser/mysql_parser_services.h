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

#include "wbpublic_public_interface.h"

#include "parser_services_common.h"
#include "mysql-recognition-types.h"
#include "grtdb/db_helpers.h"

#ifndef HAVE_PRECOMPILED_HEADERS
#include "grts/structs.db.mysql.h"
#endif

class MySQLRecognizer;
class MySQLSyntaxChecker;
class MySQLScanner;
class MySQLQueryIdentifier;

namespace parser {

  class WBPUBLICBACKEND_PUBLIC_FUNC MySQLParserContext {
  private:
    MySQLRecognizer *_recognizer;
    MySQLSyntaxChecker *_syntax_checker;

    GrtVersionRef _version;
    bool _case_sensitive;
    std::string _sql_mode;
    std::set<std::string> _filtered_charsets;

    void update_filtered_charsets(long version);

  public:
    typedef std::shared_ptr<MySQLParserContext> Ref;

    MySQLParserContext(GrtCharacterSetsRef charsets, GrtVersionRef version, bool case_sensitive);
    ~MySQLParserContext();

    MySQLRecognizer *recognizer() {
      return _recognizer;
    };
    MySQLSyntaxChecker *syntax_checker() {
      return _syntax_checker;
    };
    std::shared_ptr<MySQLScanner> createScanner(
      const std::string &text); // The scanner uses the same version etc as the other recognizers.
    std::shared_ptr<MySQLQueryIdentifier> createQueryIdentifier();

    void use_sql_mode(const std::string &mode);
    std::string get_sql_mode();

    void use_server_version(GrtVersionRef version);
    GrtVersionRef get_server_version() {
      return _version;
    };

    bool case_sensitive() {
      return _case_sensitive;
    };

    std::vector<ParserErrorEntry> get_errors_with_offset(size_t offset, bool for_syntax_check);

    uint32_t get_keyword_token(const std::string &keyword);
  };

  /**
   * Defines an abstract interface for parser services. The actual implementation is done in a module
   * (and hence a singleton).
   */
  class WBPUBLICBACKEND_PUBLIC_FUNC MySQLParserServices {
  public:
    typedef MySQLParserServices *Ref; // We only have a singleton, so define Ref only to keep the pattern.

    static MySQLParserServices::Ref get();
    static MySQLParserContext::Ref createParserContext(GrtCharacterSetsRef charsets, GrtVersionRef version,
                                                       bool case_sensitive);

    virtual size_t stopProcessing() = 0;

    virtual size_t parseTable(parser::MySQLParserContext::Ref context, db_mysql_TableRef table,
                              const std::string &sql) = 0;
    virtual size_t parseRoutine(parser::MySQLParserContext::Ref context, db_mysql_RoutineRef routine,
                                const std::string &sql) = 0;
    virtual size_t parseRoutines(parser::MySQLParserContext::Ref context, db_mysql_RoutineGroupRef group,
                                 const std::string &sql) = 0;
    virtual size_t parseTrigger(MySQLParserContext::Ref context, db_mysql_TriggerRef trigger,
                                const std::string &sql) = 0;
    virtual size_t parseView(parser::MySQLParserContext::Ref context, db_mysql_ViewRef view,
                             const std::string &sql) = 0;
    virtual size_t parseSchema(parser::MySQLParserContext::Ref context, db_mysql_SchemaRef schema,
                               const std::string &sql) = 0;
    virtual size_t parseIndex(parser::MySQLParserContext::Ref context, db_mysql_IndexRef index,
                              const std::string &sql) = 0;
    virtual size_t parseEvent(parser::MySQLParserContext::Ref context, db_mysql_EventRef event,
                              const std::string &sql) = 0;
    virtual size_t parseLogfileGroup(parser::MySQLParserContext::Ref context, db_mysql_LogFileGroupRef group,
                                     const std::string &sql) = 0;
    virtual size_t parseServer(parser::MySQLParserContext::Ref context, db_mysql_ServerLinkRef server,
                               const std::string &sql) = 0;
    virtual size_t parseTablespace(parser::MySQLParserContext::Ref context, db_mysql_TablespaceRef tablespace,
                                   const std::string &sql) = 0;

    virtual size_t parseSQLIntoCatalog(parser::MySQLParserContext::Ref context, db_mysql_CatalogRef catalog,
                                       const std::string &sql, grt::DictRef options) = 0;

    virtual size_t checkSqlSyntax(MySQLParserContext::Ref context, const char *sql, size_t length,
                                  MySQLParseUnit unitType) = 0;
    virtual size_t renameSchemaReferences(parser::MySQLParserContext::Ref context, db_mysql_CatalogRef catalog,
                                          const std::string old_name, const std::string new_name) = 0;

    virtual size_t determineStatementRanges(const char *sql, size_t length, const std::string &initial_delimiter,
                                            std::vector<std::pair<size_t, size_t> > &ranges,
                                            const std::string &line_break = "\n") = 0;

    virtual grt::DictRef parseStatement(parser::MySQLParserContext::Ref context, const std::string &sql) = 0;

    // Query manipulation services.
    virtual std::string replaceTokenSequenceWithText(parser::MySQLParserContext::Ref context, const std::string &sql,
                                                     size_t start_token, size_t count,
                                                     const std::vector<std::string> replacements) = 0;
  };

} // namespace parser
