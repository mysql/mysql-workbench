/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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
#include <string>
#include <iterator>
#include <glib.h>
#include <vector>

namespace base
{
  /**
   * @brief Class to store UTF-8 encoded string.
   */
  class utf8string: public std::string
  {
  private:
    int compareNormalized(const utf8string &s) const;

  public:
    
  /**
   * @brief Class to store UTF-8 encoded character.
   */
    class utf8char
    {
      uint32_t _c;
    public:
      utf8char(char c) : _c(c)                          {  }
      utf8char(const utf8char &c) : _c(c._c)            {  }
      utf8char(const char *c) : _c(g_utf8_get_char(c))  {  }
      bool operator == (const utf8char &c) const        { return _c == c._c; }
      bool operator == (char c) const           { return _c == (uint32_t)c; }
      bool operator == (const char *c) const    { return _c == g_utf8_get_char(c); }
      operator unsigned int () const                     { return _c; }
    };
    
    utf8string();
    utf8string(const char *s);
    utf8string(const wchar_t *s);
    utf8string(const std::string &s);
    utf8string(const std::wstring &s);
    utf8string(const utf8string &s);
    utf8string(size_t size, char c);

    /**
     * @brief Function determinate the number of bytes in string.
     * @return Return number of bytes.
     */
    size_t bytes() const;

    /**
     * @brief Function determinate the number of characters in string, the same as size function.
     * @return Return number of characters.
     */
    size_t length() const;

    /**
     * @brief Function convert utf8string to std::string.
     * @return Return utf8string converted to std::string.
     */
    std::string toString() const;

    /**
     * @brief Function convert utf8string to std::wstring.
     * @return Return utf8string converted to std::wstring.
     */
    std::wstring toWString() const;

    /**
     * @brief Function for get sub string from utf8string.
     * @param start Start character (not byte)
     * @param start Count of characters (not bytes)
     * @return Return sub string.
     */
    utf8string substr(const size_t start, size_t count = std::string::npos) const;

    /**
     * @brief Operator to obtain single character from utf8string.
     * @param pos Position of character to get.
     * @return Return character at pos.
     */
    utf8char operator[](const size_t pos) const;

    /**
     * @brief Check if is valid UTF-8 string.
     */
    bool validate() const;
    utf8string normalize() const;
    utf8string trimRight();
    utf8string trimLeft();
    utf8string trim();
    utf8string &operator=(char c);
    bool operator==(const utf8string &s) const;
    bool operator==(const std::string &s) const;
    bool operator==(const char *s) const;
    bool operator!=(const utf8string &s) const;
    bool operator>(const utf8string &s) const;
    bool operator<(const utf8string &s) const;
    bool operator>=(const utf8string &s) const;
    bool operator<=(const utf8string &s) const;
    utf8string toLower() const;
    utf8string toUpper() const;

    /**
     * @brief Converts a string into a form that is independent of case.
     */
    utf8string toCaseFold() const;
    static utf8string strfmt(const char* fmt, ...);
    utf8string truncate(const size_t max_length);
    std::vector<utf8string> split(const utf8string &sep, int count = -1);
    bool startsWith(const utf8string& s) const;
    bool endsWith(const utf8string& s) const;
    bool contains(const utf8string& s, const bool case_sensitive = true) const;
    size_t charIndexToByteOffset(const size_t index) const;
    size_t byteOffsetToCharIndex(const size_t offset) const;

    class iterator : public std::iterator<std::bidirectional_iterator_tag, utf8string>
    {
    public:
      iterator(gchar* s, gchar* p = nullptr);
      bool operator==(iterator const& rhs) const;
      bool operator!=(iterator const& rhs) const;
      utf8string::iterator& operator++();
      utf8string::iterator& operator--();
      utf8char operator*() const;

    private:
      gchar* str;
      gchar* pos;
    };

    utf8string::iterator begin() const;
    utf8string::iterator end() const;
  };
};
