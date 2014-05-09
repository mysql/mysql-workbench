/* 
 * Copyright (c) 2007, 2014, Oracle and/or its affiliates. All rights reserved.
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

RoutineEditorBE::RoutineEditorBE(GRTManager *grtm, const db_RoutineRef &routine, const db_mgmt_RdbmsRef &rdbms)
  : DBObjectEditorBE(grtm, routine, rdbms), _routine(routine), _has_syntax_error(false)
{
  if ((*_routine->sqlDefinition()).empty())
  {
    std::string template_name;
    int cursor_pos;
    std::string sql= get_sql_template(template_name, cursor_pos);
    size_t pos= sql.find_last_not_of("/\n\t ");
    if (pos != std::string::npos)
      sql= sql.substr(0, pos+1);
    _routine->sqlDefinition(sql);
  }

  MySQLEditor::Ref sql_editor= DBObjectEditorBE::get_sql_editor();
  if (sql_editor)
  {
    sql_editor->restrict_content_to(MySQLEditor::ContentTypeRoutine);
    //sql_editor->sql_checker()->context_object(_routine); TODO: do we need that reference anymore?
  }
}

//--------------------------------------------------------------------------------------------------

std::string RoutineEditorBE::get_sql()
{
  return _routine->sqlDefinition();
}

//--------------------------------------------------------------------------------------------------

std::string RoutineEditorBE::get_object_type()
{
  return "routine";
}

//--------------------------------------------------------------------------------------------------

std::string RoutineEditorBE::get_sql_definition_header()
{
  return "-- --------------------------------------------------------------------------------\n"
    "-- Routine DDL\n"
    "-- Note: comments before and after the routine body will not be stored by the server\n"
    "-- --------------------------------------------------------------------------------\n"
    + base::strfmt("DELIMITER %s\n\n", _non_std_sql_delimiter.c_str());
}

//--------------------------------------------------------------------------------------------------

std::string RoutineEditorBE::get_sql_template(const std::string &template_name, int &cursor_pos)
{
  std::string sql;
  size_t pos;
  if (template_name.empty() || template_name == "default")
  {
    std::string routine_type = _routine->routineType();

    if (routine_type == "function")
    {
      sql= base::strfmt("CREATE FUNCTION `%s` ()\nRETURNS INTEGER\nBEGIN\n\nRETURN 1;\nEND\n",
        /*get_schema_name().c_str(), */get_name().c_str());
      // better leave the schema name out to leave it portable across schemas
    }
    else
    {
      sql= base::strfmt("CREATE PROCEDURE `%s` ()\nBEGIN\n\nEND\n",
        /*get_schema_name().c_str(), */get_name().c_str());
    }
  }

  pos= sql.find("BEGIN");
  if (pos != std::string::npos)
    cursor_pos = (int)(pos + 6);

  return sql;
}

//--------------------------------------------------------------------------------------------------

std::string RoutineEditorBE::get_formatted_sql_for_editing(int &cursor_pos)
{
  std::string sql= get_sql_definition_header();
  std::string code;

  code= get_sql();

  if (code.empty())
  {
    code= get_sql_template("", cursor_pos);
    cursor_pos += (int)sql.length();
    sql.append(code);
  }
  else
  {
    sql.append(code).append("\n");
  }

  return sql;
}

//--------------------------------------------------------------------------------------------------

void RoutineEditorBE::set_sql(const std::string &sql, bool sync)
{
  if (get_sql() != sql)
  {
    set_sql_parser_task_cb(boost::bind(&RoutineEditorBE::parse_sql, this, _1, _2));
    DBObjectEditorBE::set_sql(sql, sync, _routine);
  }
}

//--------------------------------------------------------------------------------------------------

grt::ValueRef RoutineEditorBE::parse_sql(grt::GRT* grt, grt::StringRef sql)
{
  AutoUndoEdit undo(this);

  int err_count= _sql_parser->parse_routine(_routine, sql.c_str());
  _has_syntax_error= 0 < err_count;

  undo.end(base::strfmt(_("Edit routine `%s`.`%s`"), get_schema_name().c_str(), get_name().c_str()));

  check_sql();

  return grt::IntegerRef(err_count);
}

//--------------------------------------------------------------------------------------------------

std::string RoutineEditorBE::get_title()
{
  return base::strfmt("%s - Routine", get_name().c_str()); 
}

//--------------------------------------------------------------------------------------------------
