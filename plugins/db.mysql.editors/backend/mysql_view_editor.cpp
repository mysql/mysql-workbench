/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "grt/tree_model.h"

#include "mysql_view_editor.h"
#include "grt/editor_base.h"

#include "mforms/code_editor.h"

using namespace bec;

//--------------------------------------------------------------------------------------------------

MySQLViewEditorBE::MySQLViewEditorBE(const db_mysql_ViewRef& view) : bec::ViewEditorBE(view) {
  // In modeling we apply the text on focus change. For live editing however we don't.
  // The user has to explicitly commit his changes.
  if (!is_editing_live_object())
    scoped_connect(get_sql_editor()->get_editor_control()->signal_lost_focus(),
                   std::bind(&MySQLViewEditorBE::commit_changes, this));
}

//--------------------------------------------------------------------------------------------------

/**
 * Loads the current view sql text into the editor control without touching its state.
 */
void MySQLViewEditorBE::load_view_sql() {
  mforms::CodeEditor* editor = get_sql_editor()->get_editor_control();
  std::string sql = get_sql();
  editor->set_text_keeping_state(sql.c_str());
}

//--------------------------------------------------------------------------------------------------

void MySQLViewEditorBE::commit_changes() {
  mforms::CodeEditor* editor = get_sql_editor()->get_editor_control();
  if (editor->is_dirty()) {
    const std::string sql = editor->get_text(false);
    if (sql != get_sql()) {
      db_mysql_ViewRef view = db_mysql_ViewRef::cast_from(get_view());
      AutoUndoEdit undo(this, view, "sql");

      freeze_refresh_on_object_change();
      _parser_services->parseView(_parser_context, view, sql);
      thaw_refresh_on_object_change();

      undo.end(base::strfmt(_("Edit view `%s` of `%s`.`%s`"), view->name().c_str(), get_schema_name().c_str(),
                            get_name().c_str()));
    }
  }
}

//--------------------------------------------------------------------------------------------------

bool MySQLViewEditorBE::can_close() {
  commit_changes();
  return bec::ViewEditorBE::can_close();
}

//--------------------------------------------------------------------------------------------------
