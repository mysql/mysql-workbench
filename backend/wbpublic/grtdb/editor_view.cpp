/* 
 * Copyright (c) 2007, 2013, Oracle and/or its affiliates. All rights reserved.
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

#include "editor_view.h"
#include "base/string_utilities.h"

using namespace grt;
using namespace bec;
using namespace base;

//--------------------------------------------------------------------------------------------------

ViewEditorBE::ViewEditorBE(GRTManager *grtm, const db_ViewRef &view, const db_mgmt_RdbmsRef &rdbms)
  : DBObjectEditorBE(grtm, view, rdbms), _view(view), _has_syntax_error(true)
{
  Sql_editor::Ref sql_editor = get_sql_editor();
  if (sql_editor)
  {
    sql_editor->sql_checker()->only_object_type_of(Sql_syntax_check::ot_view);
    sql_editor->sql_checker()->context_object(_view);
  }
}

//--------------------------------------------------------------------------------------------------

std::string ViewEditorBE::get_query()
{
  std::string ret = get_view()->sqlDefinition();
  if ( ret.empty() )
  {
    ret = "CREATE VIEW `";
    // make views schema portable by default
    ret += /*get_schema_name() + "`.`" + */get_name() + "` AS\n";
  }
  
  return ret;
}

//--------------------------------------------------------------------------------------------------

void ViewEditorBE::set_query(const std::string &sql, bool sync)
{
  if (get_query() != sql)
  {
    // TODO: is it really necessary to bind it every time we set a new SQL text?
    set_sql_parser_task_cb(boost::bind(&ViewEditorBE::parse_sql, this, _1, _2));
    set_sql(sql, sync, _view);
  }
}

//--------------------------------------------------------------------------------------------------

grt::ValueRef ViewEditorBE::parse_sql(grt::GRT* grt, grt::StringRef sql)
{
  AutoUndoEdit undo(this);

  int err_count= _sql_parser->parse_view(_view, sql.c_str());
  _has_syntax_error= 0 < err_count;

  undo.end(strfmt(_("Edit view `%s`.`%s`"), get_schema_name().c_str(), get_name().c_str()));

  check_sql();

  return grt::IntegerRef(err_count);
}

//--------------------------------------------------------------------------------------------------

std::string ViewEditorBE::get_title()
{
  return base::strfmt("%s - View", get_name().c_str()); 
}

//--------------------------------------------------------------------------------------------------
