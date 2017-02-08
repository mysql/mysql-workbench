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
#include "editor_routinegroup.h"

#include "base/string_utilities.h"

using namespace grt;
using namespace bec;
using namespace base;

//--------------------------------------------------------------------------------------------------

RoutineGroupEditorBE::RoutineGroupEditorBE(const db_RoutineGroupRef &group) : DBObjectEditorBE(group) {
  // No specific query type setting for the group editor. We have to parse the full spectrum (especially for delimiter
  // changes).
}

//--------------------------------------------------------------------------------------------------

/**
 * For routine groups getting the SQL is different compared to any other object editor. We are not using sqlDefinition()
 * of the object being edited (in fact, it doesn't even have this member), but collect all SQL text from the individual
 * routine objects we have in the routines() collection.
 */
std::string RoutineGroupEditorBE::get_sql() {
  grt::ListRef<db_Routine> routines = get_routine_group()->routines();
  if (!routines.is_valid())
    return "";

  std::string sql = "DELIMITER $$\n\n";

  typedef std::map<size_t, db_RoutineRef> OrderedRoutines;
  typedef std::list<db_RoutineRef> UnorderedRoutines;
  OrderedRoutines ordered_routines;
  UnorderedRoutines unordered_routines; // routines with duplicated sequence number. to upgrade old models smoothly,
                                        // where sequence numbers are 0.

  // XXX: the sequence number idea is utter nonsense. If a routine is part in different groups
  //       this number may be different in each group. Find a replacement or maybe we don't need it at all.
  for (size_t i = 0; i < routines.count(); ++i) {
    db_RoutineRef routine = routines.get(i);
    size_t sequenceNumber = routine->sequenceNumber();
    if (ordered_routines.find(sequenceNumber) == ordered_routines.end())
      ordered_routines[sequenceNumber] = routine;
    else
      unordered_routines.push_back(routine);
  }

  for (OrderedRoutines::iterator i = ordered_routines.begin(); i != ordered_routines.end(); ++i)
    sql += base::trim(i->second->sqlDefinition()) + "$$\n\n";

  for (UnorderedRoutines::iterator i = unordered_routines.begin(); i != unordered_routines.end(); ++i)
    sql += base::trim((*i)->sqlDefinition()) + "$$\n\n";

  return sql;
}

//--------------------------------------------------------------------------------------------------

std::vector<std::string> RoutineGroupEditorBE::get_routines_names() {
  std::vector<std::string> result;
  grt::ListRef<db_Routine> routines = get_routine_group()->routines();
  if (!routines.is_valid())
    return result;

  // TODO: isn't the owner for all routines the same?
  for (size_t i = 0; i < routines.count(); ++i)
    result.push_back(*routines[i]->owner()->name() + "." + *routines[i]->name());

  return result;
}

//--------------------------------------------------------------------------------------------------

std::string RoutineGroupEditorBE::get_routine_sql(db_RoutineRef routine) {
  return routine->sqlDefinition();
}

//--------------------------------------------------------------------------------------------------

void RoutineGroupEditorBE::delete_routine_with_name(const std::string &str) {
  grt::ListRef<db_Routine> routines = get_routine_group()->routines();

  if (!routines.is_valid())
    return;

  for (size_t i = 0; i < routines.count(); i++) {
    std::string qname = *routines[i]->owner()->name() + "." + *routines[i]->name();
    if (base::same_string(str, qname, _parser_context->case_sensitive())) {
      AutoUndoEdit undo(this);
      routines.remove(i);
      undo.end(strfmt(_("Remove routine from routine group `%s`.%s`"), get_schema_name().c_str(), get_name().c_str()));

      return;
    }
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Removes the routine at the given index from this group. Does nothing if index is out of range.
 */
void RoutineGroupEditorBE::remove_routine_by_index(size_t index) {
  grt::ListRef<db_Routine> routines = get_routine_group()->routines();

  if (!routines.is_valid() || index > routines.count())
    return;

  AutoUndoEdit undo(this);
  routines.remove(index);
  undo.end(strfmt(_("Remove routine from routine group `%s`.%s`"), get_schema_name().c_str(), get_name().c_str()));
}

//--------------------------------------------------------------------------------------------------

void RoutineGroupEditorBE::append_routine_with_id(const std::string &id) {
  // First ensure no routine with the same id exists already.
  grt::ListRef<db_Routine> routines = get_routine_group()->routines();

  if (!routines.is_valid())
    return;

  for (size_t i = 0; i < routines.count(); ++i) {
    if (base::same_string(id, routines[i].id(), _parser_context->case_sensitive()))
      return;
  }

  routines = get_schema()->routines();
  for (size_t i = 0; i < routines.count(); ++i) {
    if (base::same_string(id, routines[i].id(), _parser_context->case_sensitive())) {
      AutoUndoEdit undo(this);
      get_routine_group()->routines().insert(routines[i]);
      undo.end(strfmt(_("Add routine to routine group `%s`.%s`"), get_schema_name().c_str(), get_name().c_str()));

      return;
    }
  }
}

//--------------------------------------------------------------------------------------------------

std::string RoutineGroupEditorBE::get_title() {
  return get_name() + " - Group";
}

//--------------------------------------------------------------------------------------------------

void RoutineGroupEditorBE::open_editor_for_routine_at_index(size_t index) {
  if (index < get_routine_group()->routines().count())
    bec::GRTManager::get()->open_object_editor(get_routine_group()->routines()[index]);
}

//--------------------------------------------------------------------------------------------------
