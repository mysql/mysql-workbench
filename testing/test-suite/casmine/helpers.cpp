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

#include <random>
#include <algorithm>
#include <codecvt> // TODO: deprecated in C++17, replace with ICU.

#ifndef _MSC_VER
#include <wordexp.h>
#endif

#include "helpers.h"

namespace casmine {

//----------------------------------------------------------------------------------------------------------------------

/**
  * Splits a string into parts based on a set of separators.
  */
std::vector<std::string> splitBySet(std::string s, std::string const& separators) {
  std::vector<std::string> parts;
  std::string::size_type p;

  if (s.empty())
    return parts;

  p = s.find_first_of(separators);
  while (!s.empty() && p != std::string::npos) {
    parts.push_back(s.substr(0, p));
    s = s.substr(p + 1);
    p = s.find_first_of(separators);
  }
  parts.push_back(s);

  return parts;
}

//----------------------------------------------------------------------------------------------------------------------

#ifdef _MSC_VER

static thread_local std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> utf16Converter;
static thread_local std::wstring_convert<std::codecvt_utf8<__int32>, __int32> utf32Converter;

std::string utf32ToUtf8(std::u32string const& text) {
  return utf32Converter.to_bytes(std::basic_string<__int32>(text.begin(), text.end()));
}

//----------------------------------------------------------------------------------------------------------------------

std::string utf16ToUtf8(std::u16string const& text) {
  return utf16Converter.to_bytes(std::wstring(text.begin(), text.end()));
}

//----------------------------------------------------------------------------------------------------------------------

std::u16string utf8ToUtf16(std::string const& s) {
  auto result = utf16Converter.from_bytes(s);
  return std::u16string(result.begin(), result.end());
}

#else

static thread_local std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> utf16Converter;
static thread_local std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> utf32Converter;

std::string utf32ToUtf8(std::u32string const& text) {
  return utf32Converter.to_bytes(text);
}

//----------------------------------------------------------------------------------------------------------------------

std::string utf16ToUtf8(std::u16string const& text) {
  return utf16Converter.to_bytes(text);
}

//----------------------------------------------------------------------------------------------------------------------

std::u16string utf8ToUtf16(std::string const& s) {
  return utf16Converter.from_bytes(s);
}

#endif

//----------------------------------------------------------------------------------------------------------------------

std::string randomString(std::size_t maxLength) {
  std::string str("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890");
  std::random_device r;
  std::ranlux24 gen(r());
  std::shuffle(str.begin(), str.end(), gen);
  return str.substr(0, maxLength);
}

//----------------------------------------------------------------------------------------------------------------------

std::string getEnvVar(std::string const& name, std::string const& defaultValue) {
  const char *value = getenv(name.c_str());
  if (value == nullptr)
    return defaultValue;
  return value;
}

//----------------------------------------------------------------------------------------------------------------------

/**
  * Resolve the given path by replacing environment variables and special chars like ~.
  */
std::string expandPath(std::string const &path) {
#ifdef _MSC_VER
  TCHAR outPath[_MAX_PATH] = { 0 };
  std::u16string temp = utf8ToUtf16(path);
  std::wstring pathUnicode(temp.begin(), temp.end());
  DWORD result = ExpandEnvironmentStrings(pathUnicode.c_str(), outPath, sizeof(outPath) / sizeof(*outPath));
  if (!result) {
    return path;
  }
  pathUnicode = outPath;
  return utf16ToUtf8(std::u16string(pathUnicode.begin(), pathUnicode.end()));
#else
  if (path.empty())
    return path;

  std::string result;
  wordexp_t p;
  wordexp(path.c_str(), &p, 0);
  if (p.we_wordv[0] != nullptr) {
    result = p.we_wordv[0];
  }
  wordfree(&p);

  return result;
#endif
}

//----------------------------------------------------------------------------------------------------------------------

/**
  * Simple string comparison, based on binary code points and hence not culturally correct.
  * TODO: use ICU for this.
  */
static bool sameString(std::string const& lhs, std::string const& rhs, bool caseSensitive) {
  if (!caseSensitive) {
    std::string lhsTransformed;
    std::string rhsTransformed;
    std::transform(lhs.begin(), lhs.end(), std::back_inserter(lhsTransformed), ::tolower);
    std::transform(rhs.begin(), rhs.end(), std::back_inserter(rhsTransformed), ::tolower);
    return lhsTransformed.compare(rhsTransformed) == 0;
  } else {
    return lhs.compare(rhs) == 0;
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
  * Returns the second path relative to the given base path, provided both have a common ancestor.
  * If not then the second path is return unchanged.
  * Paths can contain both forward and backward slash separators. The result only uses backslashes.
  * Folder names are compared case insensitively on Windows, otherwise case matters.
  */
std::string relativePath(std::string const& basePath, std::string const& pathToMakeRelative) {
  std::vector<std::string> basePathList = splitBySet(basePath, "/\\");
  std::vector<std::string> otherPathList = splitBySet(pathToMakeRelative, "/\\");

  size_t totalDepth = std::min(basePathList.size(), otherPathList.size());
  size_t commonDepth = 0;
  for (size_t i = 0; i < totalDepth; ++i, ++commonDepth) {
#ifdef _MSC_VER
    if (!sameString(basePathList[i], otherPathList[i], false))
#else
    if (!sameString(basePathList[i], otherPathList[i], true))
#endif
      break;
  }

  if (commonDepth == 0)
    return pathToMakeRelative;

  std::string result;
  for (size_t i = 0; i < basePathList.size() - commonDepth; ++i)
    result += "../";

  for (size_t i = commonDepth; i < otherPathList.size(); ++i) {
    result += otherPathList[i];
    if (i < otherPathList.size() - 1)
      result += "/";
  }

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

}
