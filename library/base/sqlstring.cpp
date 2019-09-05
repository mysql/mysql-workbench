/*
 * Copyright (c) 2011, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "base/sqlstring.h"
#include "base/symbol-info.h"

using namespace base;

const sqlstring sqlstring::null(sqlstring("NULL", 0));

//----------------------------------------------------------------------------------------------------------------------

sqlstring::sqlstring(const std::string &format_string, const sqlstringformat format)
  : _format_string_left(format_string), _format(format) {
  append(consume_until_next_escape());
}

//----------------------------------------------------------------------------------------------------------------------

sqlstring::sqlstring(const char format_string[], const sqlstringformat format)
  : sqlstring(std::string(format_string), format) {
}

//----------------------------------------------------------------------------------------------------------------------

sqlstring::sqlstring(const sqlstring &copy)
  : _formatted(copy._formatted), _format_string_left(copy._format_string_left), _format(copy._format) {
}

//----------------------------------------------------------------------------------------------------------------------

sqlstring::sqlstring() : _format(0) {
}

//----------------------------------------------------------------------------------------------------------------------

std::string sqlstring::consume_until_next_escape() {
  std::string::size_type e = _format_string_left.length(), p = 0;
  while (p < e) {
    char ch = _format_string_left[p];
    if (ch == '?' || ch == '!')
      break;
    ++p;
  }
  if (p > 0) {
    std::string s = _format_string_left.substr(0, p);
    if (p < e)
      _format_string_left = _format_string_left.substr(p);
    else
      _format_string_left.clear();
    return s;
  }
  return "";
}

//----------------------------------------------------------------------------------------------------------------------

int sqlstring::next_escape() {
  if (_format_string_left.empty())
    throw std::invalid_argument("Error formatting SQL query: more arguments than escapes");
  int c = _format_string_left[0];
  _format_string_left = _format_string_left.substr(1);
  return c;
}

//----------------------------------------------------------------------------------------------------------------------

sqlstring &sqlstring::append(const std::string &s) {
  _formatted.append(s);
  return *this;
}

//----------------------------------------------------------------------------------------------------------------------

sqlstring::operator std::string() const {
  return _formatted + _format_string_left;
}

//----------------------------------------------------------------------------------------------------------------------

bool sqlstring::done() const {
  if (_format_string_left.empty())
    return true;
  return _format_string_left[0] != '!' && _format_string_left[0] != '?';
}

//----------------------------------------------------------------------------------------------------------------------

sqlstring &sqlstring::operator<<(const double v) {
  int esc = next_escape();
  if (esc != '?')
    throw std::invalid_argument("Error formatting SQL query: invalid escape for numeric argument");

  append(strfmt("%f", v));
  append(consume_until_next_escape());

  return *this;
}

//----------------------------------------------------------------------------------------------------------------------

sqlstring &sqlstring::operator<<(const sqlstringformat format) {
  _format = format;
  return *this;
}

//----------------------------------------------------------------------------------------------------------------------

sqlstring &sqlstring::operator<<(const std::string &v) {
  int esc = next_escape();
  if (esc == '!') {
    std::string escaped = escape_backticks(v);
    if ((_format._flags & QuoteOnlyIfNeeded) != 0)
      append(base::quoteIdentifierIfNeeded(escaped, '`', MySQLVersion::MySQL80)); // XXX: make this configurable.
    else
      append(base::quote_identifier(escaped, '`'));
  } else if (esc == '?') {
    if (_format._flags & UseAnsiQuotes)
      append("\"").append(escape_sql_string(v)).append("\"");
    else
      append("'").append(escape_sql_string(v)).append("'");
  } else // shouldn't happen
    throw std::invalid_argument(
      "Error formatting SQL query: internal error, expected ? or ! escape got something else");
  append(consume_until_next_escape());

  return *this;
}

//----------------------------------------------------------------------------------------------------------------------

sqlstring &sqlstring::operator<<(const sqlstring &v) {
  next_escape();

  append(v);
  append(consume_until_next_escape());

  return *this;
}

sqlstring &sqlstring::operator<<(const char *v) {
  int esc = next_escape();

  if (esc == '!') {
    if (!v)
      throw std::invalid_argument("Error formatting SQL query: NULL value found for identifier");
    std::string quoted = escape_backticks(v);
    if (quoted == v && (_format._flags & QuoteOnlyIfNeeded))
      append(quoted);
    else
      append("`").append(quoted).append("`");
  } else if (esc == '?') {
    if (v) {
      if (_format._flags & UseAnsiQuotes)
        append("\"").append(escape_sql_string(v)).append("\"");
      else
        append("'").append(escape_sql_string(v)).append("'");
    } else
      append("NULL");
  } else // shouldn't happen
    throw std::invalid_argument(
      "Error formatting SQL query: internal error, expected ? or ! escape got something else");
  append(consume_until_next_escape());

  return *this;
}

//----------------------------------------------------------------------------------------------------------------------
