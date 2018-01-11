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

#include "grts/structs.db.h"

#include "grtpp_util.h"
#include "grtpp_undo_manager.h"

//================================================================================
// db_ForeignKey

// don't hold reference to the fk!
static std::map<grt::internal::Value *, std::set<db_ForeignKey *> > referenced_table_to_fk;

void db_ForeignKey::init() {
}

void delete_foreign_key_mapping(const db_TableRef &table, db_ForeignKey *fk) {
  if (table.is_valid()) {
    grt::internal::Value *t = table.valueptr();
    std::map<grt::internal::Value *, std::set<db_ForeignKey *> >::iterator iter = referenced_table_to_fk.find(t);
    if (iter != referenced_table_to_fk.end()) {
      if (iter->second.find(fk) != iter->second.end())
        iter->second.erase(iter->second.find(fk));

      // if no more FKs to this table, remove the entry
      if (iter->second.empty())
        referenced_table_to_fk.erase(iter);
    }
  }
}

void add_foreign_key_mapping(const db_TableRef &table, db_ForeignKey *fk) {
  if (table.is_valid()) {
    std::set<db_ForeignKey *> list;
    std::map<grt::internal::Value *, std::set<db_ForeignKey *> >::iterator iter;
    if ((iter = referenced_table_to_fk.find(table.valueptr())) != referenced_table_to_fk.end()) {
      iter->second.insert(fk);
    } else {
      list.insert(fk);
      referenced_table_to_fk[table.valueptr()] = list;
    }
  }
}

db_ForeignKey::~db_ForeignKey() {
  if (_referencedTable.is_valid())
    delete_foreign_key_mapping(_referencedTable, this);
}

grt::ListRef<db_ForeignKey> get_foreign_keys_referencing_table(const db_TableRef &value) {
  std::map<grt::internal::Value *, std::set<db_ForeignKey *> >::const_iterator iter;
  grt::ListRef<db_ForeignKey> result(true);

  if ((iter = referenced_table_to_fk.find(value.valueptr())) != referenced_table_to_fk.end()) {
    for (std::set<db_ForeignKey *>::const_iterator fk = iter->second.begin(); fk != iter->second.end(); ++fk) {
      result.insert(db_ForeignKeyRef(*fk));
    }
  }
  return result;
}

void db_ForeignKey::referencedTable(const db_TableRef &value) {
  grt::ValueRef ovalue(_referencedTable);

  // remove old referenced table from backreference map
  delete_foreign_key_mapping(_referencedTable, this);

  _referencedTable = value;

  // add new referenced table to backreference map
  add_foreign_key_mapping(value, this);

  member_changed("referencedTable", ovalue, value);

  if (_owner.is_valid())
    (*owner()->signal_foreignKeyChanged())(this);
}

void db_ForeignKey::owner(const db_TableRef &value) {
  super::owner(value);

  if (value.is_valid())
    (*value->signal_foreignKeyChanged())(this);
}

void db_ForeignKey::owned_list_item_added(grt::internal::OwnedList *list, const grt::ValueRef &value) {
  super::owned_list_item_added(list, value);

  if (_owner.is_valid())
    (*owner()->signal_foreignKeyChanged())(this);
}

void db_ForeignKey::owned_list_item_removed(grt::internal::OwnedList *list, const grt::ValueRef &value) {
  super::owned_list_item_removed(list, value);

  if (_owner.is_valid())
    (*owner()->signal_foreignKeyChanged())(this);
}

/** Performs basic validation of the foreign key
 */
grt::IntegerRef db_ForeignKey::checkCompleteness() {
  if (!_owner.is_valid() || !_referencedTable.is_valid())
    return 0;

  // If we are currently undoing then don't check completeness either.
  grt::UndoManager *um = grt::GRT::get()->get_undo_manager();
  if (um != NULL && um->is_undoing())
    return 0;

  if (db_TableRef::cast_from(_owner)->foreignKeys().get_index(db_ForeignKeyRef(this)) == grt::BaseListRef::npos)
    return 0;

  if (_columns.count() != _referencedColumns.count())
    return 0;

  for (size_t i = 0, c = _columns.count(); i < c; i++) {
    if (!_columns[i].is_valid() || !_referencedColumns[i].is_valid())
      return 0;
  }

  return 1;
}
