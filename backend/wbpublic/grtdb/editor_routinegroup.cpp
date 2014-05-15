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
#include "editor_routinegroup.h"

#include "base/string_utilities.h"

using namespace grt;
using namespace bec;
using namespace base;

RoutineGroupEditorBE::RoutineGroupEditorBE(GRTManager *grtm, const db_RoutineGroupRef &group, const db_mgmt_RdbmsRef &rdbms)
  : DBObjectEditorBE(grtm, group, rdbms), _group(group)
{
}

std::string RoutineGroupEditorBE::set_routine_newlines(const std::string &routine)
{
    // The parser returns existing new lines at the beginning of each routine
    // This logic ensures at least a blank line ( two new lines ) separate the next routine
    // from the sql so far
    std::string final_routine="";
    size_t routine_size = routine.length();
    
    if (routine_size >= 1 && routine[0]!='\n')
      final_routine.append("\n");

    if (routine_size >= 2 && routine[1]!='\n')
      final_routine.append("\n");

    return final_routine.append(routine);
}

std::string RoutineGroupEditorBE::get_routines_sql()
{
  std::string sql;

  grt::ListRef<db_Routine> routines= _group->routines();
  
  if(!routines.is_valid())
    return sql; // empty
  
  sql.append("-- --------------------------------------------------------------------------------\n");
  sql.append("-- ").append(_group->name().c_str()).append(" Group Routines\n");
  sql.append("-- --------------------------------------------------------------------------------\n");
  sql.append(strfmt("DELIMITER %s", _non_std_sql_delimiter.c_str()));

  size_t count= routines.count();
  typedef std::map<size_t, db_RoutineRef> OrderedRoutines;
  typedef std::list<db_RoutineRef> UnorderedRoutines;
  OrderedRoutines ordered_routines;
  UnorderedRoutines unordered_routines; // routines with duplicated sequence number. to upgrade old models smoothly, where sequence numbers are 0.

  for(size_t i= 0; i < count; i++)
  {
    db_RoutineRef routine= routines.get(i);
    size_t sequenceNumber= routine->sequenceNumber();
    if (ordered_routines.find(sequenceNumber) == ordered_routines.end())
      ordered_routines[sequenceNumber]= routine;
    else
      unordered_routines.push_back(routine);
  }

  for(OrderedRoutines::iterator i_end = ordered_routines.end(), i = ordered_routines.begin(); i != i_end; i++)
    sql.append(set_routine_newlines(i->second->sqlDefinition().repr())).append(_non_std_sql_delimiter);


  for (UnorderedRoutines::iterator i= unordered_routines.begin(), i_end= unordered_routines.end(); i != i_end; ++i)
    sql.append(set_routine_newlines((*i)->sqlDefinition().repr())).append(_non_std_sql_delimiter);

  return sql;
}

std::vector<std::string> RoutineGroupEditorBE::get_routines_names()
{
  std::vector<std::string> retval;

  grt::ListRef<db_Routine> routines= _group->routines();
  
  if(!routines.is_valid())
    return retval; // empty
  
  size_t count= routines.count();
  for(size_t i= 0; i < count; i++)
  {
    std::string qname(routines.get(i)->owner()->name());
    qname.append(".").append(routines.get(i)->name());
    retval.push_back(qname);
  }

  return retval;
}

std::string RoutineGroupEditorBE::get_routine_sql(db_RoutineRef routine)
{
  return std::string(routine->sqlDefinition().c_str());
}

void RoutineGroupEditorBE::delete_routine_with_name(const std::string& str)
{
  grt::ListRef<db_Routine> routines= _group->routines();
  
  if(!routines.is_valid())
    return;

  size_t count= routines.count();
  for(size_t i= 0; i < count; i++)
  {
    std::string qname(routines.get(i)->owner()->name());
    qname.append(".").append(routines.get(i)->name());

    if(str == qname)
    {
      routines.remove(i);
      break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Removes the routine at the given index from this group. Does nothing if index is out of range.
 */
void RoutineGroupEditorBE::remove_routine_by_index(size_t index)
{
  grt::ListRef<db_Routine> routines= _group->routines();
  
  if(!routines.is_valid())
    return;
  
  size_t count = routines.count();
  if (index >= count)
    return;
  
  //grt::AutoUndo undo(get_grt());
  AutoUndoEdit undo(this);
  routines.remove(index);
  undo.end(strfmt(_("Remove routine from routine group `%s`.%s`"), get_schema_name().c_str(), get_name().c_str()));
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the name of a routine given by its id.
 */
std::string RoutineGroupEditorBE::get_routine_name(const std::string& id)
{
  grt::ListRef<db_Routine> routines= _group->routines();
  
  if (!routines.is_valid())
    return "";

  size_t count= routines.count();
  for (size_t i= 0; i < count; i++)
  {
    std::string next_id(routines.get(i).id());

    if(id == next_id)
    {
      std::string qname(routines.get(i)->owner()->name());
      qname.append(".").append(routines.get(i)->name());
      return qname;
    }
  }
  return "";
}

void RoutineGroupEditorBE::append_routine_with_id(const std::string& id)
{
  
  // first ensure no routine with such id already in group
  {
    grt::ListRef<db_Routine> routines= _group->routines();
    
    if(!routines.is_valid())
      return;
  
    size_t count= routines.count();
    for(size_t i= 0; i < count; i++)
    {
      std::string next_id(routines.get(i).id());
  
      if(id == next_id)
        return;
    }
  }
  // now find the routine and add it
  {
    db_SchemaRef schema(db_SchemaRef::cast_from(_group->owner()));

    grt::ListRef<db_Routine> routines(schema->routines());

    size_t count= routines.count();
    for(size_t i= 0; i < count; i++)
    {
      std::string next_id(routines.get(i).id());
  
      if(id == next_id)
      {
        //grt::AutoUndo undo(get_grt());
        AutoUndoEdit undo(this);
        _group->routines().insert(routines.get(i));
        undo.end(strfmt(_("Add routine(s) to routine group `%s`.%s`"), get_schema_name().c_str(), get_name().c_str()));
        return;
      }
    }
  }
}


std::string RoutineGroupEditorBE::get_object_type()
{
  return "routine group";
}


void RoutineGroupEditorBE::set_routines_sql(const std::string &sql, bool sync)
{
  if (get_sql() != sql)
  {
    set_sql_parser_task_cb(boost::bind(&RoutineGroupEditorBE::parse_sql, this, _1, _2));
    //set_sql(sql, sync, _group);
  }
}


grt::ValueRef RoutineGroupEditorBE::parse_sql(grt::GRT* grt, grt::StringRef sql)
{
  AutoUndoEdit undo(this);

  int err_count= _sql_parser->parse_routines(_group, sql.c_str());

  undo.end(strfmt(_("Edit routine group `%s`.`%s`"), get_schema_name().c_str(), get_name().c_str()));

  check_sql();

  return grt::IntegerRef(err_count);
}


MySQLEditor::Ref RoutineGroupEditorBE::get_sql_editor()
{
  MySQLEditor::Ref sql_editor= DBObjectEditorBE::get_sql_editor();
  if (sql_editor)
    sql_editor->restrict_content_to(MySQLEditor::ContentTypeRoutine);
  return sql_editor;
}

std::string RoutineGroupEditorBE::get_title()
{
  return base::strfmt("%s - Group", get_name().c_str()); 
}

void RoutineGroupEditorBE::open_editor_for_routine_at_index(size_t index)
{
  if (index < _group->routines().count())
    get_grt_manager()->open_object_editor(_group->routines()[index]);
}
