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

#include "editor_view.h"
#include "base/string_utilities.h"

using namespace grt;
using namespace bec;
using namespace base;

//--------------------------------------------------------------------------------------------------

ViewEditorBE::ViewEditorBE(const db_ViewRef &view) : DBObjectEditorBE(view) {
  MySQLEditor::Ref sql_editor = get_sql_editor();
  if (sql_editor)
    sql_editor->restrict_content_to(MySQLEditor::ContentTypeView);
}

//--------------------------------------------------------------------------------------------------

std::string ViewEditorBE::get_sql() {
  std::string sql = DBObjectEditorBE::get_sql();
  if (sql.empty())
    sql = "CREATE VIEW `" + get_name() + "` AS\n";

  return sql;
}

//--------------------------------------------------------------------------------------------------

std::string ViewEditorBE::get_title() {
  return get_name() + " - View";
}

//--------------------------------------------------------------------------------------------------
