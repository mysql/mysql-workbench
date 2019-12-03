/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

using namespace std::string_literals;

namespace casmine {

//----------------------------------------------------------------------------------------------------------------------

std::vector<std::string> splitBySet(std::string s, std::string const& separators);

std::string utf32ToUtf8(std::u32string const& text);
std::string utf16ToUtf8(std::u16string const& text);
std::u16string utf8ToUtf16(std::string const& text);

std::string randomString(std::size_t maxLength = 16);
std::string getEnvVar(std::string const& name, std::string const& defaultValue = "");

std::string expandPath(std::string const& path);
std::string relativePath(std::string const& basePath, std::string const& pathToMakeRelative);

//----------------------------------------------------------------------------------------------------------------------

// Compile time string literal length computation.
static int constexpr length(char const* str) { return *str ? 1 + length(str + 1) : 0; }

// Converts the passed type into a human readable (unmangled) string.
template <class T>
std::string typeToString() {
#ifdef __clang__
  std::string_view p = __PRETTY_FUNCTION__;
  return std::string(p.data() + p.find('[') + 5, p.data() + p.rfind(']'));
#elif defined(__GNUC__)
  std::string_view p = __PRETTY_FUNCTION__;
  #if __cplusplus < 201402
    return std::string(p.data() + 36, p.size() - 37);
  #else
    return std::string(p.data() + 46, p.find(';', 47) - 46);
  #endif
#elif defined(_MSC_VER)
  std::string_view p = __FUNCSIG__;
  auto start = p.rfind(":") + 1;
  return std::string(p.data() + start, p.rfind(">") - start);
#endif
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Converts values of simple types to a string. Not meant to be extended to class/struct types.
 */
template<typename Type>
std::string toString(Type const& value) {
  if constexpr (std::is_same<std::string, Type>::value) {
    return "\"" + value + "\"";
  } else if constexpr (std::is_same<Type, nullptr_t>::value) {
    return "null";
  } else if constexpr (std::is_same<bool, Type>::value) {
    return value ? "true" : "false";
  } else if constexpr (std::is_arithmetic<Type>::value) {
    if (std::isinf(static_cast<double>(value))) {
      return "INF";
    }
    if (std::isnan(static_cast<double>(value))) {
      return "NaN";
    }
    return std::to_string(value);
  } else if constexpr (std::is_convertible<Type, std::string>::value) {
    return "\"" + std::string(value) + "\"";
  } else if constexpr (std::is_convertible<Type, std::u16string>::value) {
    auto v = utf16ToUtf8(value);
    return "\"" + v + "\"";
  } else if constexpr (std::is_convertible<Type, std::u32string>::value) {
    auto v = utf32ToUtf8(value);
    return "\"" + v + "\"";
  } else if constexpr (std::is_pointer<Type>::value) {
    std::stringstream ss;
    ss << std::hex << value;
    return ss.str();
  }

  return typeToString<Type>();
}

//----------------------------------------------------------------------------------------------------------------------

template<typename T>
std::string containerToString(T const& container) {
  std::stringstream ss;
  ss << "{ ";
  for (auto &element : container) {
    ss << toString(element) << ", ";
  }
  if (!container.empty())
    ss.seekp(-2, std::ios_base::end); // Undo the last comma + space.

  ss << " }";
  return ss.str();
}

//----------------------------------------------------------------------------------------------------------------------

template<typename Key, typename Value>
std::string containerToString(std::map<Key, Value> const& container) {
  std::stringstream ss;
  ss << "{";
  for (auto &element : container) {
    ss << "{ " << toString(element.first) << ": " << toString(element.second) << " }, ";
  }

  if (!container.empty())
    ss.seekp(-2, std::ios_base::end); // Undo the last comma + space.

  ss << "}";
  return ss.str();
}

//----------------------------------------------------------------------------------------------------------------------

}
