/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <grts/structs.db.h>

#include "base/string_utilities.h"
#include "base/util_functions.h"

#include "grtpp_util.h"

#include "grtpp_undo_manager.h"
#include "grt/common.h"

using namespace base;

//================================================================================
// db_Schema

void db_Schema::init() {
}

db_Schema::~db_Schema() {
}

db_RoutineRef db_Schema::addNewRoutine(const std::string &dbpackage) {
  grt::UndoManager *um = 0;
  db_RoutineRef routine;
  std::string class_name;

  if (is_global() && grt::GRT::get()->tracking_changes())
    um = grt::GRT::get()->get_undo_manager();

  class_name = dbpackage + ".Routine";

  std::string name = grt::get_name_suggestion_for_list_object(grt::ObjectListRef::cast_from(_routines), "routine");

  routine = grt::GRT::get()->create_object<db_Routine>(class_name);
  routine->owner(this);
  routine->name(name);
  routine->createDate(base::fmttime(0, DATETIME_FMT));
  routine->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  _routines.insert(routine);

  if (um) {
    um->set_action_description(_("Add New Routine Object"));
  }

  return routine;
}

db_RoutineGroupRef db_Schema::addNewRoutineGroup(const std::string &dbpackage) {
  grt::UndoManager *um = 0;
  db_RoutineGroupRef rgroup;
  std::string class_name;

  if (is_global() && grt::GRT::get()->tracking_changes())
    um = grt::GRT::get()->get_undo_manager();

  class_name = dbpackage + ".RoutineGroup";

  std::string name =
    grt::get_name_suggestion_for_list_object(grt::ObjectListRef::cast_from(routineGroups()), "routines");

  rgroup = grt::GRT::get()->create_object<db_RoutineGroup>(class_name);
  rgroup->owner(this);
  rgroup->name(name);
  rgroup->createDate(base::fmttime(0, DATETIME_FMT));
  rgroup->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  _routineGroups.insert(rgroup);
  if (um) {
    um->set_action_description(_("Add New Routine Group Object"));
  }

  return rgroup;
}

db_TableRef db_Schema::addNewTable(const std::string &dbpackage) {
  grt::UndoManager *um = 0;
  db_TableRef table;
  std::string class_name;

  if (is_global() && grt::GRT::get()->tracking_changes())
    um = grt::GRT::get()->get_undo_manager();

  class_name = dbpackage + ".Table";

  std::string name = grt::get_name_suggestion_for_list_object(grt::ObjectListRef::cast_from(_tables), "table");

  table = grt::GRT::get()->create_object<db_Table>(class_name);
  table->owner(this);
  table->name(name);
  table->createDate(base::fmttime(0, DATETIME_FMT));

  _tables.insert(table);
  if (um) {
    um->set_action_description(_("Add New Table Object"));
  }

  return table;
}

db_ViewRef db_Schema::addNewView(const std::string &dbpackage) {
  grt::UndoManager *um = 0;
  db_ViewRef view;
  std::string class_name;

  if (is_global() && grt::GRT::get()->tracking_changes())
    um = grt::GRT::get()->get_undo_manager();

  class_name = dbpackage + ".View";

  std::string name = grt::get_name_suggestion_for_list_object(grt::ObjectListRef::cast_from(_views), "view");

  view = grt::GRT::get()->create_object<db_View>(class_name);
  view->owner(this);
  view->name(name);
  view->createDate(base::fmttime(0, DATETIME_FMT));
  view->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  _views.insert(view);
  if (um) {
    um->set_action_description(_("Add New View Object"));
  }

  return view;
}

grt::ListRef<db_ForeignKey> db_Schema::getForeignKeysReferencingTable(const db_TableRef &table) {
  // from db_ForeignKey.cpp
  extern grt::ListRef<db_ForeignKey> get_foreign_keys_referencing_table(const db_TableRef &value);

  return get_foreign_keys_referencing_table(table);
}

void db_Schema::removeTable(const db_TableRef &table) {
  grt::AutoUndo undo(!is_global());

  // check foreign keys that refer to this table and reset them
  grt::ListRef<db_ForeignKey> foreignKeys(getForeignKeysReferencingTable(table));
  for (grt::ListRef<db_ForeignKey>::const_reverse_iterator fk = foreignKeys.rbegin(); fk != foreignKeys.rend(); ++fk) {
    grt::AutoUndo undo(!is_global());

    (*fk)->referencedTable(db_TableRef());
    (*fk)->columns().remove_all();
    (*fk)->referencedColumns().remove_all();

    undo.end(strfmt(_("Clear Referencing Foreign Key %s.%s"), (*fk)->owner()->name().c_str(), (*fk)->name().c_str()));
  }

  // delete from schema
  tables().remove_value(table);

  undo.end(strfmt(_("Delete Table '%s'"), table->name().c_str()));
}
