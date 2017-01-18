/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "parse_utils.h"

namespace bec {

  bool tokenize_string_list(const std::string &str, int quote_char, bool quoted_only, std::list<std::string> &tokens) {
    const char *str_start = str.c_str();
    const char *word_start = str_start;
    const char *p;
    enum { Start, QuotedString, UnquotedString, WaitingComma } state = Start;
    bool escaping = false;

    for (p = str_start; *p; p = g_utf8_next_char(p)) {
      switch (state) {
        case Start:
          if (*p == quote_char) {
            state = QuotedString;
            word_start = p;
          } else if (isalnum(*p)) {
            if (quoted_only)
              return false;
            state = UnquotedString;
            word_start = p;
          } else if (std::isspace(*p))
            ;
          else
            return false;
          break;
        case QuotedString:
          if (*p == quote_char && !escaping) {
            tokens.push_back(std::string(word_start, p + 1));
            state = WaitingComma;
          } else if (*p == '\\' && !escaping)
            escaping = true;
          else
            escaping = false;
          break;
        case UnquotedString:
          if (std::isspace(*p)) {
            tokens.push_back(std::string(word_start, p));
            state = WaitingComma;
          } else if (*p == ',') {
            tokens.push_back(std::string(word_start, p + 1));
            state = Start;
          }
          break;
        case WaitingComma:
          if (std::isspace(*p))
            ;
          else if (*p == ',')
            state = Start;
          else
            return false;
          break;
      }
    }

    if (escaping)
      return false;

    if (state == UnquotedString) {
      tokens.push_back(std::string(word_start, p));
      state = WaitingComma;
    }

    return state == WaitingComma;
  }
};
