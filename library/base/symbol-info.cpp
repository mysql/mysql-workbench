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

#include "base/symbol-info.h"

#include "server/system-functions.h"

using namespace base;

#include "server/keyword_list56.h"
#include "server/keyword_list57.h"
#include "server/keyword_list80.h"
#include <map>

//----------------------------------------------------------------------------------------------------------------------

static std::set<std::string> empty;

std::set<std::string> const& MySQLSymbolInfo::systemFunctionsForVersion(MySQLVersion version) {
  switch (version) {
    case MySQLVersion::MySQL56:
      return systemFunctions56;
    case MySQLVersion::MySQL57:
      return systemFunctions57;
    case MySQLVersion::MySQL80:
      return systemFunctions80;

    default:
      return empty;
  }
}

//----------------------------------------------------------------------------------------------------------------------

static std::map<MySQLVersion, std::set<std::string>> keywords;
static std::map<MySQLVersion, std::set<std::string>> reservedKeywords;
std::set<std::string> const& MySQLSymbolInfo::keywordsForVersion(MySQLVersion version) {
  if (keywords.count(version) == 0) {
    std::set<std::string> list;
    std::set<std::string> reservedList;
    switch (version) {
      case MySQLVersion::MySQL56: {
        size_t listSize = sizeof(keyword_list56) / sizeof(keyword_list56[0]);
        for (size_t i = 0; i < listSize; ++i) {
          std::string word = keyword_list56[i].word;
          list.insert(word);
          if (keyword_list56[i].reserved != 0)
            reservedList.insert(word);
        }
        break;
      }

      case MySQLVersion::MySQL57: {
        size_t listSize = sizeof(keyword_list57) / sizeof(keyword_list57[0]);
        for (size_t i = 0; i < listSize; ++i) {
          std::string word = keyword_list57[i].word;
          list.insert(word);
          if (keyword_list57[i].reserved != 0)
            reservedList.insert(word);
        }
        break;
      }

      case MySQLVersion::MySQL80: {
        size_t listSize = sizeof(keyword_list80) / sizeof(keyword_list80[0]);
        for (size_t i = 0; i < listSize; ++i) {
          std::string word = keyword_list80[i].word;
          list.insert(word);
          if (keyword_list80[i].reserved != 0)
            reservedList.insert(word);
        }
        break;
      }

      default:
        break;
    }
    keywords[version] = list;
  }

  return keywords[version];
}

//----------------------------------------------------------------------------------------------------------------------

bool MySQLSymbolInfo::isReservedKeyword(std::string const& identifier, MySQLVersion version) {
  std::ignore = keywordsForVersion(version);
  return reservedKeywords[version].count(identifier) > 0;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * For both, reserved and non-reserved keywords.
 */
bool MySQLSymbolInfo::isKeyword(std::string const& identifier, MySQLVersion version) {
  auto keywords = keywordsForVersion(version);
  return keywords.count(identifier) > 0;
}

//----------------------------------------------------------------------------------------------------------------------

MySQLVersion MySQLSymbolInfo::numberToVersion(long version) {
  long major = version / 10000, minor = (version / 100) % 100;

  if (major < 5 || major > 8)
    return MySQLVersion::Unknown;

  if (major == 8)
    return MySQLVersion::MySQL80;

  if (major != 5)
    return MySQLVersion::Unknown;

  switch (minor) {
    case 6:
      return MySQLVersion::MySQL56;
    case 7:
      return MySQLVersion::MySQL57;
    default:
      return MySQLVersion::Unknown;
  }
}

//----------------------------------------------------------------------------------------------------------------------
