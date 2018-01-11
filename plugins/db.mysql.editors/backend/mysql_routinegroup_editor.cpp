/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "mysql_routinegroup_editor.h"

#include "base/string_utilities.h"

#include "mforms/code_editor.h"

using namespace bec;

MySQLRoutineGroupEditorBE::MySQLRoutineGroupEditorBE(const db_mysql_RoutineGroupRef& group)
  : bec::RoutineGroupEditorBE(group) {
  _routine_group = group;
  if (!is_editing_live_object())
    scoped_connect(get_sql_editor()->get_editor_control()->signal_lost_focus(),
                   std::bind(&MySQLRoutineGroupEditorBE::commit_changes, this));
}

//--------------------------------------------------------------------------------------------------

/**
 * Loads the current routines sql text into the editor control and marks that as not dirty.
 */
void MySQLRoutineGroupEditorBE::load_routines_sql() {
  mforms::CodeEditor* editor = get_sql_editor()->get_editor_control();
  std::string sql = get_sql();
  editor->set_text_keeping_state(sql.c_str());
  editor->reset_dirty();
}

//--------------------------------------------------------------------------------------------------

void MySQLRoutineGroupEditorBE::commit_changes() {
  mforms::CodeEditor* editor = get_sql_editor()->get_editor_control();
  if (editor->is_dirty()) {
    const std::string sql = editor->get_text(false);
    if (sql != get_sql()) {
      AutoUndoEdit undo(this, _routine_group, "sql");

      freeze_refresh_on_object_change();
      _parserServices->parseRoutines(_parserContext, _routine_group, sql);
      thaw_refresh_on_object_change();

      undo.end(base::strfmt(_("Edit routine group `%s` of `%s`.`%s`"), _routine_group->name().c_str(),
                            get_schema_name().c_str(), get_name().c_str()));
    }
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * This is a special function for unit/integration testing only. Don't use it for normal
 * processing. It pretends there was new sql set in the associated editor and parses this text
 * now into the routine group being edited by this editor.
 */
void MySQLRoutineGroupEditorBE::use_sql(const std::string& sql) {
  AutoUndoEdit undo(this, _routine_group, "sql");

  freeze_refresh_on_object_change();
  _parserServices->parseRoutines(_parserContext, _routine_group, sql);
  thaw_refresh_on_object_change();

  undo.end(base::strfmt(_("Edit routine group `%s` of `%s`.`%s`"), _routine_group->name().c_str(),
                        get_schema_name().c_str(), get_name().c_str()));
}

//--------------------------------------------------------------------------------------------------
