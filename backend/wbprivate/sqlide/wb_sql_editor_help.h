/*
 * Copyright (c) 2012, 2017, Oracle and/or its affiliates. All rights reserved.
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

// Helper class to find context sensitive help based on a statement and a position in it.
// Once a topic could be constructed the mysql help tables are used to get the help text.
// This makes it necessary however that the help tables are loaded.

#include "sqlide/wb_sql_editor_form.h"

class MySQLScanner;

class MYSQLWBBACKEND_PUBLIC_FUNC DbSqlEditorContextHelp // Made public for tests only.
{
public:
  static bool get_help_text(const SqlEditorForm::Ref &form, const std::string &topic, std::string &title,
                            std::string &text);
  static std::string find_help_topic_from_position(const SqlEditorForm::Ref &form, const std::string &query,
                                                   std::pair<ssize_t, ssize_t> caret);

protected:
  static std::string lookup_topic_for_string(const SqlEditorForm::Ref &form, std::string topic);
  static std::string topic_from_position(const SqlEditorForm::Ref &form, const std::string &query,
                                         std::pair<ssize_t, ssize_t> caret);

  static std::string topic_with_single_topic_equivalent(MySQLScanner &scanner);
};
