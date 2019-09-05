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

#pragma once

#include "base/string_utilities.h"
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_signed.hpp>
#include <stdexcept>

namespace base {

  enum SqlStringFlags { QuoteOnlyIfNeeded = 1 << 0, UseAnsiQuotes = 1 << 1 };

  class BASELIBRARY_PUBLIC_FUNC sqlstring {
  public:
    struct sqlstringformat {
      int _flags;
      sqlstringformat(const int flags) : _flags(flags) {
      }
    };

  private:
    std::string _formatted;
    std::string _format_string_left;
    sqlstringformat _format;

    std::string consume_until_next_escape();
    int next_escape();

    sqlstring &append(const std::string &s);

  public:
    static const sqlstring null;

    sqlstring();
    sqlstring(const std::string &format_string, const sqlstringformat format = 0);
    sqlstring(const char format_string[], const sqlstringformat format = 0);
    sqlstring(const sqlstring &copy);
    bool done() const;

    operator std::string() const;

    //! modifies formatting options
    sqlstring &operator<<(const sqlstringformat);

    //! replaces a ? in the format string with any integer numeric value
    template <typename T>
    typename boost::enable_if_c<boost::is_integral<T>::value, sqlstring &>::type operator<<(const T value) {
      int esc = next_escape();
      if (esc != '?')
        throw std::invalid_argument("Error formatting SQL query: invalid escape for numeric argument");

      const char *format = (sizeof(T) <= sizeof(int32_t) ? "%i" : "%lli");
      append(strfmt(format, value));
      append(consume_until_next_escape());
      return *this;
    }
    //! replaces a ? in the format string with a float numeric value
    sqlstring &operator<<(const float val) {
      return operator<<((double)val);
    }
    //! replaces a ? in the format string with a double numeric value
    sqlstring &operator<<(const double);
    //! replaces a ? in the format string with a quoted string value or ! with a back-quoted identifier value
    sqlstring &operator<<(const std::string &);
    //! replaces a ? in the format string with a quoted string value or ! with a back-quoted identifier value
    //! is the value is NULL, ? will be replaced with a NULL. ! will raise an exception
    sqlstring &operator<<(const char *);
    //! replaces a ? or ! with the content of the other string verbatim
    sqlstring &operator<<(const sqlstring &);

    bool operator == (const base::sqlstring &second) const {
      return (std::string)*this == (std::string)second;
    }
    bool operator != (const base::sqlstring &second) const {
      return (std::string)*this != (std::string)second;
    }
  };
};
