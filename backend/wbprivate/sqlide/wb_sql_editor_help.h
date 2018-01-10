/*
 * Copyright (c) 2012, 2018, Oracle and/or its affiliates. All rights reserved.
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
