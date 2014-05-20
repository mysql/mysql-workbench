/* 
 * Copyright (c) 2013, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "mysql-parser.h"

#include "db_helpers.h"

#ifdef __linux__
  #include "grts/structs.db.mysql.h"
#endif

namespace parser {

typedef struct
{
  std::string message;
  size_t position;
  size_t line;
  size_t length;
} ParserErrorEntry;

class WBPUBLICBACKEND_PUBLIC_FUNC ParserContext {

private:
  MySQLRecognizer *_recognizer;
  GrtVersionRef _version;
  bool _case_sensitive;

public:
  typedef boost::shared_ptr<ParserContext> Ref;

  ParserContext(GrtCharacterSetsRef charsets, GrtVersionRef version, bool case_sensitive);
  ~ParserContext();

  MySQLRecognizer *recognizer() { return _recognizer; };

  void use_sql_mode(const std::string &mode);
  void use_server_version(GrtVersionRef version);
  GrtVersionRef get_server_version() { return _version; };

  bool case_sensitive() { return _case_sensitive; };
  
  std::vector<ParserErrorEntry> get_errors_with_offset(size_t offset);
};

/**
 * Defines an abstract interface for parser services. The actual implementation is done in a module
 * (and hence a singleton).
 */
class WBPUBLICBACKEND_PUBLIC_FUNC MySQLParserServices
{
public:
  typedef MySQLParserServices *Ref; // We only have a singleton, so define Ref only to keep the pattern.

  static ParserContext::Ref createParserContext(GrtCharacterSetsRef charsets, GrtVersionRef version, bool case_sensitive);
  static MySQLParserServices::Ref get(grt::GRT *grt);

  virtual size_t stopProcessing() = 0;

  virtual size_t parseRoutine(parser::ParserContext::Ref context, db_mysql_RoutineRef routine, const std::string &sql) = 0;
  virtual size_t parseRoutines(parser::ParserContext::Ref context, db_mysql_RoutineGroupRef group, const std::string &sql) = 0;
  virtual size_t parseTrigger(ParserContext::Ref context, db_mysql_TriggerRef trigger, const std::string &sql) = 0;
  virtual size_t parseView(parser::ParserContext::Ref context, db_mysql_ViewRef view, const std::string &sql) = 0;

  virtual size_t checkSqlSyntax(ParserContext::Ref context, const char *sql, size_t length, MySQLQueryType type) = 0;
  virtual size_t renameSchemaReferences(parser::ParserContext::Ref context, db_mysql_CatalogRef catalog,
    const std::string old_name, const std::string new_name) = 0;

  virtual size_t determineStatementRanges(const char *sql, size_t length, const std::string &initial_delimiter,
                                          std::vector<std::pair<size_t, size_t> > &ranges, const std::string &line_break = "\n") = 0;
};

} // namespace parser
