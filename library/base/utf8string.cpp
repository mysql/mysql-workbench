/*
 * Copyright (c) 2015, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "base/utf8string.h"
#include "base/string_utilities.h"
#include <cstdlib>
#include <algorithm>
#include <boost/locale/encoding_utf.hpp>
#include <glib.h>
#include <cstring>
#include <functional>
#include <cctype>
#include <memory>

using boost::locale::conv::utf_to_utf;

namespace base {
  /*
  static utf8string::size_type utf8_byte_offset(const char* str, utf8string::size_type offset)
  {
    if(offset == utf8string::npos)
      return utf8string::npos;

    const char *const utf8_skip = g_utf8_skip;
    const char* p = str;

    for(; offset != 0; --offset)
    {
      const unsigned int c = static_cast<unsigned char>(*p);

      if(c == 0)
        return utf8string::npos;

      p += utf8_skip[c];
    }

    return (p - str);
  }
  */
  // Second overload: stop when reaching maxlen.
  static utf8string::size_type utf8_byte_offset(const char* str, utf8string::size_type offset,
                                                utf8string::size_type maxlen) {
    if (offset == utf8string::npos)
      return utf8string::npos;

    const char* const utf8_skip = g_utf8_skip;
    const char* const pend = str + maxlen;
    const char* p = str;

    for (; offset != 0; --offset) {
      if (p >= pend)
        return utf8string::npos;

      p += utf8_skip[static_cast<unsigned char>(*p)];
    }

    return (p - str);
  }

  // Third overload: stop when reaching str.size().
  //
  inline utf8string::size_type utf8_byte_offset(const std::string& str, utf8string::size_type offset) {
    return utf8_byte_offset(str.data(), offset, str.size());
  }

  // Converts byte offset to UTF-8 character offset.
  inline utf8string::size_type utf8_char_offset(const std::string& str, utf8string::size_type offset) {
    if (offset == utf8string::npos)
      return utf8string::npos;

    const char* const pdata = str.data();
    return g_utf8_pointer_to_offset(pdata, pdata + offset);
  }

  // Helper to implement ustring::find_first_of() and find_first_not_of().
  // Returns the UTF-8 character offset, or ustring::npos if not found.
  static utf8string::size_type utf8_find_first_of(const std::string& str, utf8string::size_type offset,
                                                  const char* utf8_match, long utf8_match_size, bool find_not_of) {
    const utf8string::size_type byte_offset = utf8_byte_offset(str, offset);
    if (byte_offset == utf8string::npos)
      return utf8string::npos;

    long ucs4_match_size = 0;
    gunichar* ucs4_match = g_utf8_to_ucs4_fast(utf8_match, utf8_match_size, &ucs4_match_size);

    const gunichar* const match_begin = ucs4_match;
    const gunichar* const match_end = match_begin + ucs4_match_size;

    const char* const str_begin = str.data();
    const char* const str_end = str_begin + str.size();

    for (const char* pstr = str_begin + byte_offset; pstr < str_end; pstr = g_utf8_next_char(pstr)) {
      const gunichar* const pfound = std::find(match_begin, match_end, g_utf8_get_char(pstr));

      if ((pfound != match_end) != find_not_of)
        return offset;

      ++offset;
    }

    g_free(ucs4_match);

    return utf8string::npos;
  }

  //////////////////////////////////////////////////////////////////////////////
  //  utf8string::bounds class
  //////////////////////////////////////////////////////////////////////////////
  struct utf8string::bounds {
    utf8string::size_type _index;
    utf8string::size_type _count;

    bounds(const std::string& str, utf8string::size_type index, utf8string::size_type count)
      : _index(utf8_byte_offset(str, index)), _count(utf8string::npos) {
      if (_index == utf8string::npos) {
        _index = str.size();
        _count = 0;
      } else
        _count = utf8_byte_offset(str.data() + _index, count, str.size() - _index);
    }

    utf8string::size_type index() const {
      return _index;
    }
    utf8string::size_type count() const {
      return _count;
    }
  };

  //////////////////////////////////////////////////////////////////////////////
  //  utf8string::utf8char class
  //////////////////////////////////////////////////////////////////////////////
  utf8string::utf8char::utf8char(uint32_t c) : _char(c) {
    std::memset(_buffer, 0, sizeof _buffer);
    _length = g_unichar_to_utf8(c, _buffer);
  }

  utf8string::utf8char::utf8char(const utf8char& c) : _char(c._char) {
    strncpy(_buffer, c._buffer, sizeof(_buffer));
    _length = std::strlen(_buffer);
  }

  utf8string::utf8char::utf8char(const char* c) : _char(g_utf8_get_char(c)) {
    std::memset(_buffer, 0, sizeof _buffer);
    _length = g_unichar_to_utf8(_char, _buffer);
  }

  bool utf8string::utf8char::operator==(const utf8char& c) const {
    return _char == c._char;
  }

  bool utf8string::utf8char::operator==(char c) const {
    return _char == (uint32_t)c;
  }

  bool utf8string::utf8char::operator==(uint32_t c) const {
    return _char == c;
  }

  bool utf8string::utf8char::operator==(const char* c) const {
    return _char == g_utf8_get_char(c);
  }

  bool utf8string::utf8char::operator!=(const utf8char& c) const {
    return _char != c._char;
  }

  bool utf8string::utf8char::operator!=(char c) const {
    return _char != (uint32_t)c;
  }

  bool utf8string::utf8char::operator!=(uint32_t c) const {
    return _char != c;
  }

  bool utf8string::utf8char::operator!=(const char* c) const {
    return _char != g_utf8_get_char(c);
  }

  utf8string::utf8char::operator uint32_t() const {
    return _char;
  }

  utf8string::utf8char::operator const char*() const {
    return _buffer;
  }

  size_t utf8string::utf8char::length() const {
    return _length;
  }

  //////////////////////////////////////////////////////////////////////////////
  //  utf8string class
  //////////////////////////////////////////////////////////////////////////////

  utf8string::utf8string() {
  }

  utf8string::utf8string(const char* s) : _inner_string(s) {
  }

  utf8string::utf8string(const wchar_t* s) : _inner_string(base::wstring_to_string(s)) {
  }

  utf8string::utf8string(const std::string& s) : _inner_string(s) {
  }

  utf8string::utf8string(const std::wstring& s) : _inner_string(base::wstring_to_string(s)) {
  }

  utf8string::utf8string(const utf8string& s) : _inner_string(s.to_string()) {
  }

  utf8string::utf8string(size_t size, char c) : _inner_string(size, c) {
  }

  utf8string::utf8string(size_t size, const utf8string::utf8char& c) {
    if ((uint32_t)c < 0x80) // Optimize to the most used case
      _inner_string.assign(size, static_cast<char>(c));
    else {
      utf8char ch(c);
      _inner_string.reserve(size * ch.length());

      for (; size > 0; --size)
        _inner_string.append((const char*)ch, ch.length());
    }
  }

  utf8string::utf8string(const std::string& str, size_t pos, size_t len) {
    const bounds b(str, pos, len);
    _inner_string.assign(str, b.index(), b.count());
  }

  utf8string::utf8string(const char* s, size_t pos, size_t len) {
    const bounds b(s, pos, len);
    _inner_string.assign(s, b.index(), b.count());
  }

  utf8string::utf8string(const utf8string& str, size_t pos, size_t len) {
    const bounds b(str._inner_string, pos, len);
    _inner_string.assign(str._inner_string, b.index(), b.count());
  }

  size_t utf8string::bytes() const {
    return _inner_string.size();
  }

  std::string utf8string::to_string() const {
    return _inner_string;
  }

  std::wstring utf8string::to_wstring() const {
    return base::string_to_wstring(_inner_string);
  }

  utf8string utf8string::substr(const size_t start, size_t count) const {
    return utf8string(*this, start, count);
  }

  bool utf8string::validate() const {
    return g_utf8_validate(_inner_string.c_str(), -1, nullptr) == TRUE;
  }

  utf8string utf8string::normalize() const {
    gchar* norm = g_utf8_normalize(_inner_string.c_str(), -1, G_NORMALIZE_DEFAULT);
    utf8string result = norm;
    g_free(norm);
    return result;
  }

  utf8string utf8string::trim_right() {
    for (std::string::reverse_iterator iter = _inner_string.rbegin(); iter != _inner_string.rend(); ++iter) {
      if (std::isspace((unsigned char)*iter))
        continue;
      return std::string(_inner_string.begin(), iter.base());
    }
    return "";
  }

  utf8string utf8string::trim_left() {
    for (std::string::iterator iter = _inner_string.begin(); iter != _inner_string.end(); ++iter) {
      if (std::isspace((unsigned char)*iter))
        continue;
      return std::string(iter, _inner_string.end());
    }
    return "";
  }

  utf8string utf8string::trim() {
    return trim_left().trim_right();
  }

  int utf8string::compareNormalized(const utf8string& s) const {
    return g_utf8_collate(normalize().c_str(), s.normalize().c_str());
  }

  utf8string& utf8string::operator=(char c) {
    _inner_string = std::string(1, c);
    return *this;
  }

  bool utf8string::operator==(const utf8string& s) const {
    return 0 == compareNormalized(s);
  }

  bool utf8string::operator==(const std::string& s) const {
    return 0 == compareNormalized(s);
  }

  bool utf8string::operator==(const char* s) const {
    return 0 == compareNormalized(s);
  }

  bool utf8string::operator!=(const utf8string& s) const {
    return 0 != compareNormalized(s);
  }

  bool utf8string::operator>(const utf8string& s) const {
    return 0 < compareNormalized(s);
  }

  bool utf8string::operator<(const utf8string& s) const {
    return 0 > compareNormalized(s);
  }

  bool utf8string::operator>=(const utf8string& s) const {
    return !(*this > s);
  }

  bool utf8string::operator<=(const utf8string& s) const {
    return !(*this < s);
  }

  utf8string utf8string::to_lower() const {
    gchar* down = g_utf8_strdown(_inner_string.c_str(), _inner_string.size());
    utf8string result(down);
    g_free(down);
    return result;
  }

  utf8string utf8string::to_upper() const {
    gchar* up = g_utf8_strup(_inner_string.c_str(), _inner_string.size());
    utf8string result(up);
    g_free(up);
    return result;
  }

  utf8string utf8string::to_case_fold() const {
    gchar* casefold = g_utf8_casefold(_inner_string.c_str(), _inner_string.size());
    utf8string result(casefold);
    g_free(casefold);
    return result;
  }

  utf8string utf8string::strfmt(const char* fmt, ...) {
    va_list args;
    char* str;
    utf8string result;

    va_start(args, fmt);
    str = g_strdup_vprintf(fmt, args);
    va_end(args);

    result = str;
    g_free(str);

    return result;
  }

  utf8string utf8string::truncate(const size_t max_length) {
    if (length() <=
        (max_length +
         3)) // Account for current length + "..."...it's not worth to truncate if the resulting string is bigger
      return *this;

    utf8string shortened = substr(0, max_length) + "...";
    return shortened;
  }

  std::vector<utf8string> utf8string::split(const utf8string& sep, int count) {
    std::vector<utf8string> parts;

    if (empty())
      return parts;

    if (count == 0)
      count = -1;

    utf8string ss = *this;
    std::string::size_type p;

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

  bool utf8string::starts_with(const utf8string& s) const {
    return 0 == compare(0, s.bytes(), s);
  }

  bool utf8string::ends_with(const utf8string& s) const {
    if (s.bytes() > bytes())
      return false;

    return compare(size() - s.size(), std::string::npos, s) == 0;
  }

  bool utf8string::contains(const utf8string& s, const bool case_sensitive) const {
    if (bytes() == 0 || s.bytes() == 0)
      return false;

    gchar* hay_stack = g_utf8_normalize(c_str(), -1, G_NORMALIZE_DEFAULT);
    gchar* needle = g_utf8_normalize(s.c_str(), -1, G_NORMALIZE_DEFAULT);

    if (!case_sensitive) {
      gchar* temp = g_utf8_casefold(hay_stack, -1);
      g_free(hay_stack);
      hay_stack = temp;

      temp = g_utf8_casefold(needle, -1);
      g_free(needle);
      needle = temp;
    }

    gunichar start_char = g_utf8_get_char(needle);

    bool result = false;
    gchar* run = hay_stack;
    while (!result) {
      gchar* p = g_utf8_strchr(run, -1, start_char);
      if (p == nullptr)
        break;

      // Found the start char in the remaining text. See if that part matches the needle.
      gchar* needle_run = needle;
      bool mismatch = false;
      for (size_t i = 0; i < s.size(); ++i, ++p, ++needle_run) {
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

  size_t utf8string::charIndexToByteOffset(const size_t index) const {
    return g_utf8_offset_to_pointer(this->c_str(), (glong)index) - this->c_str();
  }

  size_t utf8string::byteOffsetToCharIndex(const size_t offset) const {
    return g_utf8_pointer_to_offset(this->c_str(), this->c_str() + offset);
  }

  utf8string::iterator::iterator(char* s, char* p) : str(s) {
    if (nullptr == p)
      pos = s;
    else
      pos = p;
  }

  bool utf8string::iterator::operator==(iterator const& rhs) const {
    return (pos == rhs.pos);
  }

  bool utf8string::iterator::operator!=(iterator const& rhs) const {
    return !(this->pos == rhs.pos);
  }

  utf8string::iterator& utf8string::iterator::operator++() {
    pos = g_utf8_find_next_char(pos, nullptr);
    return *this;
  }

  utf8string::iterator& utf8string::iterator::operator--() {
    pos = g_utf8_find_prev_char(str, pos);
    return *this;
  }

  utf8string::utf8char utf8string::iterator::operator*() const {
    gchar* s = g_utf8_substring(pos, 0, 1);
    utf8char result(s);
    g_free(s);
    return result;
  }

  utf8string::iterator utf8string::begin() const {
    return iterator(const_cast<char*>(_inner_string.c_str()));
  }

  utf8string::iterator utf8string::end() const {
    char* s = const_cast<char*>(_inner_string.c_str());
    return iterator(s, s + _inner_string.size());
  }

  //////////////////////////////////////////////////////////////////////////////
  //  Operations
  //////////////////////////////////////////////////////////////////////////////
  utf8string& utf8string::erase(size_type index, size_type count) {
    const bounds b(_inner_string, index, count);
    _inner_string.erase(b.index(), b.count());
    return *this;
  }
  //
  //  Append
  //
  utf8string& utf8string::append(const char* s) {
    _inner_string += s;
    return *this;
  }

  utf8string& utf8string::append(size_type count, char ch) {
    _inner_string.append(count, ch);
    return *this;
  }

  utf8string& utf8string::append(size_type count, utf8string::utf8char ch) {
    _inner_string.append(utf8string(count, ch)._inner_string);
    return *this;
  }

  utf8string& utf8string::append(const utf8string& str) {
    _inner_string += str._inner_string;
    return *this;
  }

  //
  //  operator +=
  //
  utf8string& utf8string::operator+=(const utf8string& str) {
    _inner_string += str._inner_string;
    return *this;
  }

  utf8string& utf8string::operator+=(const utf8string::utf8char& c) {
    _inner_string.append(1, c);
    return *this;
  }

  utf8string& utf8string::operator+=(const char* s) {
    _inner_string.append(s);
    return *this;
  }

  //
  //  Compare
  //
  int utf8string::compare(const utf8string& s) const {
    return g_utf8_collate(_inner_string.c_str(), s._inner_string.c_str());
  }

  int utf8string::compare(const char* s) const {
    return g_utf8_collate(_inner_string.c_str(), s);
  }

  int utf8string::compare(size_type pos1, size_type count1, const utf8string& str) const {
    return utf8string(*this, pos1, count1).compare(str);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Element access
  //////////////////////////////////////////////////////////////////////////////
  const char* utf8string::c_str() const {
    return _inner_string.c_str();
  }

  const char* utf8string::data() const {
    return _inner_string.data();
  }

  base::utf8string::const_reference utf8string::at(size_type pos) const {
    const size_type byte_offset = utf8_byte_offset(_inner_string, pos);

    // Throws std::out_of_range if the index is invalid.
    return g_utf8_get_char(&_inner_string.at(byte_offset));
  }

  utf8string::const_reference utf8string::operator[](size_type pos) const {
    return g_utf8_get_char(g_utf8_offset_to_pointer(_inner_string.data(), (glong)pos));
  }

  //////////////////////////////////////////////////////////////////////////////
  //  Capacity
  //////////////////////////////////////////////////////////////////////////////
  size_t utf8string::size() const {
    const char* ptr = _inner_string.data();
    return g_utf8_pointer_to_offset(ptr, ptr + _inner_string.size());
  }

  size_t utf8string::length() const {
    const char* ptr = _inner_string.data();
    // return g_utf8_pointer_to_offset(ptr, ptr + _inner_string.size());
    return g_utf8_strlen(ptr, _inner_string.size());
  }

  void utf8string::resize(size_t n) {
    const size_type size_now = size();
    if (n < size_now)
      erase(n, npos);
    else if (n > size_now)
      append(n - size_now, 0);
  }

  void utf8string::resize(size_t n, char c) {
    const size_type size_now = size();
    if (n < size_now)
      erase(n, npos);
    else if (n > size_now)
      _inner_string.append(n - size_now, c);
  }

  bool utf8string::empty() const {
    return _inner_string.empty();
  }

  size_t utf8string::capacity() const {
    return _inner_string.capacity();
  }

  size_t utf8string::max_size() {
    return _inner_string.max_size();
  }

  //////////////////////////////////////////////////////////////////////////////
  //  Search
  //////////////////////////////////////////////////////////////////////////////
  utf8string::size_type utf8string::find(const char* s, size_type pos) const {
    return utf8_char_offset(_inner_string, _inner_string.find(s, utf8_byte_offset(_inner_string, pos)));
  }

  utf8string::size_type utf8string::find(const utf8string& s, size_type pos) const {
    return utf8_char_offset(_inner_string, _inner_string.find(s._inner_string, utf8_byte_offset(_inner_string, pos)));
  }

  utf8string::size_type utf8string::find(char ch, size_type pos) const {
    return utf8_char_offset(_inner_string, _inner_string.find(ch, utf8_byte_offset(_inner_string, pos)));
  }

  utf8string::size_type utf8string::find(const utf8string::utf8char& ch, size_type pos) const {
    return utf8_char_offset(_inner_string,
                            _inner_string.find((const char*)ch, utf8_byte_offset(_inner_string, pos), ch.length()));
  }

  utf8string::size_type utf8string::find_first_of(const utf8string& str, size_type pos) const {
    return utf8_find_first_of(_inner_string, pos, str._inner_string.data(), (long)str._inner_string.size(), false);
  }

  utf8string::size_type utf8string::find_first_not_of(const char* s, size_type pos) const {
    return utf8_find_first_of(_inner_string, pos, s, -1, true);
  }

  //////////////////////////////////////////////////////////////////////////////
  //  New functionality
  //////////////////////////////////////////////////////////////////////////////
  utf8string utf8string::left(size_t s) {
    if (s >= length())
      return *this;
    return substr(0, s);
  }

  utf8string utf8string::right(size_t s) {
    if (s >= length())
      return *this;
    return substr(length() - s);
  }

  //////////////////////////////////////////////////////////////////////////////
  //  New operators (external from class)
  //////////////////////////////////////////////////////////////////////////////
  std::ostream& operator<<(std::ostream& o, const utf8string& str) {
    o << str.data();
    return o;
  }

  std::ostream& operator<<(std::ostream& o, const utf8string::utf8char& c) {
    o << (const char*)c;
    return o;
  }

} // namespace base

base::utf8string operator+(const base::utf8string& str1, const base::utf8string& str2) {
  base::utf8string result = str1;
  return result.append(str2);
}

base::utf8string operator+(const base::utf8string& str, const char* s) {
  base::utf8string result = str;
  return result.append(s);
}

base::utf8string operator+(const base::utf8string& str, const char c) {
  base::utf8string result = str;
  return result.append(1, c);
}
