/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "parsers-common.h"
#include "mysql-scanner.h"

// Identifiers for images used in auto completion lists.
#define AC_KEYWORD_IMAGE 1
#define AC_SCHEMA_IMAGE 2
#define AC_TABLE_IMAGE 3
#define AC_ROUTINE_IMAGE 4  // For SQL stored procedures + functions.
#define AC_FUNCTION_IMAGE 5 // For MySQL library (runtime) functions.
#define AC_VIEW_IMAGE 6
#define AC_COLUMN_IMAGE 7
#define AC_OPERATOR_IMAGE 8
#define AC_ENGINE_IMAGE 9
#define AC_TRIGGER_IMAGE 10
#define AC_LOGFILE_GROUP_IMAGE 11
#define AC_USER_VAR_IMAGE 12
#define AC_SYSTEM_VAR_IMAGE 13
#define AC_TABLESPACE_IMAGE 14
#define AC_EVENT_IMAGE 15
#define AC_INDEX_IMAGE 16
#define AC_USER_IMAGE 17
#define AC_CHARSET_IMAGE 18
#define AC_COLLATION_IMAGE 19

class MySQLObjectNamesCache;

using CandidatesList = std::vector<std::pair<int, std::string>>;

PARSERS_PUBLIC_TYPE void initializeMySQLCodeCompletionIfNeeded(const std::string &grammarPath);

PARSERS_PUBLIC_TYPE std::vector<std::pair<int, std::string>> getCodeCompletionList(
  size_t caretLine, size_t caretOffset, const std::string &writtenPart, const std::string &defaultSchema,
  bool uppercaseKeywords, std::shared_ptr<MySQLScanner> scanner, const std::string &functionNames,
  MySQLObjectNamesCache *cache);
