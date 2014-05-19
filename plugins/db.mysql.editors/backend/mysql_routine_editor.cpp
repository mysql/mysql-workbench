/* 
 * Copyright (c) 2009, 2014, Oracle and/or its affiliates. All rights reserved.
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
#include "mysql_routine_editor.h"

#include "mforms/code_editor.h"

using namespace bec;

//--------------------------------------------------------------------------------------------------

static void commit_changes_to_be(MySQLRoutineEditorBE *be) {
  be->commit_changes();
}

//--------------------------------------------------------------------------------------------------

MySQLRoutineEditorBE::MySQLRoutineEditorBE(bec::GRTManager *grtm, const db_mysql_RoutineRef &routine, const db_mgmt_RdbmsRef &rdbms)
  : RoutineEditorBE(grtm, routine, rdbms)
{
  _routine = routine;
  
  // In modeling we apply the text on focus change. For live editing however we don't.
  // The user has to explicitly commit his changes.
  if (!is_editing_live_object())
    scoped_connect(get_sql_editor()->get_editor_control()->signal_lost_focus(),boost::bind(&commit_changes_to_be,this));
}

//--------------------------------------------------------------------------------------------------

/**
 * Loads the current routine sql text into the editor control and marks that as not dirty.
 */
void MySQLRoutineEditorBE::load_routine_sql()
{
  mforms::CodeEditor* editor = get_sql_editor()->get_editor_control();
  std::string sql = get_sql();
  editor->set_text_keeping_state(sql.c_str());
}

//--------------------------------------------------------------------------------------------------

void MySQLRoutineEditorBE::commit_changes()
{
  mforms::CodeEditor* editor = get_sql_editor()->get_editor_control();
  if (editor->is_dirty())
  {
    const std::string sql = editor->get_text(false);
    if (sql != get_sql())
    {
      AutoUndoEdit undo(this, _routine, "sql");

      freeze_refresh_on_object_change();
      _parser_services->parseRoutine(_parser_context, _routine, sql);
      thaw_refresh_on_object_change();

      undo.end(base::strfmt(_("Edit routine `%s` of `%s`.`%s`"), _routine->name().c_str(), get_schema_name().c_str(), get_name().c_str()));
    }
  }
}

//--------------------------------------------------------------------------------------------------

bool MySQLRoutineEditorBE::can_close()
{
  commit_changes();
  return RoutineEditorBE::can_close();
}

//--------------------------------------------------------------------------------------------------
