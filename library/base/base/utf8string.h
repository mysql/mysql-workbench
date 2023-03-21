/*
 * Copyright (c) 2016, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include <string>
#include <iterator>
#include <glib.h>
#include <vector>
#include <cstring>
#include <cstdint>

namespace base {
  /**
   * @brief Class to store UTF-8 encoded string.
   */
  class BASELIBRARY_PUBLIC_FUNC utf8string {
  private:
    std::string _inner_string;
    int compareNormalized(const utf8string &s) const;

  public:
    class utf8char;
    typedef std::string::size_type size_type;
    typedef const utf8char const_reference;
    /*      typedef _Traits                   traits_type;
          typedef typename _Traits::char_type       value_type;
          typedef _Char_alloc_type              allocator_type;
          typedef typename _Alloc_traits::difference_type   difference_type;
          typedef typename _Alloc_traits::reference     reference;
          typedef typename _Alloc_traits::pointer       pointer;
          typedef typename _Alloc_traits::const_pointer const_pointer;
          typedef __gnu_cxx::__normal_iterator<pointer, basic_string>  iterator;
          typedef __gnu_cxx::__normal_iterator<const_pointer, basic_string>
                                const_iterator;
          typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
          typedef std::reverse_iterator<iterator>       reverse_iterator;  */

    /**
     * @brief Class to store UTF-8 encoded character.
     */
    class BASELIBRARY_PUBLIC_FUNC utf8char {
      char _buffer[7];
      size_t _length;
      uint32_t _char; //  same as gunichar
    public:
      utf8char(uint32_t c);
      utf8char(const utf8char &c);
      utf8char(const char *c);
      bool operator==(const utf8char &c) const;
      bool operator==(char c) const;
      bool operator==(uint32_t c) const;
      bool operator==(const char *c) const;
      bool operator!=(const utf8char &c) const;
      bool operator!=(char c) const;
      bool operator!=(uint32_t c) const;
      bool operator!=(const char *c) const;
      operator uint32_t() const;
      operator const char *() const;
      size_t length() const;
    };

    struct bounds;

    class BASELIBRARY_PUBLIC_FUNC iterator : public std::iterator<std::bidirectional_iterator_tag, utf8string> {
    public:
      iterator(char *s, char *p = nullptr);
      bool operator==(iterator const &rhs) const;
      bool operator!=(iterator const &rhs) const;
      utf8string::iterator &operator++();
      utf8string::iterator &operator--();
      utf8char operator*() const;

    private:
      char *str;
      char *pos;
    };

    utf8string();
    utf8string(const char *s);
    utf8string(const wchar_t *s);
    utf8string(const std::string &s);
    utf8string(const std::wstring &s);
    utf8string(const utf8string &s);
    utf8string(size_t size, char c);
    utf8string(size_t size, const utf8char &c);
    utf8string(const utf8string &str, size_t pos, size_t len);
    utf8string(const std::string &str, size_t pos, size_t len);
    utf8string(const char *s, size_t pos, size_t len);

    operator std::string() const {
      return _inner_string;
    }

    size_t charIndexToByteOffset(const size_t index) const;
    size_t byteOffsetToCharIndex(const size_t offset) const;

    //  Members
    //     basic_string& operator=( const basic_string& str );
    //     basic_string& operator=( basic_string&& str );
    //     basic_string& operator=( const CharT* s );
    //     basic_string& operator=( CharT ch );
    //     basic_string& operator=( std::initializer_list<CharT> ilist );
    //     basic_string& assign( size_type count, CharT ch );
    //     basic_string& assign( const basic_string& str );
    //     asic_string& assign( const basic_string& str, size_type pos, size_type count );
    //     basic_string& assign( basic_string&& str );
    //     basic_string& assign( const CharT* s, size_type count );
    //     basic_string& assign( const CharT* s );
    //     template< class InputIt >
    //     basic_string& assign( InputIt first, InputIt last );
    //     basic_string& assign( std::initializer_list<CharT> ilist );
    //     allocator_type get_allocator() const;

    //  Iterators
    iterator begin() const;
    iterator end() const;
    //     iterator begin();
    //     const_iterator begin() const;
    //     iterator end();
    //     const_iterator end() const;
    //     iterator rbegin();
    //     const_iterator rbegin() const;
    //     iterator rend();
    //     const_iterator rend() const;
    //     iterator cbegin();
    //     const_iterator cbegin() const;
    //     iterator cend();
    //     const_iterator cend() const;
    //     iterator crbegin();
    //     const_iterator crbegin() const;
    //     iterator crend();
    //     const_iterator crend() const;

    //  Capacity
    size_t size() const;
    void resize(size_t n);
    void resize(size_t n, char c);
    bool empty() const;
    size_t capacity() const;
    size_t max_size();
    /**
     * @brief Function determinate the number of characters in string, the same as size function.
     * @return Return number of characters.
     */
    size_t length() const;
    //     void reserve (size_t n = 0);
    //     void shrink_to_fit();

    //  Operations
    utf8string &erase(size_type index = 0, size_type count = npos);
    //     iterator erase( const_iterator position );
    //     iterator erase( const_iterator first, const_iterator last );
    //     basic_string& append( const basic_string& str );
    //     basic_string& append( const CharT* s, size_type count );
    utf8string &append(size_type count, char ch);
    utf8string &append(size_type count, utf8char ch);
    utf8string &append(const char *s);
    utf8string &append(const utf8string &str);
    utf8string &operator+=(const utf8string &str);
    utf8string &operator+=(const utf8char &c);
    utf8string &operator+=(const char *s);
    utf8string &operator=(const utf8string &other) = default;

    int compare(size_type pos1, size_type count1, const utf8string &str) const;
    int compare(const utf8string &s) const;
    int compare(const char *s) const;
    /**
     * @brief Function for get sub string from utf8string.
     * @param start Start character (not byte)
     * @param start Count of characters (not bytes)
     * @return Return sub string.
     */
    utf8string substr(const size_t start, size_t count = std::string::npos) const;
    //     void clear();
    //     basic_string& insert( size_type index, size_type count, CharT ch );
    //     basic_string& insert( size_type index, const CharT* s );
    //     basic_string& insert( size_type index, const CharT* s, size_type count );
    //     basic_string& insert( size_type index, const basic_string& str );
    //     basic_string& insert( size_type index, const basic_string& str, size_type index_str, size_type count = npos);
    //     iterator insert( const_iterator pos, CharT ch );
    //     iterator insert( const_iterator pos, size_type count, CharT ch );
    //     template< class InputIt >
    //     iterator insert( const_iterator pos, InputIt first, InputIt last );
    //     iterator insert( const_iterator pos, std::initializer_list<CharT> ilist );
    //     void push_back( CharT ch );
    //     void pop_back();
    //     template< class InputIt >
    //     basic_string& append( InputIt first, InputIt last );
    //     basic_string& append( std::initializer_list<CharT> ilist );
    //     basic_string& operator+=( const basic_string& str );
    //     basic_string& operator+=( CharT ch );
    //     basic_string& operator+=( const CharT* s );
    //     basic_string& operator+=( std::initializer_list<CharT> ilist );
    //     int compare( const basic_string& str ) const;
    //     int compare( size_type pos1, size_type count1, const basic_string& str ) const;
    //     int compare( size_type pos1, size_type count1, const basic_string& str, size_type pos2, size_type count2 )
    //     const;
    //     int compare( size_type pos1, size_type count1, const CharT* s ) const;
    //     int compare( size_type pos1, size_type count1, const CharT* s, size_type count2 ) const;
    //     basic_string& replace( size_type pos, size_type count, const basic_string& str );
    //     basic_string& replace( const_iterator first, const_iterator last, const basic_string& str );
    //     basic_string& replace( size_type pos, size_type count, const basic_string& str, size_type pos2, size_type
    //     count2 );
    //     template< class InputIt >
    //     basic_string& replace( const_iterator first, const_iterator last, InputIt first2, InputIt last2 );
    //     basic_string& replace( size_type pos, size_type count, const CharT* cstr, size_type count2 );
    //     basic_string& replace( const_iterator first, const_iterator last, const CharT* cstr, size_type count2 );
    //     basic_string& replace( size_type pos, size_type count, const CharT* cstr );
    //     basic_string& replace( const_iterator first, const_iterator last, const CharT* cstr );
    //     basic_string& replace( size_type pos, size_type count, size_type count2, CharT ch );
    //     basic_string& replace( const_iterator first, const_iterator last, size_type count2, CharT ch );
    //     basic_string& replace( const_iterator first, const_iterator last, std::initializer_list<CharT> ilist );
    //     size_type copy( CharT* dest, size_type count, size_type pos = 0) const;
    //     void resize( size_type count );
    //     void resize( size_type count, CharT ch );
    //     void swap( basic_string& other );

    //  Search
    size_type find(const char *s, size_type pos = 0) const;
    size_type find(const utf8string &s, size_type pos = 0) const;
    size_type find(char ch, size_type pos = 0) const;
    size_type find(const utf8char &ch, size_type pos = 0) const;
    size_type find_first_of(const utf8string &str, size_type pos = 0) const;
    size_type find_first_not_of(const char *s, size_type pos = 0) const;

    //     size_type find( const basic_string& str, size_type pos = 0 ) const;
    //     size_type find( const CharT* s, size_type pos, size_type count ) const;
    //     size_type rfind( const basic_string& str, size_type pos = npos ) const;
    //     size_type rfind( const CharT* s, size_type pos, size_type count ) const;
    //     size_type rfind( const CharT* s, size_type pos = npos ) const;
    //     size_type rfind( CharT ch, size_type pos = npos ) const;
    //     size_type find_first_of( const basic_string& str, size_type pos = 0 ) const;
    //     size_type find_first_of( const CharT* s, size_type pos, size_type count ) const;
    //     size_type find_first_of( const CharT* s, size_type pos = 0 ) const;
    //     size_type find_first_of( CharT ch, size_type pos = 0 ) const;
    //     size_type find_first_not_of( const basic_string& str, size_type pos = 0 ) const;
    //     size_type find_first_not_of( const CharT* s, size_type pos, size_type count ) const;
    //     size_type find_first_not_of( CharT ch, size_type pos = 0 ) const;
    //     size_type find_last_of( const basic_string& str, size_type pos = npos ) const;
    //     size_type find_last_of( const CharT* s, size_type pos, size_type count ) const;
    //     size_type find_last_of( const CharT* s, size_type pos = npos ) const;
    //     size_type find_last_of( CharT ch, size_type pos = npos ) const;
    //     size_type find_last_not_of( const basic_string& str, size_type pos = npos ) const;
    //     size_type find_last_not_of( const CharT* s, size_type pos, size_type count ) const;
    //     size_type find_last_not_of( const CharT* s, size_type pos = npos ) const;
    //     size_type find_last_not_of( CharT ch, size_type pos = npos ) const;

    // Element access
    const char *c_str() const;
    const char *data() const;
    const_reference at(size_type pos) const;
    const_reference operator[](size_type pos) const;
    //   reference       at( size_type pos );
    //   const_reference at( size_type pos ) const;
    //   reference       operator[]( size_type pos );
    //   const_reference operator[]( size_type pos ) const;
    //   CharT& front();
    //   const CharT& front() const;
    //   CharT& back();
    //   const CharT& back() const;
    //   operator std::basic_string_view<CharT, Traits>() const;

    //  Constants
    static const size_type npos = -1;

    //  New functionality
    utf8string left(size_t s);
    utf8string right(size_t s);

    /**
     * @brief Function determinate the number of bytes in string.
     * @return Return number of bytes.
     */
    size_t bytes() const;

    /**
     * @brief Function convert utf8string to std::string.
     * @return Return utf8string converted to std::string.
     */
    std::string to_string() const;

    /**
     * @brief Function convert utf8string to std::wstring.
     * @return Return utf8string converted to std::wstring.
     */
    std::wstring to_wstring() const;

    /**
     * @brief Check if is valid UTF-8 string.
     */
    bool validate() const;
    utf8string normalize() const;
    utf8string trim_right();
    utf8string trim_left();
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
    utf8string to_lower() const;
    utf8string to_upper() const;

    /**
     * @brief Converts a string into a form that is independent of case.
     */
    utf8string to_case_fold() const;
    static utf8string strfmt(const char *fmt, ...);
    utf8string truncate(const size_t max_length);
    std::vector<utf8string> split(const utf8string &sep, int count = -1);
    bool starts_with(const utf8string &s) const;
    bool ends_with(const utf8string &s) const;
    bool contains(const utf8string &s, const bool case_sensitive = true) const;
  };

  BASELIBRARY_PUBLIC_FUNC std::ostream &operator<<(std::ostream &o, const utf8string &str);
  BASELIBRARY_PUBLIC_FUNC std::ostream &operator<<(std::ostream &o, const utf8string::utf8char &c);
};

BASELIBRARY_PUBLIC_FUNC base::utf8string operator+(const base::utf8string &str1, const base::utf8string &str2);
BASELIBRARY_PUBLIC_FUNC base::utf8string operator+(const base::utf8string &str, const char *s);
BASELIBRARY_PUBLIC_FUNC base::utf8string operator+(const base::utf8string &str, const char c);
