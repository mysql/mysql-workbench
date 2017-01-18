/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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
    std::string routine_type = get_routine()->routineType();

    if (routine_type == "function")
      return "CREATE FUNCTION `" + get_name() + "` ()\nRETURNS INTEGER\nBEGIN\n\nRETURN 1;\nEND\n";

    return "CREATE PROCEDURE `" + get_name() + "` ()\nBEGIN\n\nEND\n";
  }
  return sql;
}

//--------------------------------------------------------------------------------------------------

std::string RoutineEditorBE::get_title() {
  return base::strfmt("%s - Routine", get_name().c_str());
}

//--------------------------------------------------------------------------------------------------
