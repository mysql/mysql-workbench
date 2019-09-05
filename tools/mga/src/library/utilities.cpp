/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "utilities.h"

#include "utf8proc.h"

using namespace mga;

//----------------------------------------------------------------------------------------------------------------------

template <typename Container, typename Fun>
void runForEach(const Container& c, Fun fun) {
  for (auto& e : c)
    fun(std::get<0>(e), std::get<1>(e), std::get<2>(e));
}

//----------------------------------------------------------------------------------------------------------------------

/** 
 * Formats a milliseconds time span into a string with the format HH:MM:SS,mm.
 * Part of the code taken from cppcodereviewers.com.
 */
std::string Utilities::formatTime(std::chrono::milliseconds time) {
  using namespace std::chrono;

  using T = std::tuple<milliseconds, int, const char *>;

  constexpr T formats[] = {
    T{ hours(1), 2, "" },
    T{ minutes(1), 2, ":" },
    T{ seconds(1), 2, ":" },
    T{ milliseconds(1), 3, "." }
  };

  std::ostringstream o;
  runForEach(formats, [&time, &o](auto denominator, auto width, auto separator) {
    o << separator << std::setw(width) << std::setfill('0') << (time / denominator);
    time = time % denominator;
  });
  return o.str();
}

//----------------------------------------------------------------------------------------------------------------------

std::string Utilities::format(const char *format, ...) {
  char buffer[1024];

  va_list args;
  va_start(args, format);
  vsnprintf(buffer, 1024, format, args);
  std::string result = buffer;
  va_end(args);

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

std::string Utilities::normalize(std::string const& text, NormalizationForm form) {
  utf8proc_uint8_t *value;
  switch (form) {
    case NormalizationForm::NFC:
      value = utf8proc_NFC(reinterpret_cast<const uint8_t *>(text.c_str()));
      break;

    case NormalizationForm::NFD:
      value = utf8proc_NFD(reinterpret_cast<const uint8_t *>(text.c_str()));
      break;

    case NormalizationForm::NFKC:
      value = utf8proc_NFKC(reinterpret_cast<const uint8_t *>(text.c_str()));
      break;

    case NormalizationForm::NFKD:
      value = utf8proc_NFKD(reinterpret_cast<const uint8_t *>(text.c_str()));
      break;

    default:
      return text;
  }

  std::string result(reinterpret_cast<std::string::value_type *>(value));
  free(value);

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Convert string to lower case.
 * Note: this only works reliably for ANSI strings, so use it with the appropriate care.
 */
std::string Utilities::toLower(std::string const& s) {
  std::string result(s);
  std::transform(result.begin(), result.end(), result.begin(), ::tolower);
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Tests if s begins with part.
 */
bool Utilities::hasPrefix(std::string const& s, std::string const& part) {
  return s.compare(0, part.length(), part) == 0;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Tests if s ends with part.
 */
bool Utilities::hasSuffix(std::string const& s, std::string const& part) {
  if (s.length() < part.length())
    return false;

  size_t startAt = s.length() - part.length();
  return s.compare(startAt, std::string::npos, part) == 0;
}

//----------------------------------------------------------------------------------------------------------------------

std::vector<std::string> Utilities::splitBySet(std::string const& text, std::string const& separatorSet, int count) {
  std::vector<std::string> parts;

  std::string::size_type p;

  if (text.empty())
    return parts;

  if (count == 0)
    count = -1;

  std::string work = text;
  p = work.find_first_of(separatorSet);
  while (!work.empty() && p != std::string::npos && (count < 0 || count > 0)) {
    parts.push_back(work.substr(0, p));
    work = work.substr(p + 1);

    --count;
    p = work.find_first_of(separatorSet);
  }
  parts.push_back(work);

  return parts;
}

//----------------------------------------------------------------------------------------------------------------------

std::vector<std::string> Utilities::split(std::string const& text, std::string const& sep, int count) {
  std::vector<std::string> parts;
  std::string ss = text;

  std::string::size_type p;

  if (text.empty())
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

//----------------------------------------------------------------------------------------------------------------------

std::string Utilities::concat(std::vector<std::string> const& values, std::string const& separator) {
  std::string result;

  bool first = true;
  for (auto &entry : values) {
    if (entry.empty())
      continue;
    
    if (!first)
      result += separator;
    first = false;
    result += entry;
  }
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

void Utilities::replaceStringInplace(std::string &value, std::string const& search, std::string const& replacement) {
  for (size_t next = value.find(search); next != std::string::npos; next = value.find(search, next)) {
    value.replace(next, search.length(), replacement);
    next += replacement.length();
  }
}

//----------------------------------------------------------------------------------------------------------------------

std::string Utilities::ws2s(std::wstring const& wstr) {
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
  std::string narrow = converter.to_bytes(wstr);

  return narrow;
}

//----------------------------------------------------------------------------------------------------------------------

std::wstring Utilities::s2ws(const std::string &str) {
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
  std::wstring wide = converter.from_bytes(str);

  return wide;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the last error as a string, caused by a previous posix operation (chdir, stat etc.)
 */
std::string Utilities::getLastError() {
  return strerror(errno);
}

//----------------------------------------------------------------------------------------------------------------------

std::string Utilities::trimRight(std::string s, const std::string &t) {
  std::string::size_type i(s.find_last_not_of(t));
  if (i == std::string::npos)
    return "";
  else
    return s.erase(s.find_last_not_of(t) + 1);
}

//----------------------------------------------------------------------------------------------------------------------

std::string Utilities::trimLeft(std::string s, const std::string &t) {
  return s.erase(0, s.find_first_not_of(t));
}

//----------------------------------------------------------------------------------------------------------------------

std::string Utilities::trim(std::string s, const std::string &t) {
  return trimLeft(trimRight(s, t), t);
}

//----------------------------------------------------------------------------------------------------------------------

std::string Utilities::readFile(std::string const& fileName) {
#ifdef _MSC_VER
  std::ifstream stream(s2ws(fileName).c_str(), std::ios::binary);
#else
  std::ifstream stream(fileName.c_str(), std::ifstream::binary);
#endif

  if (!stream.is_open() || stream.eof())
    return "";

  std::string result;
  stream >> result;
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

void Utilities::writeFile(std::string const& fileName, std::string const& content) {
#ifdef _MSC_VER
  std::ofstream stream(s2ws(fileName).c_str(), std::ios::binary);
#else
  std::ofstream stream(fileName.c_str(), std::ifstream::binary);
#endif

  if (stream.is_open())
    stream << content;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Escape a string to be used in a JSON
 */
std::string Utilities::escapeJSONString(std::string const& s) {
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

//----------------------------------------------------------------------------------------------------------------------
