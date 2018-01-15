/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "grt/grt_dispatcher.h"
#include "editor_routine.h"
#include "base/string_utilities.h"

using namespace bec;

//--------------------------------------------------------------------------------------------------

RoutineEditorBE::RoutineEditorBE(const db_RoutineRef &routine) : DBObjectEditorBE(routine) {
  MySQLEditor::Ref sql_editor = get_sql_editor();
  if (sql_editor)
    sql_editor->restrict_content_to(MySQLEditor::ContentTypeRoutine);
}

//--------------------------------------------------------------------------------------------------

std::string RoutineEditorBE::get_sql() {
  std::string sql = DBObjectEditorBE::get_sql();
  if (sql.empty()) {
    std::string routineType = get_routine()->routineType();

    if (routineType == "function")
      return "CREATE FUNCTION `" + get_name() + "` ()\nRETURNS INTEGER\nBEGIN\n\nRETURN 1;\nEND\n";

    if (routineType == "udf")
      return "CREATE FUNCTION `" + get_name() + "` ()\nRETURNS INTEGER SONAME \"soname\"\n";

    return "CREATE PROCEDURE `" + get_name() + "` ()\nBEGIN\n\nEND\n";
  }
  return sql;
}

//--------------------------------------------------------------------------------------------------

std::string RoutineEditorBE::get_title() {
  return base::strfmt("%s - Routine", get_name().c_str());
}

//--------------------------------------------------------------------------------------------------
