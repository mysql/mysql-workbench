/* 
 * Copyright (c) 2009, 2013, Oracle and/or its affiliates. All rights reserved.
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

#include "stdafx.h"

#include "grt/grt_dispatcher.h"
#include "mysql_routinegroup_editor.h"
#include "base/string_utilities.h"

#include "mforms/code_editor.h"

using namespace base;

MySQLRoutineGroupEditorBE::MySQLRoutineGroupEditorBE(bec::GRTManager *grtm, const db_mysql_RoutineGroupRef &group, const db_mgmt_RdbmsRef &rdbms)
  : bec::RoutineGroupEditorBE(grtm, group, rdbms)
{
  if (!is_editing_live_object())
    scoped_connect(get_sql_editor()->get_editor_control()->signal_lost_focus(),boost::bind(&MySQLRoutineGroupEditorBE::commit_changes, this));
}

//--------------------------------------------------------------------------------------------------

std::string MySQLRoutineGroupEditorBE::get_procedure_body()
{
  return strfmt("CREATE PROCEDURE `%s`.`proc`()\nBEGIN\n  \nEND %s\n\n",
    get_schema()->name().c_str(), _non_std_sql_delimiter.c_str());
}

//--------------------------------------------------------------------------------------------------

std::string MySQLRoutineGroupEditorBE::get_function_body()
{
  return strfmt("CREATE FUNCTION `%s`.`func`() RETURNS INT\nBEGIN\n  \nEND %s\n\n",
    get_schema()->name().c_str(), _non_std_sql_delimiter.c_str());
}

//--------------------------------------------------------------------------------------------------

/**
 * Loads the current view sql text into the editor control and marks that as not dirty.
 */
void MySQLRoutineGroupEditorBE::load_routines_sql()
{
  mforms::CodeEditor* editor = get_sql_editor()->get_editor_control();
  std::string sql = get_routines_sql();
  editor->set_text_keeping_state(sql.c_str());
  editor->reset_dirty();
}

//--------------------------------------------------------------------------------------------------

void MySQLRoutineGroupEditorBE::commit_changes()
{
  mforms::CodeEditor* editor = get_sql_editor()->get_editor_control();
  if (editor->is_dirty())
  {
    const std::string sql = editor->get_text(false);
    set_routines_sql(sql, true);
    editor->reset_dirty();
  }
}

//--------------------------------------------------------------------------------------------------

