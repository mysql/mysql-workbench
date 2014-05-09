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

public:
  typedef boost::shared_ptr<ParserContext> Ref;

  ParserContext(GrtCharacterSetsRef charsets, GrtVersionRef version);
  ~ParserContext();

  MySQLRecognizer *recognizer() { return _recognizer; };

  void use_sql_mode(const std::string &mode);
  void use_server_version(GrtVersionRef version);
  GrtVersionRef get_server_version() { return _version; };
  
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

  static ParserContext::Ref createParserContext(GrtCharacterSetsRef charsets, GrtVersionRef version);
  static MySQLParserServices::Ref get(grt::GRT *grt);

  virtual int stopProcessing() = 0;
  virtual int determineStatementRanges(const char *sql, size_t length, const std::string &initial_delimiter, 
    std::vector<std::pair<size_t, size_t> > &ranges, const std::string &line_break = "\n") = 0;

  virtual int parseTrigger(ParserContext::Ref context, db_TriggerRef trigger, const std::string &sql) = 0;

  virtual int checkSqlSyntax(ParserContext::Ref context, const char *sql, size_t length, MySQLQueryType type) = 0;
};

} // namespace parser
