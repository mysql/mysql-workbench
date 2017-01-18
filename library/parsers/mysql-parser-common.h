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

#include <set>
#include <antlr3.h>
#include <string>
#include <vector>

#include "parsers-common.h"
#include "mysql-recognition-types.h"

extern "C" {
ANTLR3_UINT32 check_charset(void *payload, pANTLR3_STRING text);
ANTLR3_UINT32 check_null(pANTLR3_STRING text);
}

void PARSERS_PUBLIC_TYPE determineStatementRanges(const char *sql, size_t length, const std::string &initialDelimiter,
                                                  std::vector<std::pair<size_t, size_t>> &ranges,
                                                  const std::string &lineBreak = "\n");

class PARSERS_PUBLIC_TYPE MySQLRecognitionBase : public IRecognizer {
public:
  MySQLRecognitionBase(const std::set<std::string> &charsets);
  virtual ~MySQLRecognitionBase();

  // Internal function called by static callback.
  void add_error(const std::string &message, ANTLR3_UINT32 token, ANTLR3_MARKER token_start, ANTLR3_UINT32 line,
                 ANTLR3_UINT32 offset_in_line, ANTLR3_MARKER length);

  const std::vector<ParserErrorInfo> &error_info() const;
  bool has_errors() const;
  unsigned sql_mode() const;
  virtual void set_sql_mode(const std::string &sql_mode);
  void set_sql_mode(unsigned sql_mode);

  const std::set<std::string> charsets() const;
  bool is_charset(const std::string &s) const;

  // IRecognizer implementation.
  virtual bool isIdentifier(uint32_t type) const;
  virtual bool isKeyword(ANTLR3_UINT32 type) const;

  virtual std::string tokenText(pANTLR3_BASE_TREE node, bool keepQuotes = false) const;
  virtual std::string textForTree(pANTLR3_BASE_TREE tree) const;

  uint32_t get_keyword_token(const std::string &keyword) const;

  static bool is_relation(ANTLR3_UINT32 type);
  static bool is_number(ANTLR3_UINT32 type);
  static bool is_operator(ANTLR3_UINT32 type);

protected:
  virtual void reset();

private:
  class Private;
  Private *d;
};
