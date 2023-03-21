/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "common.h"

#include "base/symbol-info.h"

#if !defined(_MSC_VER) && !defined(__APPLE)
#include <glib.h>
#endif

#include <inttypes.h>
#include <string>
#include <list>
#include <vector>
#include <sstream>
#include <typeinfo>
#include <string.h>

#include <boost/optional.hpp>
#include <memory>
#include <iomanip>
#include <cstdint>

#define _(s) s // TODO: replace with localization code.

using std::int64_t;

namespace base {
#define SPACES " \t\r\n"

  BASELIBRARY_PUBLIC_FUNC std::wstring string_to_wstring(const std::string &s);
  BASELIBRARY_PUBLIC_FUNC std::string wstring_to_string(const std::wstring &s);
#ifdef _MSC_VER
  BASELIBRARY_PUBLIC_FUNC std::wstring path_from_utf8(const std::string &s);
#else
  BASELIBRARY_PUBLIC_FUNC std::string path_from_utf8(const std::string &s);
#endif

  // use this to convert a utf8 std::string to a std::string that can be used to open files in windows (noop elsewhere)
  BASELIBRARY_PUBLIC_FUNC std::string string_to_path_for_open(const std::string &s);

  // turns a UTF8 string into something that can be used as a file name (ie, strips out special chars)
  BASELIBRARY_PUBLIC_FUNC std::string sanitize_file_name(const std::string &s);

  // Trimming, cleanup etc.
  BASELIBRARY_PUBLIC_FUNC std::string trim_right(const std::string &s, const std::string &t = SPACES);
  BASELIBRARY_PUBLIC_FUNC std::string trim_left(const std::string &s, const std::string &t = SPACES);
  BASELIBRARY_PUBLIC_FUNC std::string trim(const std::string &s, const std::string &t = SPACES);
  BASELIBRARY_PUBLIC_FUNC std::string tolower(const std::string &s);
  BASELIBRARY_PUBLIC_FUNC std::string toupper(const std::string &s);
  BASELIBRARY_PUBLIC_FUNC std::string truncate_text(const std::string &s, int max_length);
  BASELIBRARY_PUBLIC_FUNC std::string sanitize_utf8(const std::string &s);

  // Parsing/Formatting.
  BASELIBRARY_PUBLIC_FUNC std::string get_identifier(const std::string &id, std::string::const_iterator &start);
  BASELIBRARY_PUBLIC_FUNC std::vector<std::string> split_qualified_identifier(const std::string &id);
  BASELIBRARY_PUBLIC_FUNC std::string strfmt(const char *fmt, ...) G_GNUC_PRINTF(1, 2);
  BASELIBRARY_PUBLIC_FUNC std::string sizefmt(int64_t s, bool metric);
  BASELIBRARY_PUBLIC_FUNC std::string pop_path_front(std::string &path);
  BASELIBRARY_PUBLIC_FUNC std::string pop_path_back(std::string &path);
  BASELIBRARY_PUBLIC_FUNC std::string strip_text(const std::string &text, bool left = true, bool right = true);
  BASELIBRARY_PUBLIC_FUNC std::string replaceVariable(const std::string &format, const std::string &variable,
                                                      const std::string &value);

  BASELIBRARY_PUBLIC_FUNC std::string normalize_path_extension(std::string filename, std::string extension);
  BASELIBRARY_PUBLIC_FUNC std::string normalize_path(const std::string path);
  BASELIBRARY_PUBLIC_FUNC std::string expand_tilde(const std::string &path);
  BASELIBRARY_PUBLIC_FUNC std::string make_valid_filename(const std::string &name);

  BASELIBRARY_PUBLIC_FUNC std::string escape_sql_string(const std::string &string,
                                                        bool wildcards = false); // "strings" or 'strings'
  BASELIBRARY_PUBLIC_FUNC std::string escape_json_string(const std::string &string);
  BASELIBRARY_PUBLIC_FUNC std::string unescape_sql_string(const std::string &string, char escape_char);
  BASELIBRARY_PUBLIC_FUNC std::string escape_backticks(const std::string &string); // `identifier`
  BASELIBRARY_PUBLIC_FUNC std::string extract_option_from_command_line(const std::string &option,
                                                                       const std::string &command_line);

  BASELIBRARY_PUBLIC_FUNC bool parse_font_description(const std::string &fontspec, std::string &font, float &size,
                                                      bool &bold, bool &italic);

  // Searching, splitting etc.
  BASELIBRARY_PUBLIC_FUNC std::string left(const std::string &s, size_t len);
  BASELIBRARY_PUBLIC_FUNC std::string right(const std::string &s, size_t len);
  BASELIBRARY_PUBLIC_FUNC bool hasPrefix(const std::string &s, const std::string &part);
  BASELIBRARY_PUBLIC_FUNC bool hasSuffix(const std::string &s, const std::string &part);
  BASELIBRARY_PUBLIC_FUNC void replaceStringInplace(std::string &value, const std::string &search,
                                                    const std::string &replacement);
  BASELIBRARY_PUBLIC_FUNC std::string replaceString(const std::string &s, const std::string &from,
                                                    const std::string &to);

  /**
   * @brief Split the a string into a vector, using @a sep as a separator
   *
   * This function splits all the words of a string and stores them into a vector. To differentiate
   * each, it uses the @a sep parameter as a separator
   *
   * @param s The string to split
   * @param sep The separator to use
   * @param count The limit of parts to retrieve. Defaults to -1.
   * @return std::vector< std::string, std::allocator >
   */
  BASELIBRARY_PUBLIC_FUNC std::vector<std::string> split(const std::string &s, const std::string &sep, int count = -1);
  /**
   * @brief Split the a string into a vector, using @a sep as a separator
   *
   * This function splits all the words of a string and stores them into a vector. To differenciate
   * each, it uses the @a sep parameter as a list separator characters
   *
   * @param s The string to split
   * @param separator_set The separator set to use. Each character os this string will be used as a separator
   * @param count The limit of parts to retrieve. Defaults to -1.
   * @return std::vector< std::string, std::allocator >
   */
  BASELIBRARY_PUBLIC_FUNC std::vector<std::string> split_by_set(const std::string &s, const std::string &separator_set,
                                                                int count = -1);
  BASELIBRARY_PUBLIC_FUNC std::vector<std::string> split_token_list(const std::string &s, int sep);
  BASELIBRARY_PUBLIC_FUNC bool partition(const std::string &s, const std::string &sep, std::string &left,
                                         std::string &right);
  BASELIBRARY_PUBLIC_FUNC int index_of(const std::vector<std::string> &list, const std::string &s);

  // You cannot export a template function, only specializations. But since the code is in the
  // header we don't need to export.
  template <class C>
  std::string join(const C &list, const std::string &sep) {
    std::string s;
    for (typename C::const_iterator i = list.begin(); i != list.end(); ++i) {
      if (i != list.begin())
        s.append(sep);
      s.append(*i);
    }
    return s;
  }

  BASELIBRARY_PUBLIC_FUNC void setTextFileContent(const std::string &filename, const std::string &data);
  BASELIBRARY_PUBLIC_FUNC std::string getTextFileContent(const std::string &filename);

  BASELIBRARY_PUBLIC_FUNC std::string quote_identifier(const std::string &identifier, const char quote_char);
  BASELIBRARY_PUBLIC_FUNC std::string unquote_identifier(const std::string &identifier);
  BASELIBRARY_PUBLIC_FUNC std::string unquote(const std::string &text);
  BASELIBRARY_PUBLIC_FUNC std::string quoteIdentifierIfNeeded(const std::string &ident, const char quote_char,
                                                              base::MySQLVersion version);

  BASELIBRARY_PUBLIC_FUNC bool stl_string_compare(const std::string &first, const std::string &second,
                                                  bool case_sensitive = true);
  BASELIBRARY_PUBLIC_FUNC int string_compare(const std::string &first, const std::string &second,
                                             bool case_sensitive = true);
  BASELIBRARY_PUBLIC_FUNC bool same_string(const std::string &first, const std::string &second,
                                           bool case_sensitive = true);
  BASELIBRARY_PUBLIC_FUNC bool contains_string(const std::string &text, const std::string &candidate,
                                               bool case_sensitive = true);

  BASELIBRARY_PUBLIC_FUNC bool is_number(const std::string &word);
  BASELIBRARY_PUBLIC_FUNC bool isBool(const std::string &text);

#ifdef __APPLE__
#undef check
#endif

  class BASELIBRARY_PUBLIC_FUNC EolHelpers {
  public:
    enum Eol_format { eol_lf, eol_cr, eol_crlf };
    static Eol_format detect(const std::string &text); // detects eol format based on the first eol occurence, line
                                                       // endings consistency is implied
    static int count_lines(const std::string &text); // counts lines in the text, even if line endings are inconsistent
    static bool check(const std::string &text); // checks whether line endings are consistent (same throughout the text)
    static void conv(
      const std::string &src_text, Eol_format src_eol_format, std::string &dest_text,
      Eol_format dest_eol_format); // converts between 2 known eol formats, line endings consistency is implied
    static void fix(const std::string &src_text, std::string &dest_text,
                    Eol_format eol_format); // aligns eol format to the specified

    static bool is_eol(const char *sym_ptr) // reliably determines whether the given string position contains last
                                            // symbol of eol sequence, line ending inconsistency is allowed
    {
      switch (*sym_ptr) {
        case '\n':
          return true;
        case '\r':
          return ('\n' != *(sym_ptr + 1));
        default:
          return false;
      }
    }

    static Eol_format default_eol_format() // platform default eol format
    {
#if defined(_MSC_VER)
      return eol_crlf;
#elif defined(__APPLE__)
      return eol_cr;
#else
      return eol_lf;
#endif
    }

    static const std::string &eol(Eol_format eol = default_eol_format()) // returns eol sequence by eol format
    {
      static std::string eol_crlf_seq = "\r\n";
      static std::string eol_cr_seq = "\r";
      static std::string eol_lf_seq = "\n";
      switch (eol) {
        case eol_crlf:
          return eol_crlf_seq;
        case eol_cr:
          return eol_cr_seq;
        default:
          return eol_lf_seq;
      }
    }
  };

  /**
   * @brief Splits a string into several lines
   * *
   * This function splits the given string into several lines, depending on the line_length.
   * You can prefix each line using the left_fill string, which can be ignored if the line_length is
   * too small. For line_length lower than the minimum length (5) an empty string is returned.
   * When the text is too big and max_lines is reached, it will truncate the text and add ellipses,
   * so you should account for an extra line in this situation.
   * @param text The string to be split.
   * @param line_length The length of the line in characters.
   * @param left_fill A string prefixing each line. This will not be used if the line_length is too small.
   *                  Defaults to "".
   * @param indent_first True to indent the first line. Defaults to true.
   * @param max_lines The maximum amount of lines. Defaults to 30.
   * @return A string split into several lines. If the string is not encoded in utf8 or the
   *         line_length is too small, it will return an empty string.
   **/
  BASELIBRARY_PUBLIC_FUNC std::string reflow_text(const std::string &text, unsigned int line_length,
                                                  const std::string &left_fill = "", bool indent_first = true,
                                                  unsigned int max_lines = 30);

#ifdef _MSC_VER
  const static std::string engLocale = "en-US";
#else
  const static std::string engLocale = "en_US.UTF-8";
#endif

  template <typename T>
  std::string to_string(T val, const std::locale &loc = std::locale(engLocale.c_str())) {
    static_assert(std::is_same<float, typename std::decay<T>::type>::value ||
                    std::is_same<double, typename std::decay<T>::type>::value,
                  "Only float or double data types are allowed.");

    struct NoThousandsSep : std::numpunct<char> {
      std::string do_grouping() const {
        return "";
      } // groups of 1 digit
    };

    std::stringstream ss;
    ss.imbue(std::locale(loc, new NoThousandsSep));
    ss << std::setprecision(15) << val;
    return ss.str();
  }

  /**
   * @brief Parse a string and return a numeric value if the content of the string can be interpreted as such.
   * This function parses std::string and return it's numeric value. It can throw std::exception in case of a failure.
   *
   * @param text The string to be parsed.
   * @param T Default value to return if it's not possible to convert. If no default value is given
   *          a bad_cast exception is thrown on parse errors.
   * @return Extracted numeric value.
   */
  class ConvertHelper {
    // Need to make this public as we cannot declare the template functions that use it as friend.
  public:
    template <typename T, typename U>
    struct is_same {
      static const bool value = false;
    };

    template <typename T>
    struct is_same<T, T> {
      static const bool value = true;
    };

    template <typename T>
    T static string_to_number(const std::string &val, boost::optional<T> def_val = boost::none) {
      T tmp;
      std::stringstream ss(val);
      ss >> tmp;
      if (ss.rdstate() & std::stringstream::failbit) {
        if (def_val)
          return def_val.get();
        throw std::bad_cast();
      }
      return tmp;
    }
  };

  template <typename T>
  T inline atoi(const std::string &val, boost::optional<T> def_val = boost::none) {
    // Don't remove the double parentheses. GCC needs them here.
    BOOST_STATIC_ASSERT((ConvertHelper::is_same<T, int>::value || ConvertHelper::is_same<T, long>::value ||
                         ConvertHelper::is_same<T, long long>::value || ConvertHelper::is_same<T, size_t>::value ||
                         ConvertHelper::is_same<T, ssize_t>::value || ConvertHelper::is_same<T, int64_t>::value));

    return ConvertHelper::string_to_number<T>(val, def_val);
  }

  template <typename T>
  T inline atof(const std::string &val, boost::optional<T> def_val = boost::none) {
    BOOST_STATIC_ASSERT((ConvertHelper::is_same<T, double>::value || ConvertHelper::is_same<T, float>::value));

    return ConvertHelper::string_to_number<T>(val, def_val);
  }

  typedef std::list<std::string> StringList;
  typedef std::shared_ptr<StringList> StringListPtr;

} // namespace base
