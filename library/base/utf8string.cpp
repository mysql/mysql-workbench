/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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

#include "base/utf8string.h"
#include <cstdlib>
#include <algorithm>
#include <boost/locale/encoding_utf.hpp>
#include <glib.h>
#include <cstring>

using boost::locale::conv::utf_to_utf;

namespace base
{
  
utf8string::utf8string(): std::string()
{
  
}

utf8string::utf8string(const char* s)
: std::string(s)
{
}

utf8string::utf8string(const wchar_t* s)
: std::string(utf_to_utf<char>(s, s + wcslen(s)))
{
}

utf8string::utf8string(const std::string &s)
: std::string(s)
{
}

utf8string::utf8string(const std::wstring &s)
: std::string(utf_to_utf<char>(s.c_str(), s.c_str() + s.size()))
{
}

utf8string::utf8string(const utf8string &s)
: std::string(s)
{
}

utf8string::utf8string(size_t size, char c)
: std::string(size, c)
{
}

size_t utf8string::length() const
{
  return g_utf8_strlen(this->c_str(), std::string::size());
}

size_t utf8string::bytes() const
{
  return std::string::size();
}

std::string utf8string::toString() const
{
  return *this;
}

std::wstring utf8string::toWString() const
{
  return utf_to_utf<wchar_t>(this->c_str(), this->c_str() + std::string::size());
}

utf8string utf8string::substr(const size_t start, size_t count) const
{
  gchar* sub = nullptr;
  if( std::string::npos == count )
    sub = g_utf8_substring(this->c_str(), start, std::string::size());
  else
    sub = g_utf8_substring(this->c_str(), start, start + count);
  utf8string result(sub);
  g_free(sub);
  return result;
}

utf8string::utf8char utf8string::operator[](const size_t index) const
{
  gchar* start = g_utf8_offset_to_pointer(this->c_str(), index);
  gchar* sub = g_utf8_substring(start, 0, 1);
  
  utf8char result(sub);
  g_free(sub);
  return result;
}

bool utf8string::validate() const
{
  return g_utf8_validate(this->c_str(), -1, nullptr);
}

utf8string utf8string::normalize() const
{
  gchar* norm = g_utf8_normalize(this->c_str(), -1, G_NORMALIZE_DEFAULT);
  utf8string result = norm;
  g_free(norm);
  return result;
}

utf8string utf8string::trimRight()
{
  std::string::erase(std::find_if(std::string::rbegin(), std::string::rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), std::string::end());
  return *this;
}

utf8string utf8string::trimLeft()
{
  std::string::erase(std::string::begin(), std::find_if(std::string::begin(), std::string::end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
  return *this;
}

utf8string utf8string::trim()
{
  return trimLeft().trimRight();
}

int utf8string::compareNormalized(const utf8string &s) const
{
  return g_utf8_collate(this->normalize().c_str(), s.normalize().c_str());
}

utf8string &utf8string::operator=(char c)
{
  *this = std::string(1, c);
  return *this;
}

bool utf8string::operator==(const utf8string &s) const
{
  return 0 == this->compareNormalized(s);
}

bool utf8string::operator==(const std::string &s) const
{
  return 0 == this->compareNormalized(s);
}

bool utf8string::operator==(const char *s) const
{
  return 0 == this->compareNormalized(s);
}

bool utf8string::operator!=(const utf8string &s) const
{
  return 0 != this->compareNormalized(s);
}

bool utf8string::operator>(const utf8string &s) const
{
  return 0 < this->compareNormalized(s);
}

bool utf8string::operator<(const utf8string &s) const
{
  return 0 > this->compareNormalized(s);
}

bool utf8string::operator>=(const utf8string &s) const
{
  return !(*this > s);
}

bool utf8string::operator<=(const utf8string &s) const
{
  return !(*this < s);
}

utf8string utf8string::toLower() const
{
  gchar* down = g_utf8_strdown(this->c_str(), std::string::size());
  utf8string result(down);
  g_free(down);
  return result;
}

utf8string utf8string::toUpper() const
{
  gchar* up = g_utf8_strup(this->c_str(), std::string::size());
  utf8string result(up);
  g_free(up);
  return result;
}

utf8string utf8string::toCaseFold() const
{
  gchar* casefold = g_utf8_casefold(this->c_str(), std::string::size());
  utf8string result(casefold);
  g_free(casefold);
  return result;
}

utf8string utf8string::strfmt(const char* fmt, ...)
{
  va_list args;
  char *str;
  utf8string result;

  va_start(args, fmt);
  str = g_strdup_vprintf(fmt, args);
  va_end(args);

  result = str;
  g_free(str);

  return result;
}

utf8string utf8string::truncate(const size_t max_length)
{
  if( this->length() > max_length )
  {
    utf8string shortened = this->substr(0, max_length);
    const char *prev = g_utf8_find_prev_char(shortened.c_str(), g_utf8_offset_to_pointer(shortened.c_str(), max_length - 1));
    if (prev)
    {
      shortened.resize(prev - shortened.c_str(), 0);
      shortened.append("...");
    }
    return shortened;
  }
  return *this;
}

std::vector<utf8string> utf8string::split(const utf8string &sep, int count)
{
  std::vector<utf8string> parts;

  if (empty())
    return parts;

  if (count == 0)
    count= -1;

  utf8string ss = *this;
  std::string::size_type p;

  p = ss.find(sep);
  while (!ss.empty() && p != std::string::npos && (count < 0 || count > 0))
  {
    parts.push_back(ss.substr(0, p));
    ss= ss.substr(p+sep.size());

    --count;
    p= ss.find(sep);
  }
  parts.push_back(ss);

  return parts;
}

bool utf8string::startsWith(const utf8string& s) const
{
  return 0 == this->compare(0, s.bytes(), s);
}

bool utf8string::endsWith(const utf8string& s) const
{
  if( s.bytes() > this->bytes() )
    return false;

  return this->compare(this->bytes() - s.bytes(), std::string::npos, s) == 0;
}

bool utf8string::contains(const utf8string& s, const bool case_sensitive) const
{
  if (this->bytes() == 0 || s.bytes() == 0)
    return false;

  gchar* hay_stack = g_utf8_normalize(this->c_str(), -1, G_NORMALIZE_DEFAULT);
  gchar* needle = g_utf8_normalize(s.c_str(), -1, G_NORMALIZE_DEFAULT);

  if (!case_sensitive)
  {
    gchar *temp = g_utf8_casefold(hay_stack, -1);
    g_free(hay_stack);
    hay_stack = temp;

    temp = g_utf8_casefold(needle, -1);
    g_free(needle);
    needle = temp;
  }

  gunichar start_char = g_utf8_get_char(needle);

  bool result = false;
  gchar* run = hay_stack;
  while( !result )
  {
    gchar* p = g_utf8_strchr(run, -1, start_char);
    if( p == nullptr )
      break;

    // Found the start char in the remaining text. See if that part matches the needle.
    gchar* needle_run = needle;
    bool mismatch = false;
    for (size_t i = 0; i < s.size(); ++i, ++p, ++needle_run)
    {
      if (g_utf8_get_char(needle_run) != g_utf8_get_char(p))
      {
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

size_t utf8string::charIndexToByteOffset(const size_t index) const
{
  return g_utf8_offset_to_pointer(this->c_str(), index) - this->c_str();
}

size_t utf8string::byteOffsetToCharIndex(const size_t offset) const
{
  return g_utf8_pointer_to_offset(this->c_str(), this->c_str() + offset);
}

utf8string::iterator::iterator(gchar* s, gchar* p)
: str(s)
{
  if( nullptr == p )
    pos = s;
  else
    pos = p;
}

bool utf8string::iterator::operator==(iterator const& rhs) const
{
  return (pos == rhs.pos);
}

bool utf8string::iterator::operator!=(iterator const& rhs) const
{
  return !(this->pos == rhs.pos);
}

utf8string::iterator& utf8string::iterator::operator++()
{
  pos = g_utf8_find_next_char(pos, nullptr);
  return *this;
}

utf8string::iterator& utf8string::iterator::operator--()
{
  pos = g_utf8_find_prev_char(str, pos);
  return *this;
}

utf8string::utf8char utf8string::iterator::operator*() const
{
  gchar* s = g_utf8_substring(pos, 0, 1);
  utf8char result(s);
  g_free(s);
  return result;
}

utf8string::iterator utf8string::begin() const
{
  return iterator(const_cast<gchar*>(this->c_str()));
}

utf8string::iterator utf8string::end() const
{
  return ++iterator(const_cast<gchar*>(this->c_str()), const_cast<gchar*>(this->c_str()) + this->size());
}

} // namespace base
