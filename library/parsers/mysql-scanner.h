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

#include "mysql-parser-common.h"

/**
 * C++ interface for the ANTLR based MySQL lexer.
 * This scanner class is not needed for the MySQLRecognizer class (it uses the raw lexer)
 * but provides tokenizing functionality beside it.
 */
class PARSERS_PUBLIC_TYPE MySQLScanner : public MySQLRecognitionBase {
public:
  MySQLScanner(const char *text, size_t length, bool is_utf8, long server_version, const std::string &sql_mode_string,
               const std::set<std::string> &charsets);
  virtual ~MySQLScanner();

  static MySQLQueryType getQueryType(const char *text, size_t length, bool is_utf8, long server_version);
  void reset();

  // Informations about the current token.
  ParserToken token();
  uint32_t token_type();
  uint32_t token_line();
  size_t token_start();
  size_t token_end();
  size_t token_channel();
  std::string token_text();

  void next(bool skip_hidden = true);
  void previous(bool skip_hidden = true);
  bool skipIf(uint32_t token);

  size_t position();
  void seek(size_t position);
  void seek(size_t line, size_t offset);
  uint32_t look_around(int offset, bool ignore_hidden);

  bool is(uint32_t type);
  bool is_keyword();
  bool is_relation();
  bool is_number();
  bool is_operator();
  bool is_identifier();
  bool is_separator();

  void set_server_version(long version);
  long get_server_version() const;
  void set_sql_mode(const std::string &new_mode);
  unsigned int get_sql_mode_flags() const;

  virtual std::string text() const;
  virtual const char *lineStart() const;

protected:
  void setup();

private:
  class Private;
  Private *d;
};

/**
 * Similar to the scanner class but using only the absolute minimum setup to determine the
 * type of a query in a given sql text, to make it as fast as possible.
 */
class PARSERS_PUBLIC_TYPE MySQLQueryIdentifier : public MySQLRecognitionBase {
public:
  MySQLQueryIdentifier(long server_version, const std::string &sql_mode_string, const std::set<std::string> &charsets);
  MySQLQueryIdentifier(long server_version, unsigned sql_mode, const std::set<std::string> &charsets);
  virtual ~MySQLQueryIdentifier();

  MySQLQueryType getQueryType(const char *text, std::size_t length, bool is_utf8);

  // TODO: check if it is better to not derive from MySQLRecognitionBase to avoid these overrides.
  virtual std::string text() const {
    return "";
  };
  virtual const char *lineStart() const {
    return nullptr;
  };
  virtual std::string tokenText(pANTLR3_BASE_TREE node, bool keepQuotes = false) const {
    return "";
  };
  virtual std::string textForTree(pANTLR3_BASE_TREE tree) const {
    return "";
  };

private:
  class Private;
  Private *d;

  bool skipDefiner(pANTLR3_TOKEN_SOURCE tokenSource, pANTLR3_COMMON_TOKEN &token);
  MySQLQueryType determineQueryType(pANTLR3_TOKEN_SOURCE tokenSource);
};
