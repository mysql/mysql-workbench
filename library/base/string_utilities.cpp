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

#include "base/string_utilities.h"
#include "base/file_functions.h"
#include "base/log.h"

#include <stdexcept>
#include <functional>
#include <locale>
#include <algorithm>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <fstream>
#include <boost/locale/encoding_utf.hpp>

DEFAULT_LOG_DOMAIN(DOMAIN_BASE);

namespace base {

#ifdef _MSC_VER

  // Win uses C++11 with support for wstring_convert. Other platforms use boost for now.

  //--------------------------------------------------------------------------------------------------

  thread_local static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> utf16Converter;
  thread_local static std::wstring_convert<std::codecvt_utf8<__int32>, __int32> utf32Converter;

  /**
   * Converts an UTF-8 encoded string to an UTF-16 string.
   */
  std::wstring string_to_wstring(const std::string &s) {
    if (sizeof(wchar_t) > 2) {
      auto utf32String = utf32Converter.from_bytes(s);
      return std::wstring(utf32String.begin(), utf32String.end());
    } else
      return utf16Converter.from_bytes(s);
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Converts an UTF-16 encoded string to an UTF-8 string.
   */
  std::string wstring_to_string(const std::wstring &s) {
    if (sizeof(wchar_t) > 2)
      return utf32Converter.to_bytes((__int32 *)s.c_str());
    else
      return utf16Converter.to_bytes(s);
  }

  //--------------------------------------------------------------------------------------------------

  std::wstring path_from_utf8(const std::string &s) {
    return string_to_wstring(s);
  }

#else

  using boost::locale::conv::utf_to_utf;

  std::wstring string_to_wstring(const std::string &str) {
    return utf_to_utf<wchar_t>(str.c_str(), str.c_str() + str.size());
  }

  //--------------------------------------------------------------------------------------------------

  std::string wstring_to_string(const std::wstring &str) {
    if (sizeof(wchar_t) > 2)
      return utf_to_utf<char>((int32_t *)str.c_str(), (int32_t *)str.c_str() + str.size());
    else
      return utf_to_utf<char>(str.c_str(), str.c_str() + str.size());
  }

  //--------------------------------------------------------------------------------------------------

  std::string path_from_utf8(const std::string &s) {
    return s;
  }

#endif

  //--------------------------------------------------------------------------------------------------

  std::string string_to_path_for_open(const std::string &s) {
// XXX: convert from utf-8 to wide string and then back to utf-8?
//      How can this help in any way here?
#ifdef _MSC_VER
    std::wstring ws = string_to_wstring(s);
    int buflen = GetShortPathNameW(ws.c_str(), NULL, 0);
    if (buflen > 0) {
      wchar_t *buffer = g_new(wchar_t, buflen);
      if (GetShortPathNameW(ws.c_str(), buffer, buflen) > 0) {
        char *buffer2;
        buflen = WideCharToMultiByte(CP_UTF8, 0, buffer, buflen, NULL, 0, 0, 0);
        buffer2 = g_new(char, buflen);
        if (WideCharToMultiByte(CP_UTF8, 0, buffer, buflen, buffer2, buflen, 0, 0) == 0) {
          std::string path(buffer2);
          g_free(buffer2);
          g_free(buffer);
          return path;
        }
        g_free(buffer2);
      }
      g_free(buffer);
    }
    return s;
#else
    return s;
#endif
  }

  //--------------------------------------------------------------------------------------------------

  inline bool is_invalid_filesystem_char(int ch) {
    static const char invalids[] = "/?<>\\:*|\"^";

    return memchr(invalids, ch, sizeof(invalids) - 1) != NULL;
  }

  std::string sanitize_file_name(const std::string &s) {
    static const char *invalid_filenames[] = {"com1", "com2", "com3", "com4", "com5", "com6", "com7", "com8",
                                              "com9", "lpt1", "lpt2", "lpt3", "lpt4", "lpt5", "lpt6", "lpt7",
                                              "lpt8", "lpt9", "con",  "nul",  "prn",  ".",    "..",   NULL};
    std::string out;

    for (std::string::const_iterator c = s.begin(); c != s.end(); ++c) {
      // utf-8 has the high-bit = 1, so we just copy those verbatim
      if ((unsigned char)*c >= 128 || isalnum(*c) || (ispunct(*c) && !is_invalid_filesystem_char(*c)))
        out.push_back(*c);
      else
        out.push_back('_');
    }

    // not valid under windows
    if (!out.empty() && (out[out.size() - 1] == ' ' || out[out.size() - 1] == '.'))
      out[out.size() - 1] = '_';

    for (const char **fn = invalid_filenames; *fn; ++fn) {
      if (strcmp(out.c_str(), *fn) == 0) {
        out.append("_");
        break;
      }
    }

    return out;
  }

  //--------------------------------------------------------------------------------------------------

  std::string trim_right(const std::string &s, const std::string &t) {
    std::string d(s);
    std::string::size_type i(d.find_last_not_of(t));
    if (i == std::string::npos)
      return "";
    else
      return d.erase(d.find_last_not_of(t) + 1);
  }

  //--------------------------------------------------------------------------------------------------

  std::string trim_left(const std::string &s, const std::string &t) {
    std::string d(s);
    return d.erase(0, s.find_first_not_of(t));
  }

  //--------------------------------------------------------------------------------------------------

  std::string trim(const std::string &s, const std::string &t) {
    std::string d(s);
    return trim_left(trim_right(d, t), t);
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Simple case conversion routine, which returns a new string.
   * Note: converting to lower can be wrong when the returned string is used for string comparison,
   * because in some cultures letter cases are more complicated. Use string_compare instead in such cases.
   */
  std::string tolower(const std::string &s) {
    char *str_down = g_utf8_strdown(s.c_str(), (gsize)s.length());
    std::string result(str_down);
    g_free(str_down);
    return result;
  }

  //--------------------------------------------------------------------------------------------------

  std::string toupper(const std::string &s) {
    char *str_up = g_utf8_strup(s.c_str(), (gsize)s.length());
    std::string result(str_up);
    g_free(str_up);
    return result;
  }

  //--------------------------------------------------------------------------------------------------

  std::string truncate_text(const std::string &s, int max_length) {
    if ((int)s.length() > max_length) {
      std::string shortened(s.substr(0, max_length));
      const char *prev = g_utf8_find_prev_char(shortened.c_str(), shortened.c_str() + (max_length - 1));
      if (prev) {
        shortened.resize(prev - shortened.c_str(), 0);
        shortened.append("...");
      }
      return shortened;
    }
    return s;
  }

  //--------------------------------------------------------------------------------------------------

  std::string sanitize_utf8(const std::string &s) {
    const char *end = 0;
    if (!g_utf8_validate(s.data(), (gsize)s.size(), &end))
      return std::string(s.data(), end);
    return s;
  }

  //--------------------------------------------------------------------------------------------------

  std::vector<std::string> split(const std::string &s, const std::string &sep, int count) {
    std::vector<std::string> parts;
    std::string ss = s;

    std::string::size_type p;

    if (s.empty())
      return parts;

    if (count == 0)
      count = -1;

    p = ss.find(sep);
    while (!ss.empty() && p != std::string::npos && (count < 0 || count > 0)) {
      parts.push_back(ss.substr(0, p));
      ss = ss.substr(p + sep.size());

      --count;
      p = ss.find(sep);
    }
    parts.push_back(ss);

    return parts;
  }

  //--------------------------------------------------------------------------------------------------

  std::vector<std::string> split_by_set(const std::string &s, const std::string &separator_set, int count) {
    std::vector<std::string> parts;
    std::string ss = s;

    std::string::size_type p;

    if (s.empty())
      return parts;

    if (count == 0)
      count = -1;

    p = ss.find_first_of(separator_set);
    while (!ss.empty() && p != std::string::npos && (count < 0 || count > 0)) {
      parts.push_back(ss.substr(0, p));
      ss = ss.substr(p + 1);

      --count;
      p = ss.find_first_of(separator_set);
    }
    parts.push_back(ss);

    return parts;
  }

  //--------------------------------------------------------------------------------------------------

  static void findUntil(const char elem, const std::string &str, const int sep, std::string::size_type &p,
                        std::string::size_type &pe, std::string::size_type &end, std::vector<std::string> &parts) {
    // keep going until we find closing '
    while (pe < end) {
      auto it = str[pe++];
      if (it == elem) {
        if (pe < end && str[pe] == elem)
          pe++;
        else
          break;
      } else if (it == '\\') {
        if (pe < end)
          pe++;
      }
    }
    parts.push_back(str.substr(p, pe - p));
    p = pe;
    // skip whitespace
    while (p < end && (str[p] == ' ' || str[p] == '\t' || str[p] == '\r' || str[p] == '\n'))
      p++;
    if (p < end) {
      if (str[p] != sep)
        logDebug("Error splitting string list\n");
      else
        p++;
    }
  }

  std::vector<std::string> split_token_list(const std::string &s, int sep) {
    std::vector<std::string> parts;
    std::string ss = s;

    std::string::size_type end = s.size(), pe, p = 0;

    {
      bool empty_pending = true;
      while (p < end) {
        empty_pending = false;
        switch (s[p]) {
          case '\'':
            pe = p + 1;
            findUntil('\'', s, sep, p, pe, end, parts);
            break;

          case '"':
            pe = p + 1;
            findUntil('"', s, sep, p, pe, end, parts);
            break;

          case ' ':
          case '\t':
            p++;
            break;

          default:
            // skip until separator
            pe = p;
            while (pe < end) {
              if (s[pe] == sep) {
                empty_pending = true;
                break;
              }
              pe++;
            }
            parts.push_back(trim_right(s.substr(p, pe - p)));
            p = pe + 1;
            // skip whitespace
            while (p < end && (s[p] == ' ' || s[p] == '\t' || s[p] == '\r' || s[p] == '\n'))
              p++;
            break;
        }
      }
      if (empty_pending)
        parts.push_back("");
    }

    return parts;
  }

  //--------------------------------------------------------------------------------------------------

  bool partition(const std::string &s, const std::string &sep, std::string &left, std::string &right) {
    std::string::size_type p = s.find(sep);
    if (p != std::string::npos) {
      left = s.substr(0, p);
      right = s.substr(p + sep.size());
      return true;
    }
    left = s;
    right = "";
    return false;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Returns the index of the given string in the given vector or -1 if not found.
   */
  int index_of(const std::vector<std::string> &list, const std::string &s) {
    std::vector<std::string>::const_iterator location = std::find(list.begin(), list.end(), s);
    if (location == list.end())
      return -1;
    return (int)(location - list.begin());
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Returns a string containing all characters beginning at "start" from the given string "id", which form
   * a valid, unqualified identifier. The returned identifier does not contain any quoting anymore.
   * Note: this function is UTF-8 safe as it skips over all characters except some which are guaranteed
   *       not to be part of any valid UTF-8 sequence.
   *
   * @param id The string to examine.
   * @param start The start position to search from.
   *
   * @result Returns the first found identifier starting at "start" or an empty string if nothing was
   *         found. Parameter "start" points to the first character after the found identifier.
   */
  std::string get_identifier(const std::string &id, std::string::const_iterator &start) {
    std::string::const_iterator token_end = id.end();
    bool is_symbol_quoted = false;
    for (std::string::const_iterator i = start, i_end = token_end; i != i_end; ++i) {
      if (i_end != token_end)
        break;
      switch (*i) {
        case '.':
          if (!is_symbol_quoted)
            token_end = i;
          break;
        case ' ':
          if (!is_symbol_quoted)
            token_end = i;
          break;
        case '\'':
        case '"':
        case '`':
          if (*i == *start) {
            if (i != start)
              token_end = i + 1;
            else
              is_symbol_quoted = true;
          }
          break;
      }
    }

    if (token_end - start < 2)
      is_symbol_quoted = false;
    std::string result(start, token_end);
    start = token_end;
    if (is_symbol_quoted)
      return result.substr(1, result.size() - 2);

    return result;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Splits the given string into identifier parts assuming a format as allowed by the MySQL syntax for
   * qualified identifiers, e.g. part1.part2.part3 (any of the parts might be quoted).
   * In addition to the traditional syntax also these enhancements are supported:
   * - Unlimited level of nesting.
   * - Quoting might be done using single quotes, double quotes and back ticks.
   *
   * If an identifier is not separated by a dot from the rest of the input then this is considered
   * invalid input and ignored. Only identifiers found until that syntax violation are returned.
   */
  std::vector<std::string> split_qualified_identifier(const std::string &id) {
    std::vector<std::string> result;
    std::string::const_iterator iterator = id.begin();
    std::string token;
    do {
      token = get_identifier(id, iterator);
      if (token == "")
        break;
      result.push_back(token);
    } while ((iterator != id.end()) && (*iterator++ == '.'));

    return result;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Removes the first path part from @path and returns this part as well as the shortend path.
   */
  std::string pop_path_front(std::string &path) {
    std::string::size_type p = path.find('/');
    std::string res;
    if (p == std::string::npos || p == path.length() - 1) {
      res = path;
      path.clear();
      return res;
    }
    res = path.substr(0, p);
    path = path.substr(p + 1);
    return res;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Removes the last path part from @path and returns this part as well as the shortend path.
   */
  std::string pop_path_back(std::string &path) {
    std::string::size_type p = path.rfind('/');
    std::string res;
    if (p == std::string::npos || p == path.length() - 1) {
      res = path;
      path.clear();
      return res;
    }
    res = path.substr(p + 1);
    path = path.substr(0, p);
    return res;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Helper routine to format a string into an STL string using the printf parameter syntax.
   */
  std::string strfmt(const char *fmt, ...) {
    va_list args;
    char *tmp;
    std::string ret;

    va_start(args, fmt);
    tmp = g_strdup_vprintf(fmt, args);
    va_end(args);

    ret = tmp;
    g_free(tmp);

    return ret;
  }

  //--------------------------------------------------------------------------------------------------

  BASELIBRARY_PUBLIC_FUNC std::string sizefmt(int64_t s, bool metric) {
    float one_kb;
    const char *unit;
    if (metric) {
      one_kb = 1000;
      unit = "B";
    } else {
      one_kb = 1024;
      unit = "iB"; // http://en.wikipedia.org/wiki/Binary_prefix
    }

    if (s < one_kb)
      return strfmt("%iB", (int)s);
    else {
      float value = s / one_kb;
      if (value < one_kb)
        return strfmt("%.02fK%s", value, unit);
      else {
        value /= one_kb;
        if (value < one_kb)
          return strfmt("%.02fM%s", value, unit);
        else {
          value /= one_kb;
          if (value < one_kb)
            return strfmt("%.02fG%s", value, unit);
          else {
            value /= one_kb;
            if (value < one_kb)
              return strfmt("%.02fT%s", value, unit);
            else
              return strfmt("%.02fP%s", value / one_kb, unit);
          }
        }
      }
    }
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Helper routine to strip a string into an STL string using the printf parameter syntax.
   */
  std::string strip_text(const std::string &text, bool left, bool right) { // TODO sigc rewrite it in std/boost way
    std::locale loc;
    std::function<bool(std::string::value_type)> is_space =
      std::bind(&std::isspace<std::string::value_type>, std::placeholders::_1, loc);

    std::string::const_iterator l_edge =
      !left ? text.begin()
            : std::find_if(text.begin(), text.end(),
                           std::bind(std::logical_not<bool>(), std::bind(is_space, std::placeholders::_1)));
    std::string::const_reverse_iterator r_edge =
      !right ? text.rbegin()
             : std::find_if(text.rbegin(), text.rend(),
                            std::bind(std::logical_not<bool>(), std::bind(is_space, std::placeholders::_1)));

    return std::string(l_edge, r_edge.base());
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * replaces a variable from a string in format %variable%
   * a filter can be passed to the variable as in %variable|filter%
   * supported filters are upper, lower and capitalize
   */
  std::string replaceVariable(const std::string &format, const std::string &variable, const std::string &value) {
    std::string result = format;
    std::string::size_type pos;

    for (;;) {
      std::string s;
      std::string::size_type end;

      pos = result.find(variable.substr(0, variable.size() - 1));
      if (pos == std::string::npos)
        break;

      end = result.find('%', pos + 1);
      if (end == std::string::npos) // bad format
        break;

      s = result.substr(pos + 1, end - pos - 1);

      std::string::size_type filter_pos = s.find("|");
      std::string filtered_value = value;

      if (filter_pos == std::string::npos) {
        if (s.length() != variable.length() - 2)
          break;
      } else if (filter_pos != variable.length() - 2)
        break;
      else {
        std::string filter = s.substr(filter_pos + 1, s.size() - filter_pos);

        if (filter.compare("capitalize") == 0) {
          gunichar ch = g_utf8_get_char(value.data());

          ch = g_unichar_toupper(ch);

          gchar *rest = g_utf8_find_next_char(value.data(), value.data() + value.size());
          char utf8[10];
          utf8[g_unichar_to_utf8(ch, utf8)] = 0;
          filtered_value = std::string(utf8).append(rest);
        } else if (filter.compare("uncapitalize") == 0) {
          gunichar ch = g_utf8_get_char(value.data());

          ch = g_unichar_tolower(ch);

          gchar *rest = g_utf8_find_next_char(value.data(), value.data() + value.size());
          char utf8[10];
          utf8[g_unichar_to_utf8(ch, utf8)] = 0;
          filtered_value = std::string(utf8).append(rest);
        } else if (filter.compare("lower") == 0) {
          gchar *l = g_utf8_strdown(value.data(), (gssize)value.size());
          if (l)
            filtered_value = l;
          g_free(l);
        } else if (filter.compare("upper") == 0) {
          gchar *l = g_utf8_strup(value.data(), (gssize)value.size());
          if (l)
            filtered_value = l;
          g_free(l);
        }
      }
      result = result.substr(0, pos).append(filtered_value).append(result.substr(end + 1));
    }

    return result;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Add the given extension to the filename, if necessary.
   *
   */
  std::string normalize_path_extension(std::string filename, std::string extension) {
    if (!extension.empty() && !filename.empty()) {
      std::string::size_type p = filename.rfind('.');
      std::string old_extension = p != std::string::npos ? filename.substr(p) : "";

      if (old_extension.find('/') != std::string::npos || old_extension.find('\\') != std::string::npos)
        old_extension.clear();

      if (!extension.empty() && extension[0] != '.')
        extension = "." + extension;

      if (old_extension.empty())
        filename.append(extension);
      else {
        if (old_extension != extension)
          filename = filename.substr(0, p).append(extension);
      }
    }
    return filename;
  }

  /**
   * Removes all unnecessary path separators as well as "./" combinations.
   * If there is a parent-dir entry (../) then this as well as the directly prefacing
   * dir entry is removed.
   */
  std::string normalize_path(const std::string path) {
    // First convert all separators to the one that is used on the platform (no mix)
    // and ease so at the same time further processing here.
    std::string result;
    std::string separator(1, G_DIR_SEPARATOR);

    result = path;
    replaceStringInplace(result, "\\", separator);
    replaceStringInplace(result, "/", separator);

    std::string double_separator = separator + separator;
    while (result.find(double_separator) != std::string::npos)
      replaceStringInplace(result, double_separator, separator);

    // Sanity check. Return *after* we have converted the slashs. This is part of the normalization.
    if (result.size() < 2)
      return result;

    std::vector<std::string> parts = split(result, separator);

    // Construct result backwards while examining the path parts.
    result = "";
    int pending_count = 0;
    for (ssize_t i = parts.size() - 1; i >= 0; i--) {
      if (parts[i].compare(".") == 0)
        // References to the current directory can be removed without further change.
        continue;

      if (parts[i].compare("..") == 0) {
        // An entry that points back to the parent dir.
        // Ignore this and keep track for later removal of the parent dir.
        pending_count++;
      } else if (pending_count > 0) {
        // If this is a normal dir entry and we have pending parent-dir redirections
        // then go one step up by removing (ignoring) this entry.
        pending_count--;
      } else
        result = separator + parts[i] + result;
    }

    // Don't return the leading separator.
    return result.substr(1);
  }

  std::string expand_tilde(const std::string &path) {
    if (!path.empty() && path[0] == '~' && (path.size() == 1 || path[1] == G_DIR_SEPARATOR)) {
      const char *homedir = g_getenv("HOME");
      if (!homedir)
        homedir = g_get_home_dir();

      return std::string(homedir).append(path.substr(1));
    }
    return path;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Checks the input for characters not allowed in the file system and converts them to underscore.
   */
  std::string make_valid_filename(const std::string &name) {
    std::string result;
    std::string illegal_chars = "\\/:?\"<>|*";
    for (std::string::const_iterator iterator = name.begin(); iterator != name.end(); ++iterator) {
      if (illegal_chars.find(*iterator) != std::string::npos)
        result += '_';
      else
        result += *iterator;
    }
    return result;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Get a string containing the 'len' left most characters.
   */
  std::string left(const std::string &s, size_t len) {
    return s.substr(0, len);
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Get a string containing the 'len' right most characters.
   */
  std::string right(const std::string &s, size_t len) {
    if (len > s.size())
      len = s.size();
    if (len < 1)
      return "";

    return s.substr(std::max(s.length() - len, (size_t)0));
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Tests if s begins with part.
   */
  bool hasPrefix(const std::string &s, const std::string &part) {
    return s.compare(0, part.length(), part) == 0;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Tests if s ends with part.
   */
  bool hasSuffix(const std::string &s, const std::string &part) {
    int start_at = (int)s.length() - (int)part.length();

    // If start_at < 0 then the search string is bigger then the source, so the results is false.
    // On the other hand, if it starts after the end, something went wrong...
    if (start_at < 0 || start_at > (int)s.length())
      return false;

    return s.compare(start_at, std::string::npos, part) == 0;
  }

  //--------------------------------------------------------------------------------------------------

  void replaceStringInplace(std::string &value, const std::string &search, const std::string &replacement) {
    std::string::size_type next;

    for (next = value.find(search); next != std::string::npos; next = value.find(search, next)) {
      value.replace(next, search.length(), replacement);
      next += replacement.length();
    }
  }

  //--------------------------------------------------------------------------------------------------

  std::string replaceString(const std::string &s, const std::string &from, const std::string &to) {
    std::string::size_type p;
    std::string ss, res;

    ss = s;
    p = ss.find(from);
    while (p != std::string::npos) {
      if (p > 0)
        res.append(ss.substr(0, p)).append(to);
      else
        res.append(to);
      ss = ss.substr(p + from.size());
      p = ss.find(from);
    }
    res.append(ss);

    return res;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Write text data to file, converting to \r\n if in Windows.
   */
  void setTextFileContent(const std::string &filename, const std::string &data) {
#ifdef _MSC_VER
    // Opening a file in text mode will automatically convert \n to \r\n.
    FILE *f = base_fopen(filename.c_str(), "w+t");
    if (!f)
      throw std::runtime_error(g_strerror(errno));

    size_t bytes_written = fwrite(data.data(), 1, data.size(), f);
    fclose(f);
    if (bytes_written != data.size())
      throw std::runtime_error(g_strerror(errno));
#else
    GError *error = NULL;
    g_file_set_contents(filename.c_str(), data.data(), data.size(), &error);
    if (error) {
      std::string msg = error->message;
      g_error_free(error);
      throw std::runtime_error(msg);
    }
#endif
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Reads text data from the given file (file name encoded as utf-8) and returns the content as utf-8.
   * It can read ASCII/ANSI, utf-8 and utf-16 files (LE only) with and without BOM (BOM not included in result).
   */
  std::string getTextFileContent(const std::string &filename) {
    enum Encoding { ANSI, UTF8, UTF16LE } encoding = ANSI;

    std::string result;
#ifdef _MSC_VER
    std::ifstream stream(string_to_wstring(filename).c_str(), std::ios::binary);
#else
    std::ifstream stream(filename.c_str(), std::ifstream::binary);
#endif

    if (!stream.is_open() || stream.eof())
      return "";

    int ch1 = stream.get();
    int ch2 = stream.get();
    if (ch1 == 0xff && ch2 == 0xfe)
      encoding = UTF16LE;
    else if (ch1 == 0xfe && ch2 == 0xff)
      return "UTF-16BE not supported";
    else {
      int ch3 = stream.get();
      if (ch1 == 0xef && ch2 == 0xbb && ch3 == 0xbf)
        encoding = UTF8;
      else
        stream.seekg(0);
    }

    std::string tmp;
    stream.seekg(0, std::ios::end);
    tmp.reserve(stream.tellg());
    stream.seekg(0, std::ios::beg);

    tmp.assign((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    switch (encoding) {
      case UTF16LE:
        return wstring_to_string(std::wstring((const wchar_t *)tmp.data()));
      default:
        return tmp;
    }
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Escape a string to be used in a SQL query
   * Same code as used by mysql. Handles null bytes in the middle of the string.
   * If wildcards is true then _ and % are masked as well.
   */
  std::string escape_sql_string(const std::string &s, bool wildcards) {
    std::string result;
    result.reserve(s.size());

    for (std::string::const_iterator ch = s.begin(); ch != s.end(); ++ch) {
      char escape = 0;

      switch (*ch) {
        case 0: /* Must be escaped for 'mysql' */
          escape = '0';
          break;
        case '\n': /* Must be escaped for logs */
          escape = 'n';
          break;
        case '\r':
          escape = 'r';
          break;
        case '\\':
          escape = '\\';
          break;
        case '\'':
          escape = '\'';
          break;
        case '"': /* Better safe than sorry */
          escape = '"';
          break;
        case '\032': /* This gives problems on Win32 */
          escape = 'Z';
          break;
        case '_':
          if (wildcards)
            escape = '_';
          break;
        case '%':
          if (wildcards)
            escape = '%';
          break;
      }
      if (escape) {
        result.push_back('\\');
        result.push_back(escape);
      } else
        result.push_back(*ch);
    }
    return result;
  }

  /**
   * Escape a string to be used in a JSON
   */
  std::string escape_json_string(const std::string &s) {
    std::string result;
    result.reserve(s.size());
    for (auto ch : s) {
      char escape = 0;
      switch (ch) {
        case '"':
          escape = '"';
          break;
        case '\\':
          escape = '\\';
          break;
        case '\b':
          escape = 'b';
          break;
        case '\f':
          escape = 'f';
          break;
        case '\n':
          escape = 'n';
          break;
        case '\r':
          escape = 'r';
          break;
        case '\t':
          escape = 't';
          break;
        default:
          break;
      }
      if (escape) {
        result.push_back('\\');
        result.push_back(escape);
      } else
        result.push_back(ch);
    }
    return result;
  }

  /**
   * Removes repeated quote chars and supported escape sequences from the given string.
   * Invalid escape sequences are handled like in the server, by dropping the backslash and
   * using the wrong char as normal char.
   * The outer quoting stays intact and is not removed.
   */
  std::string unescape_sql_string(const std::string &s, char quote_char) {
    // Early out if the string is simply empty but quoted.
    if (s.size() == 2 && s[0] == quote_char && s[1] == quote_char)
      return s;

    std::string result;
    result.reserve(s.size());

    bool pendingQuote = false;
    bool pendingEscape = false;
    for (auto c : s) {
      if (!pendingEscape && c == quote_char) {
        if (pendingQuote)
          pendingQuote = false;
        else {
          pendingQuote = true;
          continue;
        }
      } else {
        if (pendingQuote) {
          pendingQuote = false;
          result.push_back(quote_char);
        }

        if (pendingEscape) {
          pendingEscape = false;
          switch (c) {
            case 'n':
              c = '\n';
              break;
            case 't':
              c = '\t';
              break;
            case 'r':
              c = '\r';
              break;
            case 'b':
              c = '\b';
              break;
            case '0':
              c = 0;
              break; // ASCII null
            case 'Z':
              c = '\032';
              break; // Win32 end of file
          }
        } else if (c == '\\') {
          pendingEscape = true;
          continue;
        }
      }
      result.push_back(c);
    }

    if (pendingQuote)
      result.push_back(quote_char);
    if (pendingEscape)
      result.push_back('\\');

    return result;
  }

  //--------------------------------------------------------------------------------------------------

  // NOTE: This is not the same as escape_sql_string, as embedded ` must be escaped as ``, not \`
  // and \ ' and " must not be escaped
  std::string escape_backticks(const std::string &s) {
    std::string result;
    result.reserve(s.size());

    for (std::string::const_iterator ch = s.begin(); ch != s.end(); ++ch) {
      char escape = 0;

      switch (*ch) {
        case 0: /* Must be escaped for 'mysql' */
          escape = '0';
          break;
        case '\n': /* Must be escaped for logs */
          escape = 'n';
          break;
        case '\r':
          escape = 'r';
          break;
        case '\032': /* This gives problems on Win32 */
          escape = 'Z';
          break;
        case '`':
          // special case
          result.push_back('`');
          break;
      }
      if (escape) {
        result.push_back('\\');
        result.push_back(escape);
      } else
        result.push_back(*ch);
    }
    return result;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Parses the given command line (which must be a usual mysql start command) and extracts the
   * value for the given parameter. The function can only return options of the form "option-name = option-value"
   * (both quoted and unquoted).
   */
  std::string extract_option_from_command_line(const std::string &option, const std::string &command_line) {
    std::string result;
    size_t position = command_line.find(option);
    if (position != std::string::npos) {
      position += option.size(); // Skip option name and find equal sign.
      while (position < command_line.size() && command_line[position] != '=')
        position++;

      if (command_line[position] == '=') {
        position++;

        // Skip any white space.
        while (position < command_line.size() && command_line[position] == ' ')
          position++;

        char terminator;
        if (command_line[position] == '"' || command_line[position] == '\'')
          terminator = command_line[position++];
        else
          terminator = ' ';

        size_t end_position = command_line.find(terminator, position);
        if (end_position == std::string::npos) {
          // Terminator not found means the string was either not properly terminated (if quoted)
          // or contains no space char. In this case take everything we can get.
          if (terminator != ' ')
            position++;
          result = command_line.substr(position);
        } else
          result = command_line.substr(position, end_position - position);
      }
    }
    return result;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Splits the given font description and returns its details in the provided fields.
   *
   * @return True if successful, otherwise false.
   */
  bool parse_font_description(const std::string &fontspec, std::string &font, float &size, bool &bold, bool &italic) {
    std::vector<std::string> parts = split(fontspec, " ");
    font = fontspec;
    size = 12;
    bold = false;
    italic = false;

    if (parts.empty())
      return false;

    for (std::vector<std::string>::iterator iter = parts.begin(); iter != parts.end(); ++iter) {
      float size_check = 0;
      if (sscanf(iter->c_str(), "%f", &size_check) == 1) {
        size = size_check;
        parts.erase(iter);
        break;
      }
    }
    /*
      if (!parts.empty() && sscanf(parts.back().c_str(), "%f", &size) == 1)
        parts.pop_back();*/

    for (int i = 0; i < 2 && !parts.empty(); i++) {
      if (g_ascii_strcasecmp(parts.back().c_str(), "bold") == 0) {
        bold = true;
        parts.pop_back();
      }

      if (g_ascii_strcasecmp(parts.back().c_str(), "italic") == 0) {
        italic = true;
        parts.pop_back();
      }
    }

    if (!parts.empty()) {
      font = parts[0];
      for (unsigned int i = 1; i < parts.size(); i++)
        font += " " + parts[i];
    }
    return true;
  }

  //--------------------------------------------------------------------------------------------------

  std::string unquote_identifier(const std::string &identifier) {
    int start = 0;
    int size = (int)identifier.size();

    if (size == 0)
      return "";

    if (identifier[0] == '"' || identifier[0] == '`')
      start++;

    if (identifier[size - 1] == '"' || identifier[size - 1] == '`')
      size--;

    size -= start;

    return identifier.substr(start, size);
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Remove outer quotes from any text.
   *
   * @param text Text to unquote
   * @return Return unqoted text.
   */
  std::string unquote(const std::string &text) {
    if (text.size() < 2)
      return text;

    if ((text[0] == '"' || text[0] == '`' || text[0] == '\'') && text[0] == text[text.size() - 1])
      return text.substr(1, text.size() - 2);
    return text;
  }

  //--------------------------------------------------------------------------------------------------

  std::string quote_identifier(const std::string &identifier, const char quote_char) {
    return quote_char + identifier + quote_char;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Quotes the given identifier, but only if it needs to be quoted.
   */
  std::string quoteIdentifierIfNeeded(const std::string &ident, const char quote_char, MySQLVersion version) {
    bool needs_quotation = MySQLSymbolInfo::isReservedKeyword(ident, version);
    size_t digits = 0;

    if (!needs_quotation) {
      for (std::string::const_iterator i = ident.begin(); i != ident.end(); ++i) {
        if ((*i >= 'a' && *i <= 'z') || (*i >= 'A' && *i <= 'Z') || (*i >= '0' && *i <= '9') || (*i == '_') ||
            (*i == '$') || ((unsigned char)(*i) > 0x7F)) {
          if (*i >= '0' && *i <= '9')
            digits++;

          continue;
        }
        needs_quotation = true;
        break;
      }
    }

    if (needs_quotation || digits == ident.length())
      return quote_char + ident + quote_char;
    else
      return ident;
  }

  bool is_number(const std::string &word) {
    if (word.empty())
      return false;
    size_t i = 0;
    if (word[0] == '-')
      i++;
    for (; i < word.size(); i++)
      if (!isdigit(word[i]))
        return false;
    return true;
  }

  //--------------------------------------------------------------------------------------------------

  /**
  * @brief Determine if a string is a boolean.
  *
  * @param text Text to check
  * @return Return true if given string is a boolean.
  **/
  bool isBool(const std::string &text) {
    std::string transformed;
    std::transform(text.begin(), text.end(), std::back_inserter(transformed), ::tolower);
    if (transformed.compare("true") != 0 && transformed.compare("false") != 0)
      return false;
    return true;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Function : stl_string_compare
   * Description : comparison function to be used on the sorting process
   * Return Value : following the STL requirements should return true if the
   *                first string is lower than the second
   */
  bool stl_string_compare(const std::string &first, const std::string &second, bool case_sensitive) {
    return string_compare(first, second, case_sensitive) < 0;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Culturally correct string comparison. Also properly compares different normalization forms.
   * For a large amount of strings this function is not very effective as it generates the sort keys
   * repeatedly (not to mention normalization).
   * So if we ever need sorting of 10000 strings we have to add a separate implementation.
   *
   * @param first, the left string to compare.
   * @param second, the right string to compare.
   * @result   0 - If the strings are equal.
   *         < 0 - If first sorts before second.
   *         > 0 - If second sorts before first.
   */
  int string_compare(const std::string &first, const std::string &second, bool case_sensitive) {
    int result = 0;

    gchar *left = g_utf8_normalize(first.c_str(), -1, G_NORMALIZE_DEFAULT);
    gchar *right = g_utf8_normalize(second.c_str(), -1, G_NORMALIZE_DEFAULT);
    if (!case_sensitive) {
      gchar *s1 = g_utf8_casefold(left, -1);
      gchar *s2 = g_utf8_casefold(right, -1);
      result = g_utf8_collate(s1, s2);
      g_free(s1);
      g_free(s2);
    } else
      result = g_utf8_collate(left, right);

    g_free(left);
    g_free(right);

    return result;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Convenience function to determine if 2 strings are the same. This works also for culturally
   * equal letters (e.g. german ß and ss) and any normalization form.
   */
  bool same_string(const std::string &first, const std::string &second, bool case_sensitive) {
    return string_compare(first, second, case_sensitive) == 0;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Determines if the given candidate is part of the given text. As with the string_compare matches
   * are culturally correct.
   */
  bool contains_string(const std::string &text, const std::string &candidate, bool case_sensitive) {
    if (text.size() == 0 || candidate.size() == 0)
      return false;

    gchar *hay_stack = g_utf8_normalize(text.c_str(), -1, G_NORMALIZE_DEFAULT);
    gchar *needle = g_utf8_normalize(candidate.c_str(), -1, G_NORMALIZE_DEFAULT);

    if (!case_sensitive) {
      gchar *temp = g_utf8_casefold(hay_stack, -1);
      g_free(hay_stack);
      hay_stack = temp;

      temp = g_utf8_casefold(needle, -1);
      g_free(needle);
      needle = temp;
    }

    gunichar start_char = g_utf8_get_char(needle);

    bool result = false;
    gchar *run = hay_stack;
    while (!result) {
      gchar *p = g_utf8_strchr(run, -1, start_char);
      if (p == NULL)
        break;

      // Found the start char in the remaining text. See if that part matches the needle.
      gchar *needle_run = needle;
      bool mismatch = false;
      for (size_t i = 0; i < candidate.size(); ++i, ++p, ++needle_run) {
        if (g_utf8_get_char(needle_run) != g_utf8_get_char(p)) {
          mismatch = true;
          break;
        }
      }
      if (mismatch)
        ++run;
      else
        result = true;
    }
    g_free(hay_stack);
    g_free(needle);

    return result;
  }

  //--------------------------------------------------------------------------------------------------

  EolHelpers::Eol_format EolHelpers::detect(const std::string &text) {
    std::string::size_type pos = text.find_first_of("\r\n");
    if (std::string::npos == pos)
      return default_eol_format();
    if ('\r' == text[pos])
      return ('\n' == text[pos + 1]) ? eol_crlf : eol_cr;
    else
      return eol_lf;
  }

  int EolHelpers::count_lines(const std::string &text) {
    Eol_format eol_format = detect(text);
    char eol_sym = (eol_cr == eol_format) ? '\r' : '\n';
    return (int)std::count(text.begin(), text.end(), eol_sym);
  }

  bool EolHelpers::check(const std::string &text) {
    std::string::size_type pos = text.find_first_of("\n\r");
    if (std::string::npos == pos)
      return true;
    Eol_format eol_format = detect(text);
    if (eol_lf == eol_format) {
      if (text.find("\r") != std::string::npos)
        return false;
    } else if (eol_cr == eol_format) {
      if (text.find("\n") != std::string::npos)
        return false;
    } else if (eol_crlf == eol_format) {
      do {
        if (('\n' == text[pos]) || ('\n' != text[pos + 1]))
          return false;
        ++pos;
        ++pos;
        pos = text.find_first_of("\n\r", pos);
      } while (std::string::npos != pos);
    }
    return true;
  }

  void EolHelpers::conv(const std::string &src_text, Eol_format src_eol_format, std::string &dest_text,
                        Eol_format dest_eol_format) {
    if (src_eol_format == dest_eol_format)
      throw std::logic_error("source and target line ending formats coincide, no need to convert");

    const std::string &src_eol = eol(src_eol_format);
    const std::string &dest_eol = eol(dest_eol_format);
    std::string::size_type src_eol_length = src_eol.size();

    if (dest_eol.size() != src_eol.size()) {
      dest_text.clear();
      int line_count = count_lines(src_text);
      size_t dest_size = src_text.size() + line_count * (dest_eol.size() - src_eol.size());
      dest_text.reserve(dest_size);
      std::string::size_type prev_pos = 0;
      std::string::size_type pos = 0;
      while ((pos = src_text.find(src_eol, pos)) != std::string::npos) {
        dest_text.append(src_text, prev_pos, pos - prev_pos).append(dest_eol);
        pos += src_eol_length;
        prev_pos = pos;
      }
      dest_text.append(src_text, prev_pos, std::string::npos);
    } else {
      dest_text = src_text;
      std::string::size_type pos = 0;
      while ((pos = dest_text.find(src_eol, pos)) != std::string::npos) {
        dest_text.replace(pos, src_eol_length, dest_eol);
        pos += src_eol_length;
      }
    }
  }

  void EolHelpers::fix(const std::string &src_text, std::string &dest_text, Eol_format eol_format) {
    const std::string &dest_eol = eol(eol_format);
    std::string::size_type dest_eol_length = dest_eol.size();

    dest_text.clear();
    if (eol_crlf == eol_format) {
      int cr_count = (int)std::count(src_text.begin(), src_text.end(), '\r');
      int lf_count = (int)std::count(src_text.begin(), src_text.end(), '\n');
      int crlf_count = 0;
      {
        std::string::size_type pos = 0;
        while ((pos = src_text.find(dest_eol, pos)) != std::string::npos) {
          ++crlf_count;
          pos += dest_eol_length;
        }
      }
      size_t dest_size = src_text.size() + (cr_count - crlf_count) + (lf_count - crlf_count);
      dest_text.reserve(dest_size);
    }

    std::string::size_type prev_pos = 0;
    std::string::size_type pos = 0;
    std::string crlf = "\r\n";
    while ((pos = src_text.find_first_of(crlf, pos)) != std::string::npos) {
      dest_text.append(src_text, prev_pos, pos - prev_pos).append(dest_eol);
      if (('\r' == src_text[pos]) && ('\n' == src_text[pos + 1]))
        ++pos;
      ++pos;
      prev_pos = pos;
    }
    dest_text.append(src_text, prev_pos, std::string::npos);
  }

  //--------------------------------------------------------------------------------------------------

  std::string reflow_text(const std::string &text, unsigned int line_length, const std::string &left_fill,
                          bool indent_first, unsigned int max_lines) {
    bool use_fill = true;
    const unsigned int minimum_text_length = 5;

    //  Check if the line length complies to the minimum required
    if (line_length < minimum_text_length)
      return "";

    //  Only use left_fill when it's small enough to fit in the line and make the function able
    //  to do what it has to do
    const unsigned int left_fill_length = (unsigned)left_fill.size();

    if (left_fill_length + minimum_text_length >= line_length)
      use_fill = false;

    //  Check for empty string...if we let it go, a left_fill will be inserted
    if (text.size() == 0)
      return "";

    //  Check if it's a valid utf8 string
    const char *invalid_data_ptr = NULL;

    if (g_utf8_validate(text.c_str(), (gsize)text.size(), &invalid_data_ptr) != TRUE)
      throw std::invalid_argument(std::string("base::reflow_text received an invalid string: ") + text);

    const std::string initial = (indent_first && use_fill) ? left_fill : "";
    const std::string new_line = use_fill ? std::string("\n") + left_fill : std::string("\n");
    std::string result = initial;

    const char *char_string = text.c_str();
    const char *iter = char_string;

    unsigned int space_position_source = 0;
    unsigned int line_char_counter = 0;
    unsigned int line_counter = 0;
    unsigned int char_count_after_space = 0;
    unsigned int text_real_length = use_fill ? line_length - left_fill_length : line_length;

    while (*iter) {
      //  Get the full utf8 char into the result string
      result += std::string(iter, g_utf8_skip[*(const guchar *)(iter)]);

      line_char_counter++;
      char_count_after_space++;

      if (g_unichar_isspace(*iter) && line_char_counter > left_fill_length) {
        space_position_source = (unsigned)(iter - char_string + 1);
        char_count_after_space = 0;
      }

      if (line_char_counter == text_real_length) {
        //  Check for special case when we have a word as big as a line
        if (char_count_after_space == text_real_length) {
          result += new_line;

          space_position_source += char_count_after_space;
          line_char_counter = char_count_after_space = 0;
        } else {
          //  Find last space character position in the result string
          unsigned int break_position =
            space_position_source + line_counter * (unsigned)new_line.size() + (unsigned)initial.size();

          //  Insert a \n in the right position, right after the space char(or at the end of the string)
          result.size() == break_position ? result += new_line : result.insert(break_position, new_line);

          //  Mark the characters that were already inserted after the new line
          line_char_counter = char_count_after_space;
        }

        if (++line_counter == max_lines) {
          result.resize(result.size() - char_count_after_space - new_line.size());
          result += "\n(...)";
          break;
        }
      }

      iter = g_utf8_next_char((gchar *)iter); //  Get the next char from the sequence
    }

#ifdef DEBUG
    if (g_utf8_validate(result.c_str(), result.size(), &invalid_data_ptr) != TRUE)
      throw std::logic_error(
        strfmt("base::reflow_text produced an invalid string:\nInput:\n%s\nOutput:\n%s", text.c_str(), result.c_str()));
#endif

    return result;
  }

} // namespace base
