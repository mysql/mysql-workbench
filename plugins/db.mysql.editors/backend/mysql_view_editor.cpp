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

#include "stdafx.h"

#include "grt/tree_model.h"
#include "mysql_view_editor.h"

#include "mforms/code_editor.h"

//--------------------------------------------------------------------------------------------------

static void commit_changes_to_be(MySQLViewEditorBE *be)
{
  be->commit_changes();
}

//--------------------------------------------------------------------------------------------------

MySQLViewEditorBE::MySQLViewEditorBE(bec::GRTManager *grtm, const db_ViewRef &view, const db_mgmt_RdbmsRef &rdbms)
  : bec::ViewEditorBE(grtm, view, rdbms)
{
  // In modeling we apply the text on focus change. For live editing however we don't.
  // The user has to explicitly commit his changes.
  if (!is_editing_live_object())
    scoped_connect(get_sql_editor()->get_editor_control()->signal_lost_focus(),boost::bind(&commit_changes_to_be, this));
}

//--------------------------------------------------------------------------------------------------

/**
 * Loads the current view sql text into the editor control without touching its state.
 */
void MySQLViewEditorBE::load_view_sql()
{
  mforms::CodeEditor* editor = get_sql_editor()->get_editor_control();
  std::string sql = get_sql();
  if (sql.empty())
    sql = get_query();
  editor->set_text_keeping_state(sql.c_str());
}

//--------------------------------------------------------------------------------------------------

void MySQLViewEditorBE::commit_changes()
{
  mforms::CodeEditor* editor = get_sql_editor()->get_editor_control();
  if (editor->is_dirty())
  {
    const std::string sql = editor->get_text(false);
    set_query(sql, true);
  }
}

bool MySQLViewEditorBE::can_close()
{
  commit_changes();
  return bec::ViewEditorBE::can_close();
}

//--------------------------------------------------------------------------------------------------

