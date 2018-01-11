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

#include "sqlide/recordset_table_inserts_storage.h"
#include "sqlide/recordset_be.h"

#include <grts/structs.db.h>
#include <grts/structs.db.query.h>

#include "objimpl/db.query/db_query_EditableResultset.h"

#include "base/string_utilities.h"
#include <grtpp_undo_manager.h>

#include "grt/common.h"

#include <sstream>

//================================================================================
// db_Table
// from db_ForeignKey.cpp
extern grt::ListRef<db_ForeignKey> get_foreign_keys_referencing_table(const db_TableRef &value);
extern void add_foreign_key_mapping(const db_TableRef &table, db_ForeignKey *fk);
extern void delete_foreign_key_mapping(const db_TableRef &table, db_ForeignKey *fk);

static void table_list_changed(grt::internal::OwnedList *list, bool added, const grt::ValueRef &value,
                               db_Table *table) {
  if (table->columns().valueptr() == list) {
    (*table->signal_refreshDisplay())("column");
  } else if (table->indices().valueptr() == list) {
    (*table->signal_refreshDisplay())("index");
  } else if (table->triggers().valueptr() == list) {
    (*table->signal_refreshDisplay())("trigger");
  } else if (table->foreignKeys().valueptr() == list) {
    db_ForeignKeyRef fk(db_ForeignKeyRef::cast_from(value));

    (*table->signal_refreshDisplay())("foreignKey");

    if (added)
      add_foreign_key_mapping(fk->referencedTable(), dynamic_cast<db_ForeignKey *>(fk.valueptr()));
    else
      delete_foreign_key_mapping(fk->referencedTable(), dynamic_cast<db_ForeignKey *>(fk.valueptr()));

    (*table->signal_foreignKeyChanged())(fk);
  }
}

void db_Table::init() {
  // No need in disconnet management since signal it part of object
  _list_changed_signal.connect(
    std::bind(&table_list_changed, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, this));
}

db_Table::~db_Table() {
}

grt::StringRef db_Table::inserts() {
  Recordset_table_inserts_storage::Ref input_storage = Recordset_table_inserts_storage::create();
  input_storage->table(db_TableRef(this));

  Recordset::Ref rs = Recordset::create();
  rs->data_storage(input_storage);
  rs->reset();

  Recordset_sql_storage::Ref output_storage = Recordset_sql_storage::create();
  output_storage->table_name(name());
  output_storage->rdbms(db_mgmt_RdbmsRef::cast_from(
    db_TableRef(this)->owner() /*schema*/->owner() /*catalog*/->owner() /*phys.model*/->get_member("rdbms")));
  output_storage->schema_name(owner()->name());
  output_storage->binding_blobs(false);

  output_storage->serialize(rs);
  return output_storage->sql_script();
}

db_query_EditableResultsetRef db_Table::createInsertsEditor() {
  Recordset_table_inserts_storage::Ref input_storage = Recordset_table_inserts_storage::create();
  input_storage->table(db_TableRef(this));

  Recordset::Ref rs = Recordset::create();
  rs->data_storage(input_storage);
  rs->reset();

  return grtwrap_editablerecordset(db_TableRef(this), rs);
}

void db_Table::addColumn(const db_ColumnRef &column) {
  _columns.insert(column);
  if (column->owner().valueptr() != this)
    column->owner(db_TableRef(this));
}

db_ForeignKeyRef db_Table::createForeignKey(const std::string &name) {
  db_ForeignKeyRef fk(grt::GRT::get()->create_object<db_ForeignKey>(_foreignKeys->content_type_spec().object_class));

  fk->owner(this);
  fk->name(name);

  _foreignKeys.insert(fk);

  return fk;
}

void db_Table::addIndex(const db_IndexRef &index) {
  _indices.insert(index);
  if (index->owner().valueptr() != this)
    index->owner(db_TableRef(this));
}

void db_Table::addPrimaryKeyColumn(const db_ColumnRef &column) {
  db_IndexRef pkindex;

  if (isPrimaryKeyColumn(column))
    return;

  grt::AutoUndo undo(!is_global());

  if (_columns.get_index(column) == grt::BaseListRef::npos)
    addColumn(column);

  std::string strname;

  pkindex = primaryKey();

  if (!pkindex.is_valid()) {
    grt::MetaClass *meta = get_metaclass();

    // primaryKey is not overridden by the db specific structs
    // strname= table.get_member_struct_name("primaryKey");
    // strname= table.get_member_content_struct_name("indices");
    strname = meta->get_member_type("indices").content.object_class;
    pkindex = grt::GRT::get()->create_object<db_Index>(strname);
    pkindex->name("PRIMARY");
    pkindex->oldName("PRIMARY");
    pkindex->owner(this);
    pkindex->indexType("PRIMARY");

    indices().insert(pkindex);

    pkindex->isPrimary(1);

    primaryKey(pkindex);
  }

  strname = pkindex.get_metaclass()->get_member_type("columns").content.object_class;

  db_IndexColumnRef pkicolumn(grt::GRT::get()->create_object<db_IndexColumn>(strname));

  pkicolumn->owner(pkindex);

  pkicolumn->referencedColumn(column);

  column->isNotNull(1);

  // hack for handling auto_increment columns in MySQL. these should be the 1st column
  // in the index
  if (column.has_member("autoIncrement") && column.get_integer_member("autoIncrement"))
    pkindex->columns().insert(pkicolumn, 0);
  else
    pkindex->columns().insert(pkicolumn);

  _signal_refreshDisplay("column");

  undo.end(_("Set Primary Key"));
}

grt::IntegerRef db_Table::isDependantTable() {
  if (primaryKey().is_valid()) {
    grt::ListRef<db_IndexColumn> columns(primaryKey()->columns());

    for (size_t c = columns.count(), i = 0; i < c; i++) {
      if (isForeignKeyColumn(columns.get(i)->referencedColumn()))
        return 1;
    }
  }
  return 0;
}

grt::IntegerRef db_Table::isForeignKeyColumn(const db_ColumnRef &column) {
  grt::ListRef<db_ForeignKey> fklist(foreignKeys());

  for (size_t c = fklist.count(), i = 0; i < c; i++) {
    grt::ListRef<db_Column> columns = fklist[i]->columns();

    for (size_t d = columns.count(), j = 0; j < d; j++) {
      if (columns[j] == column)
        return 1;
    }
  }
  return 0;
}

grt::IntegerRef db_Table::isPrimaryKeyColumn(const db_ColumnRef &column) {
  db_IndexRef pkindex = primaryKey();

  if (!pkindex.is_valid())
    return 0;

  size_t i, c;
  grt::ListRef<db_IndexColumn> column_list = pkindex->columns();
  c = column_list.count();
  for (i = 0; i < c; i++) {
    db_IndexColumnRef idxcolumn = column_list[i];

    if (idxcolumn->referencedColumn() == column)
      return 1;
  }

  return 0;
}

void db_Table::removeColumn(const db_ColumnRef &column) {
  grt::AutoUndo undo(!is_global());

  // make sure it's no longer a PK
  removePrimaryKeyColumn(column);

  // remove column from indices
  grt::ListRef<db_Index> indices = this->indices();
  for (size_t index = 0; index < indices.count(); ++index) {
    grt::ListRef<db_IndexColumn> icolumns = indices[index]->columns();
    for (size_t icolumn = 0; icolumn < icolumns.count(); ++icolumn) {
      if ((icolumns[icolumn])->referencedColumn() == column)
        icolumns.remove(icolumn);
    }

    // if the index has no columns, we delete it
    if (icolumns.count() == 0)
      indices.remove(index);
  }

  // remove column from foreign keys
  grt::ListRef<db_ForeignKey> fks = foreignKeys();
  for (ssize_t i = fks.count() - 1; i >= 0; --i) {
    db_ForeignKeyRef fk(fks[i]);
    grt::ListRef<db_Column> fcolumns(fk->columns());
    bool deleted = false;

    for (ssize_t j = fcolumns.count() - 1; j >= 0; --j) {
      db_ColumnRef col(fcolumns[j]);
      if (col == column) {
        fk->columns().remove(j);
        fk->referencedColumns().remove(j);
        deleted = true;
        break;
      }
    }

    // if the fks has no columns, we delete it
    if (fcolumns.count() == 0 && deleted)
      removeForeignKey(fk, 0);
  }

  // get all FKs that reference this column and remove them too
  grt::ListRef<db_ForeignKey> references(db_SchemaRef::cast_from(owner())->getForeignKeysReferencingTable(this));
  GRTLIST_FOREACH(db_ForeignKey, references, fk) {
    bool deleted = false;
    for (size_t c = (*fk)->referencedColumns().count(), i = 0; i < c; i++) {
      if ((*fk)->referencedColumns()[i] == column) {
        (*fk)->referencedColumns().remove(i);
        (*fk)->columns().remove(i);
        deleted = true;
        break;
      }
    }

    // if the fks has no columns, we delete it from the owning table
    if ((*fk)->columns().count() == 0 && deleted)
      (*fk)->owner()->removeForeignKey(*fk, 0);
  }

  // remove from tables column list
  columns().remove_value(column);

  undo.end(base::strfmt(_("Remove Column '%s.%s'"), name().c_str(), (*column->name()).c_str()));
}

void db_Table::removeForeignKey(const db_ForeignKeyRef &fk, ssize_t removeColumns) {
  // remove a fk from table and make sure its index is deleted too
  // if delete_columns is 1, it will also delete the columns that form the FK, except
  // columns used by other FKs

  grt::AutoUndo undo(!is_global());

  foreignKeys().remove_value(fk);
  if (fk->index().is_valid() && !fk->index()->isPrimary())
    indices().remove_value(fk->index());

  if (removeColumns > 0) {
    grt::ListRef<db_ForeignKey> fks(get_foreign_keys_referencing_table(db_TableRef(this)));

    db_ColumnRef cl;
    for (ssize_t i = fk->columns().count() - 1; i > -1; i--) {
      bool used = false;
      cl = fk->columns().get(i);

      // check if cl is used by some external FK
      for (size_t j = 0, c = fks.count(); j < c; j++) {
        db_ForeignKeyRef rfk(fks[j]);
        if (rfk != fk) {
          if (rfk->referencedColumns().get_index(cl) != grt::BaseListRef::npos) {
            used = true;
            break;
          }
        }
      }

      // check if cl is used by some other of our own FKs
      if (isForeignKeyColumn(cl))
        used = true;

      if (!used)
        removeColumn(cl);
    }
  }

  undo.end(_("Remove Foreign Key"));
}

void db_Table::removeIndex(const db_IndexRef &index) {
  throw std::logic_error("not implement");
  // QQQ
}

void db_Table::removePrimaryKeyColumn(const db_ColumnRef &column) {
  db_IndexRef pkindex;

  if (!isPrimaryKeyColumn(column))
    return;

  grt::AutoUndo undo(!is_global());

  pkindex = primaryKey();

  if (pkindex.is_valid()) {
    grt::ListRef<db_IndexColumn> pkColumns(pkindex->columns());

    for (ssize_t i = pkColumns.count() - 1; i > -1; i--) {
      db_ColumnRef pkcolumn(pkColumns.get(i)->referencedColumn());

      if (pkcolumn == column) {
        pkColumns.remove(i);
        break;
      }
    }
    if (pkColumns.count() == 0) {
      // remove primary index
      indices().remove_value(pkindex);

      primaryKey(db_IndexRef()); // set to nil
    }
  }

  undo.end(_("Unset Primary Key"));

  _signal_refreshDisplay("column");
}
