/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include <stdio.h>
#include <boost/scoped_array.hpp>
#include <string.h>

#include "base/string_utilities.h"
#include "base/util_functions.h"
#include "base/log.h"

#include "db_object_helpers.h"
#include "grtpp_undo_manager.h"
#include "grtpp_util.h"
#include "grt/parse_utils.h"
#include "grt/grt_manager.h"
#include "grts/structs.workbench.physical.h"
#include "grtdb/db_helpers.h"

DEFAULT_LOG_DOMAIN("dbhelpers");

using namespace bec;
using namespace grt;

template <class T>
class auto_array_ptr {
  T *_ptr;

public:
  auto_array_ptr(T *ptr) : _ptr(ptr) {
  }
  ~auto_array_ptr() {
    delete[] _ptr;
  }

  operator T *() {
    return _ptr;
  }
};

// Important: this functions works only for model objects, not live objects!
db_mgmt_RdbmsRef get_rdbms_for_db_object(const ::grt::ValueRef &object) {
  GrtObjectRef parent = GrtObjectRef::cast_from(object);
  while (parent.is_valid() && !parent.is_instance("workbench.physical.Model"))
    parent = parent->owner();

  // do it the hard way to avoid havig to link to objimpl for model
  // return workbench_physical_ModelRef::cast_from(parent)->rdbms();

  if (parent.is_valid())
    return db_mgmt_RdbmsRef::cast_from(parent.get_member("rdbms"));
  return db_mgmt_RdbmsRef();
}

//--------------------------------------------------------------------------------------------------

grt::ValueRef bec::getModelOption(workbench_physical_ModelRef model, const std::string &key, bool forceModel) {
  if (!model.is_valid()) {
    if (forceModel)
      return grt::ValueRef();

    if (key == "CatalogVersion")
      return bec::parse_version(bec::GRTManager::get()->get_app_option_string("DefaultTargetMySQLVersion"));
    else
      return bec::GRTManager::get()->get_app_option(key);
  }

  if (!model->options().is_valid() || (model->options().get_int("useglobal", 1) == 1  && forceModel == false) || (!model->options().has_key(key) && key != "CatalogVersion")) {
    if (key == "CatalogVersion")
      return bec::parse_version(bec::GRTManager::get()->get_app_option_string("DefaultTargetMySQLVersion"));
    else
      return bec::GRTManager::get()->get_app_option(key);
  }

  if (key == "CatalogVersion") {
    if (model->catalog().is_valid())
      return model->catalog()->version();
    else {
      logError("Unable to detect Catalog Version.\n");
      return grt::ValueRef();
    }
  }

  return model->options().get(key);
}

//--------------------------------------------------------------------------------------------------

bool CatalogHelper::is_type_valid_for_version(const db_SimpleDatatypeRef &type, const GrtVersionRef &target_version) {
  std::string validity = type->validity();
  GrtVersionRef valid_version;
  if (!validity.empty()) {
    bool match = false;
    switch (validity[0]) {
      case '<':
        if (validity[1] == '=') {
          valid_version = parse_version(validity.substr(2));
          if (version_equal(target_version, valid_version) || version_greater(valid_version, target_version))
            match = true;
        } else {
          valid_version = parse_version(validity.substr(1));
          if (version_greater(valid_version, target_version))
            match = true;
        }
        break;
      case '>':
        if (validity[1] == '=') {
          valid_version = parse_version(validity.substr(2));
          if (version_equal(target_version, valid_version) || version_greater(target_version, valid_version))
            match = true;
        } else {
          valid_version = parse_version(validity.substr(1));
          if (version_greater(target_version, valid_version))
            match = true;
        }
        break;
      case '=':
        valid_version = parse_version(validity.substr(1));
        if (version_equal(target_version, valid_version))
          match = true;
        break;
    }
    return match;
  }
  return true;
}

std::string CatalogHelper::dbobject_to_dragdata(const db_DatabaseObjectRef &object) {
  return object.class_name() + ":" + object.id();
}

db_DatabaseObjectRef CatalogHelper::dragdata_to_dbobject(const db_CatalogRef &catalog, const std::string &data) {
  if (data.find(':') != std::string::npos) {
    std::string oid = data.substr(data.find(':') + 1);

    return db_DatabaseObjectRef::cast_from(find_child_object(catalog, oid));
  }
  return db_DatabaseObjectRef();
}

std::string CatalogHelper::dbobject_list_to_dragdata(const std::list<db_DatabaseObjectRef> &objects) {
  std::string ret;
  for (std::list<db_DatabaseObjectRef>::const_iterator iter = objects.begin(); iter != objects.end(); ++iter) {
    if (!ret.empty())
      ret.append("\n");
    ret.append(dbobject_to_dragdata(*iter));
  }
  return ret;
}

std::list<db_DatabaseObjectRef> CatalogHelper::dragdata_to_dbobject_list(const db_CatalogRef &catalog,
                                                                         const std::string &data) {
  std::list<db_DatabaseObjectRef> dbobjects;
  std::vector<std::string> items = base::split(data, "\n");
  for (std::vector<std::string>::const_iterator item = items.begin(); item != items.end(); ++item) {
    db_DatabaseObjectRef object = dragdata_to_dbobject(catalog, *item);
    if (object.is_valid())
      dbobjects.push_back(object);
  }

  return dbobjects;
}

//------------------------------------------------------------------------------------
std::set<std::string> SchemaHelper::get_foreign_key_names(const db_SchemaRef &schema) {
  std::set<std::string> used_names;

  GRTLIST_FOREACH(db_Table, schema->tables(), table) {
    GRTLIST_FOREACH(db_ForeignKey, (*table)->foreignKeys(), fk) {
      used_names.insert((*fk)->name());
    }
  }

  return used_names;
}

std::string SchemaHelper::get_unique_foreign_key_name(std::set<std::string> &used_names, const std::string &prefix_,
                                                      int maxlength) {
  std::string prefix;
  std::string the_name = prefix_;
  int index = 0;

  // truncate if too long, on byte count, because that's what matters when creating
  // stuff on the server
  if ((int)the_name.size() > maxlength - 2) {
    const char *start = the_name.c_str();
    const char *end = the_name.c_str() + maxlength - 2;

    the_name = the_name.substr(0, g_utf8_find_prev_char(start, end) - start);
  }

  prefix = the_name;

  while (used_names.find(the_name) != used_names.end())
    the_name = base::strfmt("%s%i", prefix.c_str(), index++);

  if (the_name != prefix)
    used_names.insert(the_name);

  return the_name;
}

std::string SchemaHelper::get_unique_foreign_key_name(const db_SchemaRef &schema, const std::string &prefix_,
                                                      int maxlength) {
  std::set<std::string> used_names;
  std::string prefix;
  std::string the_name = prefix_;
  int index = 0;

  // truncate if too long, on byte count, because that's what matters when creating
  // stuff on the server
  if ((int)the_name.size() > maxlength - 2) {
    const char *start = the_name.c_str();
    const char *end = the_name.c_str() + maxlength - 2;

    the_name = the_name.substr(0, g_utf8_find_prev_char(start, end) - start);
  }

  prefix = the_name;

  GRTLIST_FOREACH(db_Table, schema->tables(), table) {
    GRTLIST_FOREACH(db_ForeignKey, (*table)->foreignKeys(), fk) {
      used_names.insert((*fk)->name());
      if (the_name == prefix && index == 0)
        index++;
    }
  }

  // prefix is duplicated
  if (index > 0) {
    do {
      the_name = base::strfmt("%s%i", prefix.c_str(), index++);
    } while (used_names.find(the_name) != used_names.end());
  }

  return the_name;
}

//------------------------------------------------------------------------------------

db_TableRef TableHelper::create_associative_table(const db_SchemaRef &schema, const db_TableRef &table1,
                                                  const db_TableRef &table2, bool mandatory1, bool mandatory2,
                                                  const db_mgmt_RdbmsRef &rdbms, const grt::DictRef &global_options,
                                                  const grt::DictRef &options) {
  db_TableRef atable;
  std::string name;

  AutoUndo undo;

  name = options.get_string("AuxTableTemplate", global_options.get_string("AuxTableTemplate", "%stable%_%dtable%"));

  name = base::replaceVariable(name, "%stable%", table1->name().c_str());
  name = base::replaceVariable(name, "%dtable%", table2->name().c_str());

  atable = grt::GRT::get()->create_object<db_Table>(table1.get_metaclass()->name());
  atable->owner(schema);
  atable->name(get_name_suggestion_for_list_object(schema->tables(), name, false));
  atable->oldName(atable->name());

  if (atable.has_member("tableEngine"))
    atable->set_member("tableEngine", table1.get_member("tableEngine"));
  if (atable.has_member("defaultCharacterSetName"))
    atable->set_member("defaultCharacterSetName", table1.get_member("defaultCharacterSetName"));
  if (atable.has_member("defaultCollationName"))
    atable->set_member("defaultCollationName", table1.get_member("defaultCollationName"));

  db_ForeignKeyRef first_fk =
    create_foreign_key_to_table(atable, table1, true, mandatory1, true, true, rdbms, global_options, options);
  schema->tables().insert(atable); // Insert put newly added FK into schema to avoid name collision
  create_foreign_key_to_table(atable, table2, true, mandatory2, true, true, rdbms, global_options, options);

  // when the 1st FK is created, the PK and FK will have the same columns, so the index will be reused
  // when the 2nd FK is created, the PK will have both columns so it can't be used by the 1st FK, so we have
  // to create a new one for it
  db_IndexRef index = create_index_for_fk(first_fk, rdbms->maximumIdentifierLength());
  first_fk->index(index);
  atable->indices().insert(index);

  atable->createDate(grt::StringRef(base::fmttime(0, DATETIME_FMT)));
  atable->lastChangeDate(atable->createDate());

  undo.end("Create Associative Table");

  return atable;
}

//
// static std::string format_ident_with_table(const std::string &fmt, const db_TableRef &table)
//{
//  return base::replaceVariable(fmt, "%table%", table->name().c_str());
//}

static std::string format_ident_with_stable_dtable(const std::string &fmt, const db_TableRef &stable,
                                                   const db_TableRef &dtable) {
  return base::replaceVariable(base::replaceVariable(fmt, "%stable%", stable->name().c_str()), "%dtable%",
                               dtable->name().c_str());
}

static std::string format_ident_with_column(const std::string &fmt, const db_ColumnRef &column) {
  return base::replaceVariable(
    base::replaceVariable(fmt, "%table%", db_TableRef::cast_from(column->owner())->name().c_str()), "%column%",
    column->name().c_str());
}

db_IndexRef TableHelper::create_index_for_fk(const db_ForeignKeyRef &fk, const size_t max_len) {
  std::string index_name(fk->name().c_str());
  if (index_name.length() > (max_len - 5))
    index_name.resize(max_len - 5);
  index_name.append("_idx");
  index_name = grt::get_name_suggestion_for_list_object(fk->owner()->indices(), index_name, false);

  // add the corresponding index
  db_IndexRef index = grt::GRT::get()->create_object<db_Index>(
    db_TableRef::cast_from(fk->owner()).get_metaclass()->get_member_type("indices").content.object_class.c_str());

  index->owner(fk->owner());
  index->name(index_name);
  index->oldName(fk->oldName());
  index->indexType("INDEX");

  for (size_t i = 0, sz = fk->columns().count(); i < sz; i++) {
    db_ColumnRef col(fk->columns().get(i));

    db_IndexColumnRef index_col(grt::GRT::get()->create_object<db_IndexColumn>(
      index.get_metaclass()->get_member_type("columns").content.object_class));
    index_col->owner(index);
    //    "name", col->name().valueptr(),
    index_col->descend(grt::IntegerRef(0));
    //"columnLength", grt::IntegerRef(col->length().valueptr()).valueptr(),
    index_col->columnLength(grt::IntegerRef(0));
    index_col->referencedColumn(col);

    index->columns().insert(index_col);
  }

  return index;
}

void TableHelper::reorder_foreign_key_for_index(const db_ForeignKeyRef &fk, const db_IndexRef &index) {
  // make the order of the columns in the FK match that of the index
  size_t column_count = fk->columns().count();

  if (fk->columns().count() != fk->referencedColumns().count()) {
    logError(
      "Internal consistency error: number of items in fk->columns and fk->referencedColumns() for %s.%s.%s do not "
      "match\n",
      fk->owner()->owner()->name().c_str(), fk->owner()->name().c_str(), fk->name().c_str());
    return;
  }
  if (column_count > index->columns().count()) {
    logError("Internal consistency error: number of items in index for FK is less than columns in FK %s.%s.%s\n",
             fk->owner()->owner()->name().c_str(), fk->owner()->name().c_str(), fk->name().c_str());
    return;
  }

  for (size_t j = 0; j < column_count; j++) {
    if (index->columns()[j]->referencedColumn() != fk->columns()[j]) {
      // if the column in the index is not the expected one, reorder the FK

      // find the position of the column in the FK
      for (size_t k = j + 1; k < column_count; k++) {
        if (index->columns()[j]->referencedColumn() == fk->columns()[k]) {
          // reorder the columns in the FK
          fk->columns().reorder(k, j);
          fk->referencedColumns().reorder(k, j);
          break;
        }
      }
      break;
    }
  }
}

db_IndexRef TableHelper::find_index_usable_by_fk(const db_ForeignKeyRef &fk, const db_IndexRef &other_than,
                                                 bool allow_any_order) {
  size_t column_count = fk->columns().count();
  db_TableRef table(db_TableRef::cast_from(fk->owner()));

  if (column_count == 0)
    return db_IndexRef();

  for (size_t c = table->indices().count(), i = 0; i < c; i++) {
    db_IndexRef index(table->indices()[i]);

    if (index == other_than)
      continue;

    // smaller indexes than the FK won't work
    size_t index_column_count = index->columns().count();
    if (index_column_count >= column_count) {
      bool index_usable;
      if (allow_any_order) {
        index_usable = true;
        for (size_t j = 0; j < column_count; j++) {
          bool ok = false;
          // check whether this FK column is in the index
          for (size_t k = 0; k < index_column_count; k++) {
            if (index->columns()[k]->referencedColumn() == fk->columns()[j]) {
              ok = true;
              break;
            }
          }
          if (!ok) {
            index_usable = false;
            break;
          }
        }

        if (index_usable) {
          size_t c = 0;
          // now check whether only columns from the prefix of the index are in use in the FK..
          // example: FK (a, b) and INDEX (a, b, c) is OK
          //          FK (b, c) and INDEX (a, b, c) is not ok
          for (size_t j = 0; j < column_count && c < column_count; j++) {
            if (fk->columns().get_index(index->columns()[j]->referencedColumn()) != grt::BaseListRef::npos)
              c++; // jth column is in the FK, so it's OK so far
            else {
              index_usable = false;
              break; // jth column of the index is not in the FK, so this index can't be used
            }
          }
        }
      } else {
        index_usable = false;
        // make sure that the FK form prefix of the columns in the index
        for (size_t j = 0; j < column_count; j++) {
          if (index->columns()[j]->referencedColumn() != fk->columns()[j]) {
            index_usable = false;
            break;
          } else
            index_usable = true; // there's at least one match
        }
      }
      if (index_usable)
        return index;
    }
  }
  return db_IndexRef();
}

void TableHelper::update_foreign_keys_from_column_notnull(const db_TableRef &table, const db_ColumnRef &column) {
  AutoUndo undo;

  // go through all foreign keys and update the ones that have this column
  grt::ListRef<db_ForeignKey> fklist(table->foreignKeys());

  for (size_t c = fklist.count(), i = 0; i < c; i++) {
    db_ForeignKeyRef fk(fklist[i]);
    size_t notnull = 0;
    bool flag = false;

    for (size_t d = fk->columns().count(), j = 0; j < d; j++) {
      db_ColumnRef col(fk->columns()[j]);

      if (*col->isNotNull())
        notnull++;

      if (col == column)
        flag = true;
    }
    if (flag) {
      if (notnull == fk->columns().count()) {
        // if the FK is optional, then it means the ref table is optional
        fk->referencedMandatory(1);
      } else if (notnull == 0) {
        fk->referencedMandatory(0);
      }
    }
  }

  undo.end("Update FK Mandatory Flag");
}

std::string TableHelper::generate_foreign_key_name() {
  return std::string("fk_") + grt::get_guid();
}

db_ForeignKeyRef TableHelper::create_empty_foreign_key(const db_TableRef &table, const std::string &name) {
  db_ForeignKeyRef fk;

  // create a new FK
  fk = grt::GRT::get()->create_object<db_ForeignKey>(
    table.get_metaclass()->get_member_type("foreignKeys").content.object_class);
  fk->owner(table);
  fk->name(name.empty() ? generate_foreign_key_name() : name);

  grt::AutoUndo undo;

  table->foreignKeys().insert(fk);

  /* don't always create an index for the FK, it should only be created if there are no other indexes that match it
   the check for indexes should be done every time the FK columns are edited
  db_IndexRef index(create_index_for_fk(fk));
  fk->index(index);
  table->indices().insert(index);
  */
  undo.end(_("Create Foreign Key"));

  return fk;
}

void TableHelper::update_foreign_key_index(const db_ForeignKeyRef &fk) {
  //! todo: add undo group

  db_TableRef table(fk->owner());
  db_IndexRef index(fk->index());

  if (index.is_valid()) {
    db_IndexRef other;
    if ((other = find_index_usable_by_fk(fk, index, true)).is_valid()) {
      // the index from this FK is redundant, so remove it
      fk->index(db_IndexRef());
      table->indices().remove_value(index);
      reorder_foreign_key_for_index(fk, other);
      return;
    }

    // make sure that the columns in the index match the ones in the FK

    // remove columns that are gone from the FK
    for (ssize_t i = index->columns().count() - 1; i >= 0; --i) {
      if (fk->columns().get_index(index->columns()[i]) == grt::BaseListRef::npos)
        index->columns().remove(i);
    }

    // recreate index columns
    while (index->columns().count() > 0)
      index->columns().remove(0);

    ListRef<db_Column> fk_columns(fk->columns());
    for (size_t n = 0, count = fk_columns.count(); n < count; ++n) {
      db_ColumnRef column(fk_columns.get(n));
      db_IndexColumnRef index_column(grt::GRT::get()->create_object<db_IndexColumn>(
        index.get_metaclass()->get_member_type("columns").content.object_class));
      index_column->owner(index);
      //"name", column->name().valueptr(),
      index_column->referencedColumn(column);
      index->columns().insert(index_column);
    }

    if (index->columns().count() == 0) {
      // the index is empty, remove it
      fk->index(db_IndexRef());
      table->indices().remove_value(index);
      return;
    }
  } else
    create_index_for_fk_if_needed(fk);
}

bool TableHelper::create_index_for_fk_if_needed(db_ForeignKeyRef fk) {
  db_IndexRef index = find_index_usable_by_fk(fk, db_IndexRef(), true);
  if (index.is_valid())
    reorder_foreign_key_for_index(fk, index);
  else if (fk->columns().count() > 0) {
    index = create_index_for_fk(fk);
    fk->index(index);
    fk->owner()->indices().insert(index);
    return true;
  }
  return false;
}

bool TableHelper::create_missing_indexes_for_foreign_keys(const db_TableRef &table) {
  bool created = false;
  GRTLIST_FOREACH(db_ForeignKey, table->foreignKeys(), fk) {
    // check if an index already exists for the FK columns and if not, create one
    if (!(*fk)->index().is_valid())
      created = created || create_index_for_fk_if_needed(*fk);
    else
      reorder_foreign_key_for_index(*fk, (*fk)->index());
  }
  return created;
}

/**
 ****************************************************************************
 * @brief Creates a foreign key in table, refering to ref_table
 *
 *
 * Options:
 * FKNameTemplate, FKColumnNameTemplate, db_ForeignKey:deleteRule, updateRule
 *
 * @param table the table to have the FK added to
 * @param ref_table the table that the FK refers to
 * @param mandatory if the relationship is mandatory in table
 * @param ref_mandatory if the relationship is mandatory in referenced table
 * @param many cardinality of the table in the relationship
 * @param identifying whether the rel is identifying (FK is also made PK)
 * @param options the options dictionary (see above)
 *
 * @return the created foreign key
 ****************************************************************************
 */
db_ForeignKeyRef TableHelper::create_foreign_key_to_table(const db_TableRef &table, const db_TableRef &ref_table,
                                                          bool mandatory, bool ref_mandatory, bool many,
                                                          bool identifying, const db_mgmt_RdbmsRef &rdbms,
                                                          const grt::DictRef &global_options,
                                                          const grt::DictRef &options) {
  db_ForeignKeyRef new_fk;
  db_IndexRef pk = ref_table->primaryKey();
  std::string name_format;
  std::string column_name_format;
  std::string scolumn_name;
  std::string dcolumn_name;
  size_t max_identifier_length = rdbms->maximumIdentifierLength();

  // check if there is a PK
  if (!pk.is_valid() || pk->columns().count() == 0)
    return new_fk;

  grt::AutoUndo undo(!table->is_global());

  name_format = options.get_string("FKNameTemplate", global_options.get_string("FKNameTemplate", "FK%table%"));
  column_name_format =
    options.get_string("FKColumnNameTemplate", global_options.get_string("FKColumnNameTemplate", "FK%table%%column%"));

  name_format = format_ident_with_stable_dtable(name_format, table, ref_table);
  name_format = format_ident_with_column(name_format, pk->columns().get(0)->referencedColumn());

  column_name_format = format_ident_with_stable_dtable(column_name_format, table, ref_table);

  // create a new FK
  new_fk = grt::GRT::get()->create_object<db_ForeignKey>(
    table.get_metaclass()->get_member_type("foreignKeys").content.object_class);
  new_fk->oldName(new_fk->name());

  new_fk->deleteRule(
    options.get_string("db.ForeignKey:deleteRule", global_options.get_string("db.ForeignKey:deleteRule", "NO ACTION")));
  new_fk->updateRule(
    options.get_string("db.ForeignKey:updateRule", global_options.get_string("db.ForeignKey:updateRule", "NO ACTION")));

  new_fk->mandatory(mandatory ? 1 : 0);
  new_fk->referencedMandatory(ref_mandatory ? 1 : 0);
  new_fk->many(many ? 1 : 0);

  new_fk->referencedTable(ref_table);

  for (size_t c = pk->columns().count(), i = 0; i < c; i++) {
    db_IndexColumnRef pk_column = pk->columns().get(i);
    db_ColumnRef column = pk_column->referencedColumn();
    db_ColumnRef new_fk_column;

    // create the column that will be the FK in the other table
    new_fk_column = grt::GRT::get()->create_object<db_Column>(column.class_name());
    new_fk_column->owner(table);
    new_fk_column->name(get_name_suggestion_for_list_object(
      table->columns(), format_ident_with_column(column_name_format, column), false));
    new_fk_column->oldName(new_fk_column->name());
    new_fk_column->isNotNull(ref_mandatory ? 1 : 0);

    ColumnHelper::copy_column(column, new_fk_column);

    table->columns().insert(new_fk_column);

    // add the new column to the FK
    new_fk->columns().insert(new_fk_column);
    new_fk->referencedColumns().insert(column);

    if (identifying)
      table->addPrimaryKeyColumn(new_fk_column);

    if (scolumn_name.empty()) {
      scolumn_name = column->name();
      dcolumn_name = new_fk_column->name();
    }
  }

  // substitute scolumn/dcolumn now that we know the created column name
  name_format =
    base::replaceVariable(base::replaceVariable(name_format, "%scolumn%", scolumn_name), "%dcolumn%", dcolumn_name);

  new_fk->name(SchemaHelper::get_unique_foreign_key_name(db_SchemaRef::cast_from(table->owner()), name_format,
                                                         (int)max_identifier_length));
  new_fk->oldName(new_fk->name());

  new_fk->owner(table); // set the owner last

  db_IndexRef index;
  // check if an index already exists for the FK columns and if not, create one
  if (!find_index_usable_by_fk(new_fk).is_valid()) {
    index = create_index_for_fk(new_fk, rdbms->maximumIdentifierLength());
    new_fk->index(index);
    // index is inserted later down
  }

  // add the FK to the source table
  table->foreignKeys().insert(new_fk);
  undo.set_description_for_last_action("Add Foreign Key to Table");

  if (index.is_valid()) {
    table->indices().insert(index);
    undo.set_description_for_last_action("Add Index for FK to Table");
  }
  undo.end(_("Add Foreign Key"));

  return new_fk;
}

db_ForeignKeyRef TableHelper::create_foreign_key_to_table(
  const db_TableRef &table, const std::vector<db_ColumnRef> &columns, const db_TableRef &ref_table,
  const std::vector<db_ColumnRef> &refcolumns, bool mandatory, bool many, const db_mgmt_RdbmsRef &rdbms,
  const grt::DictRef &global_options, const grt::DictRef &options) {
  db_ForeignKeyRef new_fk;
  std::string name_format;
  std::string scolumn_name;
  std::string dcolumn_name;
  size_t max_identifier_length = rdbms->maximumIdentifierLength();
  bool ref_mandatory;

  grt::AutoUndo undo(table->is_global());

  name_format = options.get_string("FKNameTemplate", global_options.get_string("FKNameTemplate", "FK%table%"));

  name_format = format_ident_with_stable_dtable(name_format, table, ref_table);
  name_format = format_ident_with_column(name_format, refcolumns[0]);

  // create a new FK
  new_fk = grt::GRT::get()->create_object<db_ForeignKey>(
    table.get_metaclass()->get_member_type("foreignKeys").content.object_class);

  new_fk->deleteRule(
    options.get_string("db.ForeignKey:deleteRule", global_options.get_string("db.ForeignKey:deleteRule", "NO ACTION")));
  new_fk->updateRule(
    options.get_string("db.ForeignKey:updateRule", global_options.get_string("db.ForeignKey:updateRule", "NO ACTION")));

  new_fk->referencedTable(ref_table);

  new_fk->mandatory(mandatory ? 1 : 0);
  // new_fk->referencedMandatory(ref_mandatory?1:0); done later
  new_fk->many(many ? 1 : 0);

  ref_mandatory = false;
  for (size_t c = refcolumns.size(), i = 0; i < c; i++) {
    db_ColumnRef column(refcolumns[i]);
    db_ColumnRef fk_column(columns[i]);

    if (column->isNotNull())
      ref_mandatory = true;

    // add the new column to the FK
    new_fk->columns().insert(fk_column);
    new_fk->referencedColumns().insert(column);
    if (scolumn_name.empty()) {
      scolumn_name = fk_column->name();
      dcolumn_name = column->name();
    }
  }

  // substitute scolumn/dcolumn now that we know the created column name
  name_format =
    base::replaceVariable(base::replaceVariable(name_format, "%scolumn%", scolumn_name), "%dcolumn%", dcolumn_name);

  new_fk->name(SchemaHelper::get_unique_foreign_key_name(db_SchemaRef::cast_from(table->owner()), name_format,
                                                         (int)max_identifier_length));
  new_fk->oldName(new_fk->name());

  new_fk->referencedMandatory(ref_mandatory ? 1 : 0);

  new_fk->owner(table);

  // check if an index already exists for the FK columns and if not, create one
  if (!find_index_usable_by_fk(new_fk).is_valid()) {
    db_IndexRef index(create_index_for_fk(new_fk, rdbms->maximumIdentifierLength()));
    new_fk->index(index);

    // add the FK to the source table
    table->foreignKeys().insert(new_fk);
    table->indices().insert(index);
  } else // add the FK to the source table
    table->foreignKeys().insert(new_fk);

  undo.end(_("Add Foreign Key"));

  return new_fk;
}

bool TableHelper::rename_foreign_key(const db_TableRef &table, db_ForeignKeyRef &fk, const std::string &new_name) {
  std::string old_name;

  // check if the name is already taken
  if (find_named_object_in_list(table->foreignKeys(), new_name).is_valid())
    return false;

  old_name = fk->name();

  grt::AutoUndo undo;

  fk->name(new_name);

  // only rename the index if the old names match
  if (fk->index().is_valid()) {
    if (old_name == *fk->index()->name())
      fk->index()->name(new_name);
  }
  undo.end(_("Rename Foreign Key"));

  return true;
}

bool TableHelper::is_identifying_foreign_key(const db_TableRef &table, const db_ForeignKeyRef &fk) {
  // check if the fk is part of the PK
  if (table->primaryKey().is_valid()) {
    for (size_t c = fk->columns().count(), i = 0; i < c; i++) {
      if (!table->isPrimaryKeyColumn(fk->columns().get(i)))
        return false;
    }
    return true;
  }
  return false;
}

db_mysql_StorageEngineRef TableHelper::get_engine_by_name(const std::string &name) {
  grt::ListRef<db_mysql_StorageEngine> engines;
  Module *module = grt::GRT::get()->get_module("DbMySQL");

  if (!module)
    throw std::logic_error("module DbMySQL not found");

  grt::BaseListRef args(true);
  engines = grt::ListRef<db_mysql_StorageEngine>::cast_from(module->call_function("getKnownEngines", args));

  if (engines.is_valid()) {
    for (grt::ListRef<db_mysql_StorageEngine>::const_iterator iter = engines.begin(); iter != engines.end(); ++iter) {
      if ((*iter)->name() == name)
        return *iter;
    }
  }
  return db_mysql_StorageEngineRef();
}

/**
 * Returns a pointer to the position of the first empty line (NOT a line break!) or the first character after the
 * maximum count, whichever comes first. If no line break was found and the string is shorter
 * than the maximum length the result points to the terminating 0.
 */
static void split_comment(const std::string &comment, size_t db_comment_len, std::string *comment_ret,
                          std::string *leftover_ret) {
  size_t res;
  // XXX: check for Unicode line breaks! especially asian languages may not use the ANSI new line.
  const gchar *pointer_to_linebreak = NULL;

  { // find 1st occurrence of 2 consecutive newlines
    std::string::size_type pos = comment.find("\n\n");
    if (pos == std::string::npos)
      pos = comment.find("\r\n\r\n");

    if (pos != std::string::npos)
      pointer_to_linebreak = comment.c_str() + pos;
  }

  // We need the number of characters which the string part includes, so convert to a char count.
  if (pointer_to_linebreak != NULL)
    res = g_utf8_pointer_to_offset(comment.c_str(), pointer_to_linebreak);
  else
    res = comment.size(); // it's wrong to use g_utf8_strlen here because the comment length must be measured in bytes,
                          // not unichars

  // Don't break in the middle of a utf8 sequence
  if (res > db_comment_len) {
    if (g_utf8_get_char_validated(comment.c_str() + db_comment_len, (gssize)(res - db_comment_len)) == (gunichar)-1)
      res = g_utf8_pointer_to_offset(comment.c_str(),
                                     g_utf8_find_prev_char(comment.c_str(), comment.c_str() + db_comment_len));
    else
      res = db_comment_len;
  }
  if (comment_ret)
    *comment_ret = comment.substr(0, res);
  if (leftover_ret) {
    if (pointer_to_linebreak != NULL)
      *leftover_ret = comment.substr(res + 1);
    else
      *leftover_ret = comment.substr(res);
  }
}

std::string TableHelper::get_sync_comment(const std::string &comment, const size_t max_len) {
  std::string ret;
  if (comment.size() > max_len)
    split_comment(comment, max_len, &ret, NULL);
  else
    ret = comment;
  return ret;
};

std::string TableHelper::normalize_table_name_list(const std::string &schema, const std::string &table_name_list) {
  std::vector<std::string> names = base::split(table_name_list, ",");

  for (std::vector<std::string>::iterator it = names.begin(); it != names.end(); ++it) {
    std::vector<std::string> tokens(base::split_qualified_identifier(base::trim(*it)));
    if (tokens.size() == 1)
      tokens.insert(tokens.begin(), schema);

    for (std::vector<std::string>::iterator t = tokens.begin(); t != tokens.end(); ++t)
      *t = base::quote_identifier(base::unquote_identifier(*t), '`');

    *it = base::join(tokens, ".");
  }

  return base::join(names, ",");
}

void ColumnHelper::copy_column(const db_ColumnRef &from, db_ColumnRef &to) {
  to->userType(from->userType());
  to->precision(from->precision());
  to->scale(from->scale());
  to->length(from->length());
  to->characterSetName(from->characterSetName());
  to->collationName(from->collationName());
  while (to->flags().count() > 0)
    to->flags().remove(0);
  for (size_t c = from->flags().count(), i = 0; i < c; i++)
    to->flags().insert(from->flags().get(i));
  to->simpleType(from->simpleType());
  to->structuredType(from->structuredType());
  to->datatypeExplicitParams(from->datatypeExplicitParams());
}

ColumnTypeCompareResult ColumnHelper::compare_column_types(const db_ColumnRef &from, const db_ColumnRef &to) {
  // not to be used for foreign key column matching as the rules are different and DB dependant
  std::string sfrom = from->formattedType();
  std::string sto = to->formattedType();

  if (sfrom != sto)
    return COLUMNS_TYPES_DIFFER;

  if (to->characterSetName() != from->characterSetName())
    return COLUMNS_CHARSETS_DIFFER;

  if (to->collationName() != from->collationName())
    return COLUMNS_COLLATIONS_DIFFER;

  if (to->flags().count() != from->flags().count())
    return COLUMNS_FLAGS_DIFFER;

  for (size_t c = from->flags().count(), i = 0; i < c; i++)
    if (to->flags().get_index(from->flags().get(i)) == BaseListRef::npos)
      return COLUMNS_FLAGS_DIFFER;

  // XXX compare db specific attribs

  return COLUMNS_TYPES_EQUAL;
}

void ColumnHelper::set_default_value(db_ColumnRef column, const std::string &value) {
  column->defaultValueIsNull(base::same_string(value, "NULL", false) ? 1 : 0);
  column->defaultValue(value.c_str());

  // If a default value of NULL was set then the column can no longer be a not-null column.
  if (column->defaultValueIsNull())
    column->isNotNull(false);

  // Handling auto-increment columns is MySQL specific and done elsewhere.
}

//------------------------------------------------------------------------------------

void CatalogHelper::apply_defaults(db_mysql_ColumnRef column) {
  // for numeric types only
  static std::map<std::string, int> def_precision_map;
  if (def_precision_map.empty()) {
    def_precision_map["INT"] = 11;
    def_precision_map["TINYINT"] = 4;
    def_precision_map["SMALLINT"] = 6;
    def_precision_map["MEDIUMINT"] = 9;
    def_precision_map["BIGINT"] = 20;
    def_precision_map["FLOAT"] = 11;
    def_precision_map["DOUBLE"] = 11;
    def_precision_map["BIT"] = 1;
    def_precision_map["CHAR"] = 1;
  }

  bool default_default_value = !column->isNotNull() && (strlen(column->defaultValue().c_str()) == 0);

  if (!column->simpleType().is_valid())
    return;

  std::map<std::string, int>::const_iterator prec_map_it = def_precision_map.find(column->simpleType()->name().c_str());
  if (prec_map_it != def_precision_map.end()) {
    if ((strcmp(column->simpleType()->name().c_str(), "CHAR") != 0)) // length should be used for chars
      column->length(grt::IntegerRef(-1));
    if (column->precision() == bec::EMPTY_COLUMN_PRECISION) {
      if (column->flags().get_index("UNSIGNED") != grt::BaseListRef::npos)
        column->precision(grt::IntegerRef(prec_map_it->second) - 1);
      else
        column->precision(grt::IntegerRef(prec_map_it->second));
    }
    if (default_default_value)
      bec::ColumnHelper::set_default_value(column, "NULL");
  } else if ((strcmp(column->simpleType()->name().c_str(), "VARCHAR") == 0) ||
             (strcmp(column->simpleType()->name().c_str(), "CHAR") == 0) ||
             (strcmp(column->simpleType()->name().c_str(), "DECIMAL") == 0) ||
             (strcmp(column->simpleType()->name().c_str(), "BOOLEAN") == 0) ||
             (strcmp(column->simpleType()->name().c_str(), "BINARY") == 0) ||
             (strcmp(column->simpleType()->name().c_str(), "TINYBLOB") == 0) ||
             (strcmp(column->simpleType()->name().c_str(), "MEDIUMBLOB") == 0) ||
             (strcmp(column->simpleType()->name().c_str(), "BLOB") == 0) ||
             (strcmp(column->simpleType()->name().c_str(), "LONGBLOB") == 0) ||
             (strcmp(column->simpleType()->name().c_str(), "TINYTEXT") == 0) ||
             (strcmp(column->simpleType()->name().c_str(), "TEXT") == 0) ||
             (strcmp(column->simpleType()->name().c_str(), "MEDIUMTEXT") == 0) ||
             (strcmp(column->simpleType()->name().c_str(), "LONGTEXT") == 0) ||
             (strcmp(column->simpleType()->name().c_str(), "ENUM") == 0) ||
             (strcmp(column->simpleType()->name().c_str(), "SET") == 0)) {
    if (default_default_value)
      bec::ColumnHelper::set_default_value(column, "NULL");
  } else if ((strcmp(column->simpleType()->name().c_str(), "DATETIME") == 0) ||
             (strcmp(column->simpleType()->name().c_str(), "DATE") == 0) ||
             (strcmp(column->simpleType()->name().c_str(), "TIME") == 0) ||
             (strcmp(column->simpleType()->name().c_str(), "TIMESTAMP") == 0) ||
             (strcmp(column->simpleType()->name().c_str(), "YEAR") == 0)) {
    column->length(grt::IntegerRef(-1));
    if (default_default_value)
      bec::ColumnHelper::set_default_value(column, "NULL");
  }
};

//--------------------------------------------------------------------------------------------------

void CatalogHelper::apply_defaults(db_mysql_CatalogRef cat, std::string default_engine) {
  cat->defaultCharacterSetName("utf8");
  cat->defaultCollationName("utf8_general_ci");

  for (size_t i = 0, schemata_count = cat->schemata().count(); i < schemata_count; i++) {
    db_mysql_SchemaRef schema = cat->schemata().get(i);

    if (schema->defaultCharacterSetName().empty())
      schema->defaultCharacterSetName(cat->defaultCharacterSetName());
    if (schema->defaultCollationName().empty())
      schema->defaultCollationName(defaultCollationForCharset(schema->defaultCharacterSetName()));

    for (size_t j = 0, tables_count = schema->tables().count(); j < tables_count; j++) {
      db_mysql_TableRef table = schema->tables().get(j);

      if (table->defaultCharacterSetName().empty())
        table->defaultCharacterSetName(schema->defaultCharacterSetName());

      if (table->defaultCharacterSetName() == schema->defaultCharacterSetName()) {
        if (strlen(table->defaultCollationName().c_str()) == 0)
          table->defaultCollationName(schema->defaultCollationName());
      } else {
        if (table->defaultCollationName().empty())
          table->defaultCollationName(defaultCollationForCharset(table->defaultCharacterSetName()));
      }

      if (table->tableEngine().empty())
        table->tableEngine(default_engine.empty() ? "InnoDB" : default_engine);

      for (size_t c = table->foreignKeys().count(), i = 0; i < c; i++) {
        db_ForeignKeyRef fk(table->foreignKeys()[i]);
        if (fk->referencedTable().is_valid()) {
          for (size_t d = fk->referencedColumns().count(), j = 0; j < d; j++) {
            db_mysql_ColumnRef col = db_mysql_ColumnRef::cast_from(fk->referencedColumns().get(j));
            apply_defaults(col);
          }
        }
      }

      for (size_t k = 0, col_count = table->columns().count(); k < col_count; k++) {
        apply_defaults(table->columns().get(k));
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------

std::string bec::get_default_collation_for_charset(const db_SchemaRef &schema, const std::string &character_set)
{
  if (schema->owner().is_valid())
  {
    db_CatalogRef catalog(db_CatalogRef::cast_from(schema->owner()));
    db_CharacterSetRef charset = find_named_object_in_list(catalog->characterSets(), character_set);
    if (charset.is_valid()) {
      return charset->defaultCollation();
    }
  } else
    logWarning("While checking diff, catalog ref was found to be invalid\n");

  return "";
}

std::string bec::get_default_collation_for_charset(const db_TableRef &table, const std::string &character_set) {
  if (table->owner().is_valid())
    return bec::get_default_collation_for_charset(db_SchemaRef::cast_from(table->owner()), character_set);
  else
    logWarning("While checking diff, table ref was found to be invalid\n");
  return "";
}

std::string bec::TableHelper::generate_comment_text(const std::string &comment_text, size_t comment_lenght) {
  if (comment_text.size() > comment_lenght) {
    std::string comment, leftover;

    split_comment(comment_text, comment_lenght, &comment, &leftover);

    if (!comment.empty())
      comment = "'" + base::escape_sql_string(comment) + "'";
    if (!leftover.empty()) {
      base::replaceStringInplace(leftover, "*/", "*\\/");
      comment.append(" /* comment truncated */ /*").append(leftover).append("*/");
    }
    return comment;
  } else if (!comment_text.empty())
    return "'" + base::escape_sql_string(comment_text) + "'";

  return "";
}
