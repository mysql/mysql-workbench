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

#pragma once

#include "common.h"

#define SPACES " \t\r\n"

namespace mga {
  enum class NormalizationForm {
    NFC,
    NFD,
    NFKC,
    NFKD
  };

  // Some general purpose code.
  // Note: strings are considered to use the UTF-8 encoding, unless otherwise specified.
  class Utilities {
  public:
    static std::string formatTime(std::chrono::milliseconds time);

    static std::string toLower(std::string const& s);

    static bool hasPrefix(std::string const& s, std::string const& part);
    static bool hasSuffix(std::string const& s, std::string const& part);

    static std::vector<std::string> splitBySet(std::string const& text, std::string const& separatorSet = "\n",
                                          int count = -1);
    static std::vector<std::string> split(std::string const& text, std::string const& separator = "\n",
                                          int count = -1);
    static std::string concat(std::vector<std::string> const& values, std::string const& separator = "\n");
    static void replaceStringInplace(std::string &value, std::string const& search, std::string const& replacement);

    template <typename T>
    static void appendVector(std::vector<std::unique_ptr<T>> &target, std::vector<std::unique_ptr<T>> &source) {
      for (auto &element : source)
        target.emplace_back(std::move(element));
    }

    // string <-> wstring conversion (UTF-16), e.g. for use with Window's wide APIs.
    static std::string ws2s(std::wstring const& wstr);
    static std::wstring s2ws(std::string const& str);

    static std::string getLastError();
    
    static std::string trimRight(std::string s, const std::string &t = SPACES);
    static std::string trimLeft(std::string s, const std::string &t = SPACES);
    static std::string trim(std::string s, const std::string &t = SPACES);

    static std::string format(const char* format, ...);
    static std::string normalize(std::string const& text, NormalizationForm form);

    static std::string readFile(std::string const& fileName);
    static void writeFile(std::string const& fileName, std::string const& content);

    static std::string escapeJSONString(std::string const& s);
  };

} // namespace mga

