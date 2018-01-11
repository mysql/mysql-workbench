/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "sql_normalizer.h"
#include <algorithm>

struct normalized_string_builder {
  bool skip_next;
  bool check_comment;
  std::string& result;
  std::string::value_type quote;
  std::string::value_type skip_until;
  normalized_string_builder(std::string& result_str) : result(result_str) {
    skip_next = false;
    check_comment = false;
    quote = 0;
    skip_until = 0;
  }

  void operator()(const std::string::value_type c) {
    if (quote) {
      if (skip_next) // previos char was \ this one is ecaped
        skip_next = false;
      else if (c == '\\') // Skip next char as escaped
        skip_next = true;
      else if (c == quote) // end of quoted string
        quote = 0;
      result += c;
    } else {
      if (skip_until) {
        if (check_comment) {
          if (c == '/') {
            check_comment = false;
            skip_until = 0;
            return;
          }
        }
        if (c == skip_until) {
          if (c == '*')
            check_comment = true;
          else
            skip_until = 0;
        }
        return;
      }
      if (c == '#') {
        skip_until = '\n';
        return;
      }
      if (c == '/') {
        check_comment = true;
        return;
      }
      if (check_comment) {
        if (c == '*') {
          skip_until = '*';
          return;
        } else
          result += '/';
      }
      if ((c == '\'') || (c == '"') || (c == '`')) // quoted string started
        quote = c;
      if (!std::isspace(c))
        result += c; // std::toupper(c); doesn't work since chema.table will go uppercase, while `schema`.`table` won't
    }
  }
};

std::string Sql_normalizer::remove_inter_token_spaces(const std::string& text) {
  std::string result;
  result.reserve(text.size());
  normalized_string_builder builder(result);
  std::for_each(text.begin(), text.end(), builder);
  return result;
}
